/**
 * @file     Audio.c
 * @brief Audio module implementation.
 *
 * This file contains the implementation of the audio streaming logic:
 * - PIT configuration for sample-rate timing
 * - DAC initialization
 * - DMA configuration for transferring samples to the DAC
 * - Ping-pong buffer swap on DMA major-loop completion
 * - Background buffer refilling via ::Audio_Service()
 * @author   Grupo 3
  	  	  	  - Ezequiel Díaz Guzmán
  	  	  	  - José Iván Hertter
  	  	  	  - Cristian Damián Meichtry
  	  	  	  - Lucía Inés Ruiz
 */


#include "Audio.h"
#include <math.h>
#include <stdint.h>
#include <stdbool.h>

// Internal states
static volatile uint16_t *g_playing = NULL;     // buffer DMA is currently playing
static volatile uint16_t *g_fill_next = NULL;   // buffer CPU should refill next
static volatile bool g_need_fill = false;

static float g_phase = 0.0f;

static void Audio_FillSine(volatile uint16_t *dst, uint32_t n);


/**
 * @brief DMA major-loop completion callback.
 *
 * This callback is executed at the end of each DMA major loop, corresponding
 * to the playback of one complete audio buffer. It swaps the active ping-pong
 * buffers, reconfigures the DMA source address to the next buffer, and signals
 * the main loop that the finished buffer is ready to be refilled.
 *
 * @note This function is called from DMA interrupt context.
 */
static void AudioDMA_cb(void){
	DMA_trigger = true;

	// 1) Disable further requests while reprogramming
	DMA_SetEnableRequest(DMA_CH1, false);          // or clear ERQ for that channel

	// 2) Clear interrupt flags / DONE (API depends on your driver)
	DMA_ClearChannelIntFlag(DMA_CH1);

    volatile uint16_t *just_finished = g_playing;
    volatile uint16_t *next = (g_playing == bufA) ? bufB : bufA;

    g_playing = next;

    DMA_SetSourceAddr(DMA_CH1, (uint32_t)next);		// Change DMA source to the next buffer and restart major loop

    DMA_SetCurrMajorLoopCount(DMA_CH1, AUDIO_BUF_LEN);	    // Reset loop counts for the new major loop
    DMA_SetStartMajorLoopCount(DMA_CH1, AUDIO_BUF_LEN);


//    DMA_SetSourceLastAddrOffset(DMA_CH1, -(int32_t)(2 * AUDIO_BUF_LEN));    // Source wraps back to the start at the end of the major loop

    DMA_SetEnableRequest(DMA_CH1, true);

    g_fill_next = just_finished;    // Tell main loop which buffer to refill
    g_need_fill = true;
}

/**
 * @brief PIT interrupt callback.
 *
 * This callback is invoked on each PIT trigger event and is used to signal
 * that a new audio sample period has elapsed.
 *
 * @note The PIT interrupt itself is not required for audio streaming, but
 *       this callback can be used for debugging or timing instrumentation.
 */
static void PIT_cb(void){
	PIT_trigger = true;
}

/**
 * @brief Initializes the audio output subsystem.
 *
 * Configures the PIT as the audio sample-rate timebase, initializes the DAC,
 * sets up the DMA channel for ping-pong buffer operation, and starts continuous
 * audio streaming from memory to the DAC.
 *
 * Both ping-pong buffers are pre-filled before enabling DMA to avoid initial
 * underrun conditions.
 */
void Audio_Init(void)
{
    PIT_Init(PIT_1, AUDIO_FS_HZ);
//    PIT_DisableInterrupt(PIT_1);		// i don't really need the pit irq
    PIT_SetCallback(PIT_cb, PIT_1);

    DAC_Init(DAC0);
    DAC_SetData(DAC0, DAC_MID); // midscale


    // Pre-fill both buffers before starting DMA
	Audio_FillSine(bufA, AUDIO_BUF_LEN);
	Audio_FillSine(bufB, AUDIO_BUF_LEN);

	g_playing   = bufA;
	g_fill_next = bufB;
	g_need_fill = false;

	// DMA/DMAMUX
    DMA_Init();

    DMAMUX_ConfigChannel(DMA_CH1, true, true, kDmaRequestMux0AlwaysOn58);     // PIT --> DMAMUX --> DMA

    // TCD setup: audio_buffer -> DAC0 DAT0
    DMA_SetSourceAddr(DMA_CH1, (uint32_t)(bufA));   // dirección de la fuente de datos
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
    DMA_SetSourceLastAddrOffset(DMA_CH1, -(int32_t)(2 * AUDIO_BUF_LEN));

    // Destination does not move
    DMA_SetDestLastAddrOffset(DMA_CH1, 0);

    // Major-loop interrupt (buffer boundary)
    DMA_SetChannelInterrupt(DMA_CH1, true, AudioDMA_cb);

    // Continuous streaming: do NOT auto-disable requests at end
    DMA0->TCD[DMA_CH1].CSR &= ~DMA_CSR_DREQ_MASK;

    DMA_SetEnableRequest(DMA_CH1, true);
}


/**
 * @brief Audio background service routine.
 *
 * This function must be called periodically from the main application loop.
 * When signaled by the DMA callback, it refills the buffer that has just
 * finished playing. In the current implementation, the buffer is filled
 * with a test sine waveform.
 *
 * @note In the final system, this function will consume decoded PCM samples
 *       instead of generating a test tone.
 */
void Audio_Service(void)
{
    volatile uint16_t *dst = NULL;

    __disable_irq();
    if (g_need_fill) {
        dst = g_fill_next;   // take ownership of the buffer to fill
        g_need_fill = false; // consume the request
    }
    __enable_irq();

    if (!dst) return;

    Audio_FillSine(dst, AUDIO_BUF_LEN);
}

/**
 * @brief Fills an audio buffer with a continuous-phase sine wave.
 *
 * Generates a sine waveform at the configured test frequency and sample rate,
 * converts it to the DAC numeric range, and stores it in the provided buffer.
 * Phase continuity is preserved across consecutive calls.
 *
 * @param dst Pointer to the destination audio buffer.
 * @param n   Number of samples to generate.
 */
static void Audio_FillSine(volatile uint16_t *dst, uint32_t n)
{
    const float phase_inc =
        2.0f * 3.14159265358979323846f *
        ((float)SINE_FREQ_HZ / (float)AUDIO_FS_HZ);

    for (uint32_t i = 0; i < n; i++) {
        float s = sinf(g_phase);

        float y = (float)DAC_MID + 0.8f * (float)DAC_MID * s;
        if (y < 0.0f) y = 0.0f;
        if (y > (float)DAC_MAX) y = (float)DAC_MAX;

        dst[i] = (uint16_t)(y + 0.5f);

        g_phase += phase_inc;
        if (g_phase >= 2.0f * 3.14159265358979323846f)
            g_phase -= 2.0f * 3.14159265358979323846f;
    }
}
