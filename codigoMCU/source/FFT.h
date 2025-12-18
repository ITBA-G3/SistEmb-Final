/**
 * @file FFT.h
 * @brief FFT-based spectrum analysis module.
 *
 * This module provides initialization and runtime functions to compute
 * an 8-band spectrum representation from PCM audio samples. It is optimized
 * for Cortex-M4F using the CMSIS-DSP library (arm_rfft_fast_f32).
 *
 * Typical usage:
 * - Call ::FFT_Init() once at startup
 * - Call ::FFT_ComputeBands() periodically with blocks of PCM samples
 *
 * @author   Grupo 3
  	  	  	  - Ezequiel Díaz Guzmán
  	  	  	  - José Iván Hertter
  	  	  	  - Cristian Damián Meichtry
  	  	  	  - Lucía Inés Ruiz
 */

#ifndef FFT_H_
#define FFT_H_

#include <stdint.h>
#include <stddef.h>
#include "Audio.h"

#define FFT_N 1024u  // power of two

/**
 * @brief Initialize the FFT processing module.
 *
 * This function performs all one-time initialization required by the FFT:
 * - Precomputes the Hann window of length FFT_N
 * - Initializes the CMSIS-DSP real FFT instance
 * - Computes the FFT bin indices corresponding to the fixed frequency bands
 *
 * It must be called exactly once before any call to ::FFT_ComputeBands().
 *
 * The function is not re-entrant.
 */
void FFT_Init(void);


/**
 * @brief Compute normalized spectrum band levels from PCM audio samples.
 *
 * Processes a block of PCM samples and computes 8 normalized band levels
 * suitable for visualization (e.g. VU meter or spectrum display).
 *
 * Processing steps:
 * - Mean subtraction (DC removal)
 * - Hann windowing
 * - Real FFT using CMSIS-DSP (arm_rfft_fast_f32)
 * - Magnitude-squared spectrum computation
 * - Integration over 8 fixed frequency bands
 * - Logarithmic (dB) scaling and normalization to [0.0 .. 1.0]
 *
 * If the input pointer is NULL or fewer than FFT_N samples are provided,
 * the output bands are set to zero.
 *
 * @param[in]  pcm
 *     Pointer to an array of PCM samples in signed 16-bit format.
 *     At least FFT_N samples must be available.
 *
 * @param[in]  n
 *     Number of samples available in @p pcm.
 *     Must be greater than or equal to FFT_N.
 *
 * @param[in]  fs_hz
 *     Sampling frequency in Hz.
 *     Currently unused internally; band edges are computed using AUDIO_FS_HZ.
 *
 * @param[out] out_bands
 *     Pointer to an array of 8 floats receiving the normalized band levels.
 *     Each value is clamped to the range [0.0 .. 1.0].
 */
void FFT_ComputeBands(const int16_t *pcm, size_t n, uint32_t fs_hz, float out_bands[8]);


#endif /* FFT_H_ */
