/*
 * Audio.c
 *
 *  Created on: 14 Dec 2025
 *      Author: lucia
 */

#include "Audio.h"

#include <math.h>

uint16_t audio_buffer[AUDIO_BUF_LEN];

static void AudioDMA_cb(void){
	// do nithing for now. later refill audio_buffer[]
	DMA_trigger = true;
//	DMA_SetEnableRequest(DMA_CH1, true);

}

static void PIT_cb(void){
	PIT_trigger = true;
}

void Audio_Init(void)
{
    PIT_Init(PIT_1, AUDIO_FS_HZ);
//    PIT_DisableInterrupt(PIT_1);
    PIT_SetCallback(PIT_cb, PIT_1);

    DAC_Init(DAC0);
    DAC_SetData(DAC0, DAC_MID); // midscale


    // DMA Periodic Trigger Mode: DMA channels 0-3
    DMA_Init();

    DMAMUX_ConfigChannel(DMA_CH1, true, true, kDmaRequestMux0AlwaysOn58);     // PIT --> DMAMUX --> DMA

    // TCD setup: audio_buffer -> DAC0 DAT0
    DMA_SetSourceAddr(DMA_CH1, (uint32_t)(audio_buffer));   // dirección de la fuente de datos
    DMA_SetDestAddr(DMA_CH1, (uint32_t)&DAC0->DAT[0]);     // dirección de destino (FTM0 CnV)

    DMA_SetSourceAddrOffset(DMA_CH1, 2); // cuanto se mueve la dirección de origen después de cada minor loop
    DMA_SetDestAddrOffset(DMA_CH1, 0);   // always write same DAC register

    DMA_SetSourceTransfSize(DMA_CH1, DMA_TransSize_16Bit);
    DMA_SetDestTransfSize(DMA_CH1, DMA_TransSize_16Bit);

    DMA_SetMinorLoopTransCount(DMA_CH1, 2); // 1 sample = 2 bytes

    // Major loop counts: set once

	DMA_SetStartMajorLoopCount(DMA_CH1, AUDIO_BUF_LEN);
	DMA_SetCurrMajorLoopCount (DMA_CH1, AUDIO_BUF_LEN);

    // Wrap source back to buffer start after major loop:
    // SLAST = -(bytes transferred per major loop) = -(2 * AUDIO_BUF_LEN)

    DMA_SetSourceLastAddrOffset(DMA_CH1, -(int32_t)(2 * AUDIO_BUF_LEN));

    // Destination does not move
    DMA_SetDestLastAddrOffset(DMA_CH1, 0);

    // Major-loop interrupt (buffer boundary)
    DMA_SetChannelInterrupt(DMA_CH1, true, AudioDMA_cb);

    // Continuous streaming: do NOT auto-disable requests at end
//    DMA0->TCD[DMA_CH0].CSR |= DMA_CSR_DREQ_MASK; // auto-disable request al terminar major loop
    DMA0->TCD[DMA_CH1].CSR &= ~DMA_CSR_DREQ_MASK;


    DMA_SetEnableRequest(DMA_CH1, true);
}


void build_sine_table(void)
{
    const float phase_inc =
        2.0f * 3.14159265358979323846f *
        ((float)SINE_FREQ_HZ / (float)AUDIO_FS_HZ);

    float phase = 0.0f;

    for (uint32_t i = 0; i < AUDIO_BUF_LEN; i++) {
        float s = sinf(phase);

        /* 80% full-scale sine centered at midscale */
        float y = (float)DAC_MID + 0.8f * (float)DAC_MID * s;

        if (y < 0.0f) y = 0.0f;
        if (y > (float)DAC_MAX) y = (float)DAC_MAX;

        audio_buffer[i] = (uint16_t)(y + 0.5f);

        phase += phase_inc;
        if (phase >= 2.0f * 3.14159265358979323846f)
            phase -= 2.0f * 3.14159265358979323846f;
    }
}
