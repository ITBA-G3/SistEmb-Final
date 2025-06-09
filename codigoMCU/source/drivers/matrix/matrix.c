/***************************************************************************/ /**
   @file     matrix.c
   @brief    matrix driver functions
   @author   Grupo 3
  ******************************************************************************/

/*******************************************************************************
 * INCLUDE HEADER FILES
 ******************************************************************************/
#include "matrix.h"
#include "gpio.h"
#include "pisr.h"
#include "board.h"
#include "DMA.h"
#include "../SDK/CMSIS/MK64F12.h"

/*******************************************************************************
 * FUNCTION PROTOTYPES FOR PRIVATE FUNCTIONS WITH FILE LEVEL SCOPE
 ******************************************************************************/

/*******************************************************************************
 * STATIC VARIABLES AND CONST VARIABLES WITH FILE LEVEL SCOPE
 ******************************************************************************/
static pixel_t matrix[MATRIX_PIXELS];

/*******************************************************************************
 * STATIC VARIABLES AND CONST VARIABLES WITH FILE LEVEL SCOPE
 * ******************************************************************************/

bool matrix_init(void)
{
    static bool matrix_initialized = false;

    if (!matrix_initialized)
    {
        // iniciar GPIO ??
        // iniciar DMA
        DMA_Init();
        matrix_initialized = true;

        /*// REVISAR
        DMAChannel_t dma_channel = DMA_CHANNEL_0;
        DMAMUX_ConfigChannel(dma_channel, true, false, DMA_REQUEST_SOURCE_MATRIX);
        DMA_SetSourceModulo(dma_channel, 0);
        DMA_SetDestModulo(dma_channel, 0);
        DMA_SetDestTransfSize(dma_channel, sizeof(uint8_t));
        DMA_SetCurrMajorLoopCount(dma_channel, MATRIX_HEIGHT * MATRIX_WIDTH);
        DMA_SetStartMajorLoopCount(dma_channel, MATRIX_HEIGHT * MATRIX_WIDTH);
        DMA_SetMinorLoopTransCount(dma_channel, 1);
        DMA_SetDestAddrOffset(dma_channel, 0);   // Set destination address offset
        DMA_SetSourceAddrOffset(dma_channel, 0); // Set source address offset
        DMA_SetSourceAddr(dma_channel, (uint32_t)matrix_get_buffer());
        DMA_SetDestAddr(dma_channel, (uint32_t)matrix_get_buffer());
        DMA_SetEnableRequest(dma_channel, true);
        */
        matrix_clear(); // Clear the matrix to start with a clean state
    }
    return matrix_initialized;
}

/*******************************************************************************
 * FUNCTION DEFINITIONS
 ******************************************************************************/
void matrix_clear(void)
{
    for (uint16_t i = 0; i < MATRIX_PIXELS; i++)
    {
        matrix_set_pixel(i, 0, 0, 0);
    }
}

void matrix_fill(uint8_t r, uint8_t g, uint8_t b)
{
    for (uint16_t i = 0; i < MATRIX_PIXELS; i++)
    {
        matrix_set_pixel(i, r, g, b);
    }
}

// Convert a RGB value to a sequence of duty cycles bits for the specific LED.
void matrix_set_pixel(uint16_t pixel, uint8_t r, uint8_t g, uint8_t b)
{
    for (int i = 0; i < BITS_COLOR; i++)
    {
        matrix[pixel].green[i] = (((g) >> i) & (1 << (7 - i))) > 0 ? DUT_ONE : DUT_ZERO;
        matrix[pixel].red[i] = (((r) >> i) & (1 << (7 - i))) > 0 ? DUT_ONE : DUT_ZERO;
        matrix[pixel].blue[i] = (((b) >> i) & (1 << (7 - i))) > 0 ? DUT_ONE : DUT_ZERO;
    }
}
