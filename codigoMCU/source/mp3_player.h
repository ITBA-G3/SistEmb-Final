/*
 * mp3_player.h
 *      Author: Grupo 3
 */

#pragma once
#include <stdint.h>
#include <stdbool.h>
#include "drivers/FAT/ff.h"

#define PCM_RING_SIZE 16384u
#define PCM_RING_MASK (PCM_RING_SIZE - 1u)

static int16_t g_pcm_ring[PCM_RING_SIZE];
static volatile uint32_t g_pcm_wr = 0;
static volatile uint32_t g_pcm_rd = 0;


bool MP3Player_InitWithOpenFile(FIL *fp);
void MP3Player_FillDacBuffer(volatile uint16_t *dst, uint32_t n);

uint32_t MP3Player_GetSampleRateHz(void);
uint32_t MP3Player_GetChannels(void);
void MP3Player_GetLastPCMwindow(int16_t *pcm, uint32_t max_samples);

bool MP3Player_DecodeAsMuchAsPossibleToRing(void);

uint32_t pcm_ring_level(void);
uint32_t pcm_ring_free(void);
uint32_t pcm_ring_pop_block(volatile uint16_t *dst, uint32_t n);

void pcm_ring_clear(void);

//para saber si un archivo es .mp3
bool is_mp3_file(const char *name);