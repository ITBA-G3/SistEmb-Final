/*
 * mp3_player.c
 *
 *  Created on: 21 Dec 2025
 *      Author: hertt
 */


#include "mp3_player.h"
#include "helix/pub/mp3dec.h"
#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

// Ajustes
#define MP3_INBUF_SZ   4096
#define MP3_MIN_FILL   1024

#ifndef DAC_MAX
#define DAC_MAX 4095u
#endif
#ifndef DAC_MID
#define DAC_MID (DAC_MAX/2u)
#endif

static FIL *g_fp = NULL;
static HMP3Decoder g_hmp3 = NULL;
static MP3FrameInfo g_fi;

static uint8_t  g_inbuf[MP3_INBUF_SZ];
static int      g_bytes_left = 0;
static uint8_t *g_read_ptr   = g_inbuf;

// PCM temporal (máx)
static int16_t g_pcm[1152 * 2];
static int     g_pcm_total = 0;
static int     g_pcm_idx   = 0;

// Para inspección si querés (no printf)
volatile uint32_t g_mp3_decode_errs = 0;
volatile uint32_t g_mp3_frames_ok   = 0;

// --- ID3v2 skip (recomendado) ---
static bool mp3_skip_id3v2(FIL *fp)
{
    UINT br = 0;
    uint8_t hdr[10];

    if (f_lseek(fp, 0) != FR_OK) return false;
    if (f_read(fp, hdr, sizeof(hdr), &br) != FR_OK || br != sizeof(hdr)) return false;

    if (hdr[0]=='I' && hdr[1]=='D' && hdr[2]=='3') {
        uint32_t sz =
            ((uint32_t)(hdr[6] & 0x7F) << 21) |
            ((uint32_t)(hdr[7] & 0x7F) << 14) |
            ((uint32_t)(hdr[8] & 0x7F) << 7)  |
            ((uint32_t)(hdr[9] & 0x7F) << 0);
        return (f_lseek(fp, 10u + sz) == FR_OK);
    }
    return (f_lseek(fp, 0) == FR_OK);
}

static bool mp3_fill_inbuf(void)
{
    if (g_bytes_left >= MP3_MIN_FILL) return true;

    // compactar bytes restantes al inicio
    if (g_read_ptr != g_inbuf && g_bytes_left > 0) {
        for (int i = 0; i < g_bytes_left; i++) {
            g_inbuf[i] = g_read_ptr[i];
        }
        g_read_ptr = g_inbuf;
    } else if (g_bytes_left == 0) {
        g_read_ptr = g_inbuf;
    }

    UINT br = 0;
    uint32_t space = (uint32_t)MP3_INBUF_SZ - (uint32_t)g_bytes_left;
    if (space == 0) return true;

    if (f_read(g_fp, &g_inbuf[g_bytes_left], (UINT)space, &br) != FR_OK) {
        return false;
    }
    g_bytes_left += (int)br;
    return true;
}

static bool mp3_decode_next_frame(void)
{
    if (!mp3_fill_inbuf()) return false;
    if (g_bytes_left < 4) return false;

    int off = MP3FindSyncWord(g_read_ptr, g_bytes_left);
    if (off < 0) {
        // descartar casi todo y reintentar
        if (g_bytes_left > 16) {
            g_read_ptr += (g_bytes_left - 16);
            g_bytes_left = 16;
        }
        return false;
    }

    g_read_ptr += off;
    g_bytes_left -= off;

    int err = MP3Decode(g_hmp3, &g_read_ptr, &g_bytes_left, g_pcm, 0);
    if (err != 0) {
        g_mp3_decode_errs++;
        // avanzar 1 byte para resync
        if (g_bytes_left > 0) { g_read_ptr++; g_bytes_left--; }
        return false;
    }

    MP3GetLastFrameInfo(g_hmp3, &g_fi);

    // outputSamps suele venir como total interleaved (stereo => 2304)
    g_pcm_total = g_fi.outputSamps;
    g_pcm_idx   = 0;

    g_mp3_frames_ok++;
    return (g_pcm_total > 0);
}

static inline uint16_t pcm16_to_dac(int16_t s)
{
    int32_t y = (int32_t)DAC_MID + ((int32_t)s * (int32_t)DAC_MID) / 32768;
    if (y < 0) y = 0;
    if (y > (int32_t)DAC_MAX) y = (int32_t)DAC_MAX;
    return (uint16_t)y;
}

// API
bool MP3Player_InitWithOpenFile(FIL *fp)
{
    if (!fp) return false;

    g_fp = fp;

    if (!mp3_skip_id3v2(g_fp)) return false;

    g_hmp3 = MP3InitDecoder();
    if (!g_hmp3) return false;

    g_read_ptr = g_inbuf;
    g_bytes_left = 0;
    g_pcm_total = 0;
    g_pcm_idx = 0;

    // Decodificar un frame para fijar frameInfo (samprate/chans) temprano
    // Si falla, no abortamos: el Fill va a ir intentando.
    (void)mp3_decode_next_frame();

    return true;
}

void MP3Player_FillDacBuffer(volatile uint16_t *dst, uint32_t n)
{
    if (!dst || n == 0) return;

    if (!g_fp || !g_hmp3) {
        for (uint32_t i = 0; i < n; i++) dst[i] = (uint16_t)DAC_MID;
        return;
    }

    uint32_t out = 0;
    while (out < n) {
        if (g_pcm_idx >= g_pcm_total) {
            if (!mp3_decode_next_frame()) {
                // underrun o EOF => midscale
                dst[out++] = (uint16_t)DAC_MID;
                continue;
            }
        }

        int16_t mono = 0;

        if (g_fi.nChans == 2) {
            if (g_pcm_idx + 1 < g_pcm_total) {
                int16_t L = g_pcm[g_pcm_idx];
                int16_t R = g_pcm[g_pcm_idx + 1];
                mono = (int16_t)(((int32_t)L + (int32_t)R) / 2);
                g_pcm_idx += 2;
            } else {
                g_pcm_idx = g_pcm_total;
                continue;
            }
        } else {
            mono = g_pcm[g_pcm_idx++];
        }

        dst[out++] = pcm16_to_dac(mono);
    }
}

uint32_t MP3Player_GetSampleRateHz(void)
{
    return (uint32_t)g_fi.samprate;
}

uint32_t MP3Player_GetChannels(void)
{
    return (uint32_t)g_fi.nChans;
}
