/**
 * @file     ws2812_ftm.c
 * @brief    WS2812 transport implementation using FTM and DMA.
 *
 * This file contains the low-level WS2812 driver implementation:
 * - FTM configuration for WS2812 timing generation
 * - DMA setup for automatic CnV register updates
 * - Bitstream generation from GRB pixel data
 * - DMA-based, non-blocking LED frame transmission
 *
 * @author   Grupo 3
 *           - Ezequiel Díaz Guzmán
 *           - José Iván Hertter
 *           - Cristian Damián Meichtry
 *           - Lucía Inés Ruiz
 */

#include "ws2812_ftm.h"
#include "FTM.h"
#include <stdint.h>
#include <stdbool.h>
#include <stdlib.h>
#include "DMA/DMA.h"
#include "hardware.h"
#include "MK64F12.h"
#include "gpio.h"
#include "cpu.h"

#define WS_NUM_LEDS      64
#define WS_BITS_PER_LED  24
#define WS_RESET_BITS    60
#define WS_TOTAL_BITS    (WS_NUM_LEDS * WS_BITS_PER_LED+ WS_RESET_BITS)

static uint16_t ws_total_transfers = 0;
static volatile bool ws_dma_done = false;
static volatile bool ws_busy = false;

static bool ws_build_cnv_buffer(const uint8_t *buf, uint32_t len);
static void DMA_cb(void);


//static uint16_t ws_cnv_buffer[WS_TOTAL_BITS];
typedef struct {
    uint16_t cnv[WS_TOTAL_BITS];
    uint32_t canary;
} ws_buf_t;

static ws_buf_t ws_buf = {
    .canary = 0xDEADBEEF
};

#define ws_cnv_buffer (ws_buf.cnv)



/**
 * @brief Initializes the WS2812 transport layer using FTM0 + DMA.
 *
 * Configures:
 * - FTM peripheral (via FTM_Init()) for WS2812 timing generation.
 * - PORTC clock gating.
 * - DMA + DMAMUX so that each FTM0 overflow triggers a DMA transfer that writes
 *   the next CnV value into FTM0 channel 0 compare register.
 * - DMA channel interrupt callback to detect end of major loop.
 *
 * @return true Always returns true in the current implementation.
 */
bool WS2_TransportInit(void)
{
    FTM_Init();

    /* Enable the clock for the PORT C*/
    SIM->SCGC5 |= SIM_SCGC5_PORTC_MASK;

    // DMA Periodic Trigger Mode: DMA channels 0-3
    DMA_Init();
    // Every FTM overflow, a DMA minor loop transfer is triggered.
    DMAMUX_ConfigChannel(DMA_CH0, true, false, kDmaRequestMux0FTM0Channel0);     // FTM TOF --> DMAMUX --> DMA --> copies next CnV to FTM CnV.

    DMA_SetSourceAddr(DMA_CH0, (uint32_t)(&ws_cnv_buffer[0]));   // dirección de la fuente de datos
    DMA_SetDestAddr(DMA_CH0, (uint32_t)&FTM0->CONTROLS[0].CnV);     // dirección de destino (FTM0 CnV)

    DMA_SetSourceAddrOffset(DMA_CH0, 2); // cuanto se mueve la dirección de origen después de cada minor loop
    DMA_SetDestAddrOffset(DMA_CH0, 0);   // cuanto se mueve la dirección de destino después de cada minor loop

    DMA_SetSourceTransfSize(DMA_CH0, DMA_TransSize_16Bit);
    DMA_SetDestTransfSize(DMA_CH0, DMA_TransSize_16Bit);

    DMA_SetMinorLoopTransCount(DMA_CH0, 2); // cada minor loop transfiere 2 bytes (16 bits)

    DMA_SetDestLastAddrOffset(DMA_CH0, 0);  // Destino no se mueve al final del major loop

    DMA_SetChannelInterrupt(DMA_CH0, true, DMA_cb);

    DMA0->TCD[DMA_CH0].CSR |= DMA_CSR_DREQ_MASK; // auto-disable request al terminar major loop

    return true;
}

/**
 * @brief Deinitializes the WS2812 transport layer.
 *
 * Stops the FTM0 clock, halting further DMA-triggered CnV updates.
 */
void WS2_TransportDeinit(void)
{
    FTM_StopClock(FTM0);
}

/**
 * @brief Builds the DMA CnV buffer from a GRB byte stream plus reset low time.
 *
 * Converts each bit of the input byte stream into a CnV value:
 * - Bit '1' -> CNV_1
 * - Bit '0' -> CNV_0
 *
 * Appends WS_RESET_BITS of CNV_0 at the end to generate the WS2812 reset/latch time.
 *
 * Side effects:
 * - Writes ws_cnv_buffer[]
 * - Updates ws_total_transfers with the number of generated entries
 *
 * @param buf Pointer to input byte buffer (pixel data).
 * @param len Length of input buffer in bytes.
 */
static bool ws_build_cnv_buffer(const uint8_t *buf, uint32_t len)
{
    if (!buf) return false;

    const uint32_t max_bytes = WS_NUM_LEDS * 3;   // 192
    if (len < max_bytes) return false;            // strict
    if (len > max_bytes) len = max_bytes;         // clamp (optional)

    uint32_t bit_idx = 0;

    for (uint32_t i = 0; i < 192; i++) {          // start at 0, not 3
        uint8_t byte = buf[i];
        for (int bit = 7; bit >= 0; --bit) {
            if (bit_idx >= (WS_TOTAL_BITS - WS_RESET_BITS)) return false;
            ws_cnv_buffer[bit_idx++] = ((byte >> bit) & 1u) ? CNV_1 : CNV_0;
        }
    }

    for (uint32_t i = 0; i < WS_RESET_BITS; i++) {
        if (bit_idx >= WS_TOTAL_BITS) return false;
        ws_cnv_buffer[bit_idx++] = CNV_0;
    }

    ws_total_transfers = bit_idx; // should be exactly WS_TOTAL_BITS
    return (ws_total_transfers == (WS_TOTAL_BITS));
}


/**
 * @brief Starts a WS2812 DMA transfer for the provided pixel buffer.
 *
 * Behavior:
 * - Rejects null/empty buffers.
 * - Applies a one-time "first call" hardcoded skip (returns false once).
 * - Builds the ws_cnv_buffer from the input data.
 * - Programs DMA major-loop counters to transfer ws_total_transfers 16-bit values.
 * - Arms DMA requests and starts the FTM0 clock so overflows trigger transfers.
 *
 * Note: Completion is signaled via DMA_cb(), which stops FTM0 and sets ws_dma_done.
 *
 * @param buf Pointer to input byte buffer (pixel data).
 * @param len Length of input buffer in bytes.
 * @return true if the transfer was started; false if input invalid or first-call skip triggered.
 */
bool WS2_TransportSend(uint8_t *buf, uint32_t len)
{
	static bool first_call_hardcoded_fix = true;
    if (!buf || len == 0) return false;
    if(first_call_hardcoded_fix){
    	first_call_hardcoded_fix = false;
    	return false;
    }

    if (!ws_build_cnv_buffer(buf, len)) return false;

    // NBYTES cuantos bytes se transfieren en cada minor loop
    // CITER/BITER cuanto minor loops por major loop
    // SLAST y DLAST_SGA se usan para setear la dirección a la que vuelven una vez terminado el major loop
    // CSR flogs de control (int, etc)

    DMA_SetStartMajorLoopCount(DMA_CH0, ws_total_transfers);	// BITER
    DMA_SetCurrMajorLoopCount(DMA_CH0, ws_total_transfers); // CITER

    DMA_SetSourceAddr(DMA_CH0, (uint32_t)(&ws_cnv_buffer[0]));   // dirección de la fuente de datos
    DMA_SetSourceLastAddrOffset(DMA_CH0, -((int32_t)ws_total_transfers * 2)); // volver al inicio del buffer de CnV

    // Limpiar flags DONE / interrupt del canal
    DMA_ClearChannelDoneFlag(DMA_CH0);
    DMA_ClearChannelIntFlag(DMA_CH0);

    ws_dma_done = false;

    DMA_SetEnableRequest(DMA_CH0, true); // habilita la solicitud DMA TIENE QUE IR ANTES DE QUE SE EMIECE EL CLK DE FTM SI O SI

    FTM_ClearOverflowFlag(FTM0);
    FTM0->CNT = 0;
    FTM_StartClock(FTM0);

    return true;
}

/**
 * @brief Indicates whether a WS2812 DMA transfer is still running.
 *
 * @return true if a transfer is in progress; false if the last transfer completed.
 */
bool WS2_TransferInProgress(void)
{
    return !ws_dma_done;
}

/**
 * @brief DMA completion callback for the WS2812 transport transfer.
 *
 * Stops the FTM0 clock and marks the transfer as complete by setting ws_dma_done.
 * This callback is registered as the DMA channel interrupt handler.
 */
void DMA_cb(void)
{
    FTM_StopClock(FTM0);
    FTM0->CNT = 0;
    FTM0->CONTROLS[0].CnV = 0;           // fuerza LOW al inicio
    FTM_ClearOverflowFlag(FTM0);

    ws_dma_done = true;
}

