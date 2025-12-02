#include "LEDmatrix.h"
#include "drivers/ws2812_ftm.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

void apply_brightness_and_prepare(const LEDM_t* dev, uint8_t *out_buffer);
uint16_t LEDM_GetPhysicalIndex(uint8_t x, uint8_t y);

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

bool LEDM_SetPixel(LEDM_t* matrix, uint8_t x, uint8_t y, LEDM_color_t color){

    if (!matrix) return false;

    if (x >= matrix->width || y >= matrix->height) return false;


    uint16_t led_index = LEDM_GetPhysicalIndex(x,y);

    matrix->pixels[led_index] = color;

    return true;
}


//uint16_t LEDM_GetPhysicalIndex(uint8_t x, uint8_t y) {
//	return (y-1) * 8 + (x-1);
//}

uint16_t LEDM_GetPhysicalIndex(uint8_t x, uint8_t y)
{
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
    if (matrix->num_pixels > 64) return false;

    static uint8_t tx[64 * 3];

    for (uint32_t i = 0; i < matrix->num_pixels; i++) {
        LEDM_color_t c = matrix->pixels[i];
        tx[i*3 + 0] = (uint8_t)(((uint16_t)c.g * matrix->brightness) / 255);
        tx[i*3 + 1] = (uint8_t)(((uint16_t)c.r * matrix->brightness) / 255);
        tx[i*3 + 2] = (uint8_t)(((uint16_t)c.b * matrix->brightness) / 255);
    }

    matrix->transfer_in_progress = true;
    bool status = WS2_TransportSend(tx, matrix->num_pixels * 3);
    matrix->transfer_in_progress = false;

    return status;
}

 void LEDM_SetBrightness(LEDM_t* dev, uint8_t brightness)
 {
     if (!dev) return;
     dev->brightness = brightness;
}

 void LEDM_Clear(LEDM_t* matrix){
	 for(int i=0; i<matrix->num_pixels; i++){
		 matrix->pixels = 0;
	 }
 }


