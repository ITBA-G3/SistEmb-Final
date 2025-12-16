/*
 * FFT8Bands.h
 *
 *  Created on: 15 Dec 2025
 *      Author: lucia
 */

#ifndef FFT_H_
#define FFT_H_

#include <stdint.h>
#include <stddef.h>
#include "Audio.h"

#define FFT_N 1024u  // power of two

/**
 * @brief Initialize internal tables (window, twiddles).
 * Call once at startup.
 */
void FFT_Init(void);

/**
 * @brief Compute 8-band normalized energy (0..1) from a PCM frame.
 *
 * @param pcm        Pointer to signed 16-bit PCM samples (mono).
 * @param n          Number of samples available at pcm (must be >= FFT_N).
 * @param fs_hz      Sample rate in Hz (e.g., 48000).
 * @param out_bands  Output array of 8 floats, each in [0..1].
 */
void FFT_ComputeBands(const int16_t *pcm, size_t n, uint32_t fs_hz, float out_bands[8]);


#endif /* FFT_H_ */
