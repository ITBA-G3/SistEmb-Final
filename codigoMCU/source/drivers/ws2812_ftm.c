#include "ws2812_ftm.h"
#include "FTM.h"
#include <stdint.h>
#include <stdbool.h>
#include <stdlib.h>
#include "DMA/DMA.h"
#include "hardware.h"
#include "MK64F12.h"
#include "gpio.h"

#define WS_NUM_LEDS      64
#define WS_BITS_PER_LED  24
#define WS_RESET_BITS    40
#define WS_TOTAL_BITS    (WS_NUM_LEDS * WS_BITS_PER_LED)

static uint16_t ws_cnv_buffer[WS_TOTAL_BITS];
static uint16_t ws_total_transfers = 0;
static volatile bool ws_dma_done = false;

static void ws_build_cnv_buffer(uint8_t *buf, uint32_t len);
static void DMA_cb(void);


bool WS2_TransportInit(void)
{
    FTM_Init();

    /* Enable the clock for the PORT C*/
    SIM->SCGC5 |= SIM_SCGC5_PORTC_MASK;

    // DMA Periodic Trigger Mode: DMA channels 0-3
    DMA_Init();
    // Every FTM overflow, a DMA minor loop transfer is triggered.
    DMAMUX_ConfigChannel(DMA_CH0, true, false, kDmaRequestMux0FTM0Channel0);     // FTM TOF --> DMAMUX --> DMA --> copies next CnV to FTM CnV.

    DMA_SetSourceAddr(DMA_CH0, (uint32_t)(ws_cnv_buffer));   // dirección de la fuente de datos
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

void WS2_TransportDeinit(void)
{
    FTM_StopClock(FTM0);
}

static void ws_build_cnv_buffer(uint8_t *buf, uint32_t len)
{
    uint32_t bit_idx = 0;

    for (uint32_t i = 0; i < len; i++) {
        uint8_t byte = buf[i];
        for (int bit = 7; bit >= 0; bit--) {
            bool one = (byte >> bit) & 0x01;
            ws_cnv_buffer[bit_idx++] = one ? CNV_1 : CNV_0;
        }
    }

    ws_total_transfers = bit_idx; // esto debería ser WS_TOTAL_BITS = 1576
}


bool WS2_TransportSend(uint8_t *buf, uint32_t len)
{
    if (!buf || len == 0) return false;

    ws_build_cnv_buffer(buf, len);

    // NBYTES cuantos bytes se transfieren en cada minor loop
    // CITER/BITER cuanto minor loops por major loop
    // SLAST y DLAST_SGA se usan para setear la dirección a la que vuelven una vez terminado el major loop
    // CSR flogs de control (int, etc)

    DMA_SetStartMajorLoopCount(DMA_CH0, ws_total_transfers);
//    DMA_SetCurrMajorLoopCount(DMA_CH0, ws_total_transfers); // cantidad de minor loops por major loop

    DMA_SetSourceLastAddrOffset(DMA_CH0, -((int32_t)ws_total_transfers * 2)); // volver al inicio del buffer de CnV

    // Limpiar flags DONE / interrupt del canal
    DMA_ClearChannelDoneFlag(DMA_CH0);
    DMA_ClearChannelIntFlag(DMA_CH0);       // just in case

    ws_dma_done = false;

    FTM_ClearOverflowFlag(FTM0);
    FTM0->CNT = 0;
    FTM_StartClock(FTM0);

    DMA_SetEnableRequest(DMA_CH0, true); // habilita la solicitud DMA

//    DMA0->SSRT = DMA_SSRT_SSRT(0);


//    FTM_SetCnV(FTM0, 0, 0); // apaga la señal después de la transferencia
    while(!ws_dma_done)
    {

    }

    return true;
}

bool WS2_TransferInProgress(void)
{
    return !ws_dma_done;
}


void DMA_cb(void)
{
    FTM_StopClock(FTM0);
    // Indica que la transferencia DMA ha finalizado
    ws_dma_done = true;
}

