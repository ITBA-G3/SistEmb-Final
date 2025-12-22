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
#include "mp3_player.h"
#include "drivers/gpio.h"
#include "os.h"

// Internal states
static volatile uint16_t *g_playing = NULL;     // buffer DMA is currently playing
static volatile uint16_t *g_fill_next = NULL;   // buffer CPU should refill next
static volatile bool g_need_fill = false;

//static float g_phase = 0.0f;


extern OS_SEM g_AudioSem;


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
    OS_ERR err;

    gpioWrite(PORTNUM2PIN(PC,11), HIGH);

    OSIntEnter();

    DMA_SetEnableRequest(DMA_CH1, false);          // clear ERQ for that channel
	DMA_ClearChannelIntFlag(DMA_CH1);
    
    volatile uint16_t *just_finished = g_playing;
    volatile uint16_t *next = (g_playing == bufA) ? bufB : bufA;

    g_playing = next;
    g_fill_next = just_finished;    // Tell main loop which buffer to refill

    // ---- FLAG BINARIO ----
    if (!g_need_fill) {
        g_need_fill = true;
        OSSemPost(&g_AudioSem, OS_OPT_POST_1, &err);
    }
    
    // g_need_fill = true;
    
    DMA_SetSourceAddr(DMA_CH1, (uint32_t)next);		// Change DMA source to the next buffer and restart major loop
    DMA_SetCurrMajorLoopCount(DMA_CH1, AUDIO_BUF_LEN);	    // Reset loop counts for the new major loop
    DMA_SetStartMajorLoopCount(DMA_CH1, AUDIO_BUF_LEN);
    DMA_SetEnableRequest(DMA_CH1, true);
    
    OSIntExit();
    
    gpioWrite(PORTNUM2PIN(PC,11), LOW); 
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
void Audio_Init()
{
    PIT_Init(PIT_1, 44100);
    PIT_DisableInterrupt(PIT_1);		// i don't really need the pit irq
    // PIT_SetCallback(PIT_cb, PIT_1);

    DAC_Init(DAC0);
    DAC_SetData(DAC0, DAC_MID); // midscale


    // Pre-fill both buffers before starting DMA
	// Audio_FillSine(bufA, AUDIO_BUF_LEN);
	// Audio_FillSine(bufB, AUDIO_BUF_LEN);

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

    // Destination does not move
    DMA_SetDestLastAddrOffset(DMA_CH1, 0);

    // Major-loop interrupt (buffer boundary)
    DMA_SetChannelInterrupt(DMA_CH1, true, AudioDMA_cb);

    // Continuous streaming: do NOT auto-disable requests at end
    DMA0->TCD[DMA_CH1].CSR &= ~DMA_CSR_DREQ_MASK;

    DMA_SetEnableRequest(DMA_CH1, true);
}


static inline uint16_t pcm16_to_dac(int16_t s)
{
    int32_t y = (int32_t)DAC_MID + ((int32_t)s * (int32_t)DAC_MID) / 32768;
    if (y < 0) y = 0;
    if (y > (int32_t)DAC_MAX) y = (int32_t)DAC_MAX;
    return (uint16_t)y;
}

/**
 * @brief Audio background service routine.
 *
 * This function must be called periodically from the main application loop.
 * When signaled by the DMA callback, it refills the buffer that has just
 * finished playing.
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

//    if (pcm_ring_level() > AUDIO_BUF_LEN) {
    uint32_t got = pcm_ring_pop_block(dst, AUDIO_BUF_LEN);
//    }
    for (uint32_t i = got; i < AUDIO_BUF_LEN; i++) {
            dst[i] = (uint16_t)DAC_MID;
	}
}
