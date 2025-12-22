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
#include "MK64F12.h"
#include "drivers/gpio.h"
#include <string.h>


// Ajustes
#define MP3_INBUF_SZ   16384
#define MP3_MIN_FILL   8192
#define MP3_READ_CHUNK 4096

#ifndef DAC_MAX
#define DAC_MAX 4095u
#endif
#ifndef DAC_MID
#define DAC_MID (DAC_MAX/2u)
#endif

// static bool pcm_ring_push(int16_t s);

static FIL *g_fp = NULL;
static HMP3Decoder g_hmp3 = NULL;
static MP3FrameInfo g_fi;

static uint8_t  g_inbuf[MP3_INBUF_SZ];          // lo que traigo de la sd
static int      g_bytes_left = 0;
static uint8_t *g_read_ptr   = g_inbuf;

// PCM temporal (máx)
static int16_t g_pcm[1152*8];//TODO si funciona volver acá
static int     g_pcm_total = 0;
static int     g_pcm_idx   = 0;

// Para inspección si querés (no printf)
volatile uint32_t g_mp3_decode_errs = 0;
volatile uint32_t g_mp3_frames_ok   = 0;

static uint32_t pcm_ring_push_left_block(const int16_t *pcm, uint32_t interleaved_samps, uint32_t *io_idx);
static inline void pcm_ring_snapshot(uint32_t *rd, uint32_t *wr);

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
//	gpioWrite(PORTNUM2PIN(PC,11),HIGH);

    // Compactar (mejor con memmove)
    if (g_read_ptr != g_inbuf && g_bytes_left > 0) {
        memmove(g_inbuf, g_read_ptr, (size_t)g_bytes_left);
        g_read_ptr = g_inbuf;
    } else if (g_bytes_left == 0) {
        g_read_ptr = g_inbuf;
    }

    uint32_t space = (uint32_t)MP3_INBUF_SZ - (uint32_t)g_bytes_left;
    if (space == 0) return true;

    // Leer chunk grande (cap), preferentemente múltiplo de 512
    uint32_t to_read = space;
    if (to_read > MP3_READ_CHUNK) to_read = MP3_READ_CHUNK;

    // Alinear hacia abajo a 512 si no te deja en 0
    uint32_t aligned = to_read & ~0x1FFu;
    if (aligned >= 512u) to_read = aligned;  // si queda muy chico, usá lo que haya

    UINT br = 0;
    FRESULT fr = f_read(g_fp, &g_inbuf[g_bytes_left], (UINT)to_read, &br);
    if (fr != FR_OK) return false;

    g_bytes_left += (int)br;
//    gpioWrite(PORTNUM2PIN(PC,11),LOW);
    return (g_bytes_left > 0);
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

    gpioWrite(PORTNUM2PIN(PC,11),HIGH);
    int err = MP3Decode(g_hmp3, &g_read_ptr, &g_bytes_left, g_pcm, 0);
    gpioWrite(PORTNUM2PIN(PC,11),LOW);
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
    // s: -32768..32767
    // shift to unsigned 16-bit: 0..65535
    uint16_t u = (uint16_t)(s + 32768);
    // take top 12 bits -> 0..4095
    return (u >> 4);
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

////////////PRUEBAS

static uint32_t pcm_ring_push_mono_block(const int16_t *pcm, uint32_t samps, uint32_t *io_idx)
{
    uint32_t pushed = 0;

    while (*io_idx < samps) {
        uint32_t free = pcm_ring_free();
        if (free == 0) break;

        uint32_t remaining = samps - *io_idx;
        uint32_t n = free;
        if (n > remaining) n = remaining;

        for (uint32_t k = 0; k < n; k++) {
            g_pcm_ring[g_pcm_wr & PCM_RING_MASK] = pcm[*io_idx];
            (*io_idx)++;
            g_pcm_wr++;
        }
        pushed += n;
    }
    return pushed;
}

bool MP3Player_DecodeAsMuchAsPossibleToRing(void)
{
    if (!g_fp || !g_hmp3) return false;

    bool progressed = false;

    for (;;) {

        // 1) Si no hay lugar, cortar
        if (pcm_ring_free() == 0) return progressed;

        // 2) Si NO hay PCM pendiente, decodificar un frame nuevo
        if (g_pcm_idx >= g_pcm_total) {
//        	gpioWrite(PORTNUM2PIN(PC,11),HIGH);
            if (!mp3_decode_next_frame()) {
                return progressed ? true : false;
            }
//            gpioWrite(PORTNUM2PIN(PC,11),LOW);
            // mp3_decode_next_frame() deja g_pcm_total y g_pcm_idx=0
        }

        // 3) Empujar PCM pendiente del frame actual
        uint32_t before = (uint32_t)g_pcm_idx;

        if (g_fi.nChans == 2) {
            (void)pcm_ring_push_left_block(g_pcm,
                                           (uint32_t)g_pcm_total,
                                           (uint32_t*)&g_pcm_idx);
        } else {
            (void)pcm_ring_push_mono_block(g_pcm,
                                           (uint32_t)g_pcm_total,
                                           (uint32_t*)&g_pcm_idx);
        }

        // 4) Progreso o no
        if ((uint32_t)g_pcm_idx != before) progressed = true;

        // Si no pudiste empujar nada, ring lleno (o algo raro): cortar
        if ((uint32_t)g_pcm_idx == before) return progressed;

        // Opcional: no monopolizar CPU
        // break; o OSTimeDly(1) fuera, según tu arquitectura
    }
}




void MP3Player_GetLastPCMwindow(int16_t *pcm, uint32_t max_samples)
{
    if (!pcm || max_samples == 0) return;

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

static inline void pcm_ring_snapshot(uint32_t *rd, uint32_t *wr)
{
    __disable_irq();
    *rd = g_pcm_rd;
    *wr = g_pcm_wr;
    __enable_irq();
}

uint32_t pcm_ring_level(void)
{
    uint32_t rd, wr;
    pcm_ring_snapshot(&rd, &wr);
    return (uint32_t)(wr - rd);
}

uint32_t pcm_ring_free(void)
{
    uint32_t rd, wr;
    pcm_ring_snapshot(&rd, &wr);
    return (uint32_t)(PCM_RING_SIZE - (wr - rd));
}


// static bool pcm_ring_push(int16_t s)
// {
//     if (pcm_ring_free() == 0) return false;
//     g_pcm_ring[g_pcm_wr & PCM_RING_MASK] = s;
//     g_pcm_wr++;
//     return true;
// }

uint32_t pcm_ring_pop_block(volatile uint16_t *dst, uint32_t n)
{
    uint32_t rd, wr;

    __disable_irq();
    rd = g_pcm_rd;
    wr = g_pcm_wr;
    __enable_irq();

    uint32_t avail = (wr - rd);            // asumiendo indices crecientes
    if (n > avail) n = avail;

    for (uint32_t i = 0; i < n; i++) {
        int16_t s = g_pcm_ring[(rd + i) & PCM_RING_MASK];
        dst[i] = pcm16_to_dac(s);
    }

    __disable_irq();
    g_pcm_rd = rd + n;
    __enable_irq();

    return n;
}

static uint32_t pcm_ring_push_left_block(const int16_t *pcm,
                                         uint32_t interleaved_samps,
                                         uint32_t *io_idx)
{
    uint32_t pushed = 0;
    uint32_t rd, wr;

    while (*io_idx < interleaved_samps) {

        __disable_irq();
        rd = g_pcm_rd;
        wr = g_pcm_wr;
        __enable_irq();

        uint32_t free = PCM_RING_SIZE - (wr - rd);
        if (free == 0) break;

        uint32_t remaining_inter = interleaved_samps - *io_idx;
        uint32_t remaining_mono  = remaining_inter / 2;
        if (remaining_mono == 0) break;

        uint32_t n = (free < remaining_mono) ? free : remaining_mono;

        // escribir usando un wr_local, sin tocar g_pcm_wr todavía
        uint32_t wr_local = wr;
        for (uint32_t k = 0; k < n; k++) {
            int16_t L = pcm[*io_idx];
            *io_idx += 2;
            g_pcm_ring[wr_local & PCM_RING_MASK] = L;
            wr_local++;
        }

        __disable_irq();
        g_pcm_wr = wr_local;
        __enable_irq();

        pushed += n;
    }

    return pushed;
}

