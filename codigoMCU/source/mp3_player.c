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

static bool pcm_ring_push(int16_t s);

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
    space &= ~(uint32_t)0x1FF;   // múltiplo de 512
    if (space == 0) return true;
    FRESULT fr = f_read(g_fp, &g_inbuf[g_bytes_left], (UINT)space, &br);
    if (fr != FR_OK) {
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


bool MP3Player_DecodeOneFrameToRing(void)
{
    if (!g_fp || !g_hmp3) return false;

    if (!mp3_decode_next_frame()) {
        return false;
    }

    // g_pcm_total y g_pcm[] ya están cargados
    int idx = 0;

    if (g_fi.nChans == 2) {
        while (idx + 1 < g_pcm_total) {
            int16_t mono = g_pcm[idx];
            // int16_t R = g_pcm[idx + 1];
            // int16_t mono = (int16_t)(((int32_t)L + (int32_t)R) / 2);
            idx += 4;
            if (!pcm_ring_push(mono)) return true; // ring lleno: salir, ya hay data
        }
    } else {
        while (idx < g_pcm_total) {
            int16_t mono = g_pcm[idx++];
            if (!pcm_ring_push(mono)) return true;
        }
    }

    return true;
}

void MP3Player_GetLastPCMwindow(int16_t *pcm, uint32_t max_samples)
{
    if (!pcm || max_samples == 0) return 0;

    uint32_t to_copy = (uint32_t)g_pcm_total;
    if (to_copy > max_samples) {
        to_copy = max_samples;
    }

    for (uint32_t i = 0; i < to_copy; i++) {
        pcm[i] = g_pcm[i];
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

uint32_t pcm_ring_level(void)
{
    return (uint32_t)(g_pcm_wr - g_pcm_rd);
}

uint32_t pcm_ring_free(void)
{
    return PCM_RING_SIZE - pcm_ring_level();
}

static bool pcm_ring_push(int16_t s)
{
    if (pcm_ring_free() == 0) return false;
    g_pcm_ring[g_pcm_wr & PCM_RING_MASK] = s;
    g_pcm_wr++;
    return true;
}

uint32_t pcm_ring_pop_block(uint16_t *dst, uint32_t n)
{
    uint32_t avail = pcm_ring_level();
    if (n > avail) n = avail;

    for (uint32_t i = 0; i < n; i++) {
        dst[i] = g_pcm_ring[g_pcm_rd & PCM_RING_MASK];
        dst[i] = pcm16_to_dac(dst[i]);
        g_pcm_rd++;
    }
    return n;
}
