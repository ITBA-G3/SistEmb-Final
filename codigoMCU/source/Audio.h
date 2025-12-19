/**
 * @file     Audio.h
 * @brief Audio output driver using PIT-triggered DMA to DAC with ping-pong buffering.
 *
 * This module implements a continuous audio streaming path from RAM to the DAC
 * using the Kinetis K64 PIT as a sample-rate timebase and eDMA for transfers.
 * A ping-pong (double) buffer scheme is used so that one buffer is played by DMA
 * while the other buffer is refilled by the CPU in the background.
 *
 * The current implementation refills buffers with a generated sine wave for
 * validation. In the final system, buffers will be refilled with decoded PCM
 * audio samples (e.g., from an MP3 decoder).
 *
 * @note The application must call ::Audio_Service() periodically to keep the
 *       ping-pong buffers refilled and avoid underruns.
 *
 * @author   Grupo 3
  	  	  	  - Ezequiel Díaz Guzmán
  	  	  	  - José Iván Hertter
  	  	  	  - Cristian Damián Meichtry
  	  	  	  - Lucía Inés Ruiz
 */

#ifndef AUDIO_H_
#define AUDIO_H_

#include "MK64F12.h"
#include "hardware.h"
#include "drivers/PIT.h"
#include "drivers/DMA/DMA.h"
#include "drivers/DAC/DAC.h"

#define AUDIO_FS_HZ     48000u      // sample rate
#define SINE_FREQ_HZ    1000u       // 1 kHz test tone
#define AUDIO_BUF_LEN   512u        // must match DMA major loop
#define DAC_BITS        12u
#define DAC_MAX         ((1u << DAC_BITS) - 1u)
#define DAC_MID         (DAC_MAX / 2u)

// Flags defined in App.c as globals
extern volatile bool PIT_trigger;
extern volatile bool DMA_trigger;

// Ping-pong buffers are defined in App.c
extern volatile uint16_t bufA[AUDIO_BUF_LEN];
extern volatile uint16_t bufB[AUDIO_BUF_LEN];

/**
 * @brief Initializes the audio module.
 *
 * Sets up PIT timing, DAC output, DMA transfers, and internal state required
 * for ping-pong buffered audio playback.
 */
void Audio_Init(void);

/**
 * @brief Services audio buffer refilling.
 *
 * This function must be called from the main loop to refill audio buffers
 * once they are released by the DMA engine.
 */
void Audio_Service(void);

#endif /* AUDIO_H_ */
