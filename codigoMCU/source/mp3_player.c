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
#define MP3_MIN_FILL   2048

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
static int16_t g_pcm[1152 * 8];//TODO si funciona volver acá
static int     g_pcm_total = 0;
static int     g_pcm_idx   = 0;

// Para inspección si querés (no printf)
volatile uint32_t g_mp3_decode_errs = 0;
volatile uint32_t g_mp3_frames_ok   = 0;

static uint32_t pcm_ring_push_left_block(const int16_t *pcm, uint32_t interleaved_samps, uint32_t *io_idx);

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
    // compactar bytes restantes al inicio
    if (g_read_ptr != g_inbuf && g_bytes_left > 0) {
        for (int i = 0; i < g_bytes_left; i++) g_inbuf[i] = g_read_ptr[i];
        g_read_ptr = g_inbuf;
    } else if (g_bytes_left == 0) {
        g_read_ptr = g_inbuf;
    }

    while (g_bytes_left < MP3_MIN_FILL) {
        uint32_t space = (uint32_t)MP3_INBUF_SZ - (uint32_t)g_bytes_left;
        if (space == 0) break;

        UINT br = 0;
        FRESULT fr = f_read(g_fp, &g_inbuf[g_bytes_left], (UINT)space, &br);
        if (fr != FR_OK) return false;

        if (br == 0) {
            // EOF real (o no hay más datos)
            break;
        }

        g_bytes_left += (int)br;
    }

    // devolvés true aunque no llegues a MIN_FILL si al menos tenés algo;
    // pero ahora la chance de quedar “corto” baja muchísimo
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


//bool MP3Player_DecodeOneFrameToRing(void)
//{
//    if (!g_fp || !g_hmp3) return false;
//
//    if (!mp3_decode_next_frame()) {
//        return false;
//    }
//
//    // g_pcm_total y g_pcm[] ya están cargados
//    int idx = 0;
//
//    if (g_fi.nChans == 2) {
//        while (idx + 1 < g_pcm_total) {
//            int16_t mono = g_pcm[idx];
//            // int16_t R = g_pcm[idx + 1];
//            // int16_t mono = (int16_t)(((int32_t)L + (int32_t)R) / 2);
//            idx += 2;
//            if (!pcm_ring_push(mono)) return true; // ring lleno: salir, ya hay data
//        }
//    } else {
//        while (idx < g_pcm_total) {
//            int16_t mono = g_pcm[idx++];
//            if (!pcm_ring_push(mono)) return true;
//        }
//    }
//
//    return true;
//}

////////////PRUEBAS
bool MP3Player_DecodeOneFrameToRing(void)
{
    if (!g_fp || !g_hmp3) return false;

    // Mientras haya espacio en ring, empujar audio
    // while (pcm_ring_free() > 0) {
    for(int i = 0; i < 512; i++){
        // Si no quedan samples del frame actual => decodificar otro
        if (g_pcm_idx >= g_pcm_total-2048) {
            if (!mp3_decode_next_frame()) {
                return false; // EOF/underrun real
            }
        }
        
        if (g_fi.nChans == 2) {
            // Empujar solo LEFT en bloque hasta que ring se llene o se acabe frame
            pcm_ring_push_left_block(g_pcm, (uint32_t)g_pcm_total, (uint32_t*)&g_pcm_idx);
        } else {
            // Mono: empujar lo que se pueda
            while (g_pcm_idx < g_pcm_total && pcm_ring_free() > 0) {
                g_pcm_ring[g_pcm_wr & PCM_RING_MASK] = g_pcm[g_pcm_idx++];
                g_pcm_wr++;
            }
        }
        
        // Si ring se llenó, cortamos ya (sin seguir “recorriendo por deporte”)
        if (pcm_ring_free() == 0) break;
    }
    // }

    return true;
}

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
        // Si no hay lugar, no tiene sentido decodificar más.
        if (pcm_ring_free() == 0) {
            return progressed ? true : false; // ring lleno: hiciste “lo posible”
        }

        // Decodificar un frame
        if (!mp3_decode_next_frame()) {
            // Si ya empujaste algo antes, devolvé true (hiciste progreso)
            // Si no empujaste nada, devolvé false (no hubo progreso)
            return progressed ? true : false;
        }

        // Empujar el frame decodificado en bloque
        uint32_t idx = 0;

        if (g_fi.nChans == 2) {
            // stereo interleaved -> mono (left)
        	uint32_t pushed = pcm_ring_push_left_block(g_pcm, (uint32_t)g_pcm_total, &idx);
            if (pushed > 0) progressed = true;

            // Si no pudiste empujar nada (ring lleno), cortá
            if (pushed == 0) return true;

        } else {
            // mono -> mono
            uint32_t pushed = pcm_ring_push_mono_block(g_pcm, (uint32_t)g_pcm_total, &idx);
            if (pushed > 0) progressed = true;

            if (pushed == 0) return true;
        }

        // Si por alguna razón quedó PCM sin empujar (ring chico), salís:
        if (idx < (uint32_t)g_pcm_total) {
            return true;
        }

        // Si querés evitar monopolizar CPU, podés poner un “budget” de frames por llamada:
        // if (++frames_decoded >= 2) return true;
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

uint32_t pcm_ring_pop_block(volatile uint16_t *dst, uint32_t n)
{
    uint32_t avail = pcm_ring_level();
    if (n > avail) n = avail;

    for (uint32_t i = 0; i < n; i++) {
        int16_t s = g_pcm_ring[g_pcm_rd & PCM_RING_MASK];
        g_pcm_rd++;
        dst[i] = pcm16_to_dac(s);
    }
    return n;
}

static uint32_t pcm_ring_push_left_block(const int16_t *pcm, uint32_t interleaved_samps, uint32_t *io_idx)
{
    // interleaved_samps = g_pcm_total (ej: stereo => L,R,L,R...)
    // io_idx: índice dentro de pcm[] (interleaved)
    uint32_t pushed = 0;

    while (*io_idx < interleaved_samps) {

        uint32_t free = pcm_ring_free();
        if (free == 0) break;

        // Queremos empujar "mono" tomando solo L => consumimos 2 ints por sample mono
        // Cantidad máxima de monos que podemos sacar de lo que queda del frame:
        uint32_t remaining_inter = interleaved_samps - *io_idx;
        uint32_t remaining_mono  = remaining_inter / 2;          // porque stereo consume 2 por mono (L,R)
        if (remaining_mono == 0) break;

        uint32_t n = free;
        if (n > remaining_mono) n = remaining_mono;

        // Escribimos n samples mono al ring
        for (uint32_t k = 0; k < n; k++) {
            int16_t L = pcm[*io_idx];     // left
            *io_idx += 2;                 // saltar right
            g_pcm_ring[g_pcm_wr & PCM_RING_MASK] = L;
            g_pcm_wr++;
        }
        pushed += n;
    }

    return pushed;
}
