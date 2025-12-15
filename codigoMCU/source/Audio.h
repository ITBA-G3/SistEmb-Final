/*
 * Audio.h
 *
 *  Created on: 14 Dec 2025
 *      Author: lucia
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

extern uint16_t audio_buffer[AUDIO_BUF_LEN];
extern volatile bool PIT_trigger;
extern volatile bool DMA_trigger;

void Audio_Init(void);
void build_sine_table(void); // FOR TESTING
void build_ramp(void);

#endif /* AUDIO_H_ */
