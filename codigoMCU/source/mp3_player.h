/*
 * mp3_player.h
 *
 *  Created on: 21 Dec 2025
 *      Author: hertt
 */


#pragma once
#include <stdint.h>
#include <stdbool.h>
#include "drivers/FAT/ff.h"

bool MP3Player_InitWithOpenFile(FIL *fp);
void MP3Player_FillDacBuffer(volatile uint16_t *dst, uint32_t n);

// Opcional para el próximo paso (timing correcto)
uint32_t MP3Player_GetSampleRateHz(void);
uint32_t MP3Player_GetChannels(void);
