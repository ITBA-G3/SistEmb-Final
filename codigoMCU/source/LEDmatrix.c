#include "LEDmatrix.h"
#include "drivers/ws2812_ftm.h"
#include <stdlib.h>
#include <string.h>
#include "fsl_debug_console.h"
#include <stdio.h>

void apply_brightness_and_prepare(const LEDM_t* dev, uint8_t *out_buffer);

LEDM_t* LEDM_Init(uint16_t width, uint16_t height){

    LEDM_t* matrix;

    if (width == 0 || height == 0) return NULL;

    matrix = (LEDM_t *)malloc(sizeof *matrix);
    if (!matrix) return NULL;

    matrix->width = width;
    matrix->height = height;
    matrix->num_pixels = (uint32_t)width * height;

    
    matrix->pixels = (LEDM_color_t *)calloc(matrix->num_pixels, sizeof(LEDM_color_t));      // calloc para inicializar en cero
    if (!matrix->pixels) {
        free(matrix);
        return NULL;
    }

    matrix->brightness = 64; // No me quiero quedar ciega.
    matrix->transfer_in_progress = false;

    /* Initialize transport FTM.
       If transport init fails, free resources and return NULL. */
    if (!WS2_TransportInit()) {
        free(matrix->pixels);
        free(matrix);
        return NULL;
    }

    return matrix;
}

void LEDM_Deinit(LEDM_t* matrix)
{
    if (!matrix) return;
    WS2_TransportDeinit();
    free(matrix->pixels);
    free(matrix);
}

bool LEDM_SetPixel(LEDM_t* matrix, uint16_t x, uint16_t y, LEDM_color_t color){

    if (!matrix) return false;

    if (x >= matrix->width || y >= matrix->height) return false;


    uint16_t led_index = LEDM_GetPhysicalIndex(x,y);
//    uint16_t led_index = index;

    matrix->pixels[led_index] = color;

    return true;
}


uint16_t LEDM_GetPhysicalIndex(uint8_t x, uint8_t y) {
	return (y-1) * 8 + (x-1);
}

bool LEDM_TransferInProgress(LEDM_t* matrix)
{
    if (!matrix) return false;
    return matrix->transfer_in_progress;
}

bool LEDM_Show(LEDM_t* matrix)
{
    if (!matrix) return false;

    if (matrix->transfer_in_progress) return false;

    
    uint8_t *tx = (uint8_t*)malloc(matrix->num_pixels * 3);

    if (!tx) return false;

    // Create bitstream

    for (uint32_t i = 0; i < matrix->num_pixels; i++) {
		LEDM_color_t c = matrix->pixels[i];
		tx[i*3 + 0] = (uint8_t)(((uint16_t)c.g * matrix->brightness) / 255);
		tx[i*3 + 1] = (uint8_t)(((uint16_t)c.r * matrix->brightness) / 255);
		tx[i*3 + 2] = (uint8_t)(((uint16_t)c.b * matrix->brightness) / 255);
    }

    matrix->transfer_in_progress = true;

    bool status = WS2_TransportSend(tx, matrix->num_pixels * 3);
    if (!status) {
        matrix->transfer_in_progress = false;
        free(tx);
        return false;
    }

    matrix->transfer_in_progress = false;
    free(tx);
    
    return true;
}

 bool LEDM_GetTransferProgress(LEDM_t* dev)
 {
        /* Use a static TX buffer to avoid heap corruption and make debugging
           deterministic. Size assumed small (e.g., 64 leds -> 192 bytes). If you
           need larger, increase MAX_LEDS. */
    #define MAX_LEDS 256
        static uint8_t tx_static[MAX_LEDS * 3];
        if (matrix->num_pixels > MAX_LEDS) return false;

        /* Create bitstream into static buffer */
        for (uint32_t i = 0; i < matrix->num_pixels; i++) {
            LEDM_color_t c = matrix->pixels[i];
    #if LEDM_TX_ORDER == LEDM_TX_ORDER_GRB
            tx_static[i*3 + 0] = (uint8_t)(((uint16_t)c.g * matrix->brightness) / 255);
            tx_static[i*3 + 1] = (uint8_t)(((uint16_t)c.r * matrix->brightness) / 255);
            tx_static[i*3 + 2] = (uint8_t)(((uint16_t)c.b * matrix->brightness) / 255);
    #else /* LEDM_TX_ORDER_RGB */
            tx_static[i*3 + 0] = (uint8_t)(((uint16_t)c.r * matrix->brightness) / 255);
            tx_static[i*3 + 1] = (uint8_t)(((uint16_t)c.g * matrix->brightness) / 255);
            tx_static[i*3 + 2] = (uint8_t)(((uint16_t)c.b * matrix->brightness) / 255);
    #endif
        }

        /* Debug: print the first bytes prepared */
        PRINTF("LEDM_Show: prepared tx addr=%p len=%lu first: ", tx_static, (unsigned long)(matrix->num_pixels * 3));
        for (uint32_t k = 0; k < 12 && k < matrix->num_pixels * 3; ++k) {
            PRINTF("%02X ", tx_static[k]);
        }
        PRINTF("\r\n");
 }

 void LEDM_SetBrightness(LEDM_t* dev, uint8_t brightness)
 {
     if (!dev) return;
     dev->brightness = brightness;
 }


