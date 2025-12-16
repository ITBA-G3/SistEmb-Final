#include "LEDmatrix.h"
#include "drivers/ws2812_ftm.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

void apply_brightness_and_prepare(const LEDM_t* dev, uint8_t *out_buffer);
uint16_t LEDM_GetPhysicalIndex(uint8_t x, uint8_t y);

static LEDM_t g_matrix;
static LEDM_color_t g_pixels[LEDM_MAX_PIXELS];


LEDM_t* LEDM_Init(uint16_t width, uint16_t height){

    if (width == 0 || height == 0) return NULL;
    if (width > LEDM_MAX_WIDTH || height > LEDM_MAX_HEIGHT) return NULL;

    LEDM_t *matrix = &g_matrix;

    matrix->width = width;
    matrix->height = height;
    matrix->num_pixels = (uint32_t)width * height;

    matrix->pixels = g_pixels;
    memset(matrix->pixels, 0, sizeof(g_pixels));

    if (!WS2_TransportInit()) {
            return NULL;
    }

    return matrix;
}

void LEDM_Deinit(LEDM_t* matrix)
{
    if (!matrix) return;
    WS2_TransportDeinit();
}

bool LEDM_SetPixel(LEDM_t* matrix, uint8_t y, uint8_t x, LEDM_color_t color){

    if (!matrix) return false;

    if (x >= matrix->width || y >= matrix->height) return false;

    uint16_t led_index = LEDM_GetPhysicalIndex(x,y);

    matrix->pixels[led_index] = color;

    return true;
}


uint16_t LEDM_GetPhysicalIndex(uint8_t x, uint8_t y)
{
    return (y) * 8 + (x);
}


bool LEDM_TransferInProgress()
{
	return (WS2_TransferInProgress());
}

bool LEDM_Show(LEDM_t* matrix)
{
    if (!matrix) return false;
//    if (LEDM_TransferInProgress()) return false;
    if (matrix->num_pixels > 64) return false;

    uint8_t tx[64 * 3];

    for (uint32_t i = 0; i < matrix->num_pixels; i++) {
        LEDM_color_t c = matrix->pixels[i];
        tx[i*3 + 0] = (uint8_t)(((uint16_t)c.g * matrix->brightness) / 255);
        tx[i*3 + 1] = (uint8_t)(((uint16_t)c.r * matrix->brightness) / 255);
        tx[i*3 + 2] = (uint8_t)(((uint16_t)c.b * matrix->brightness) / 255);
    }

    bool status = WS2_TransportSend(tx, matrix->num_pixels * 3);

    return status;
}

 void LEDM_SetBrightness(LEDM_t* dev, uint8_t brightness)
 {
     if (!dev) return;
     dev->brightness = brightness;
}

 void LEDM_Clear(LEDM_t* matrix){
	 for(int i=0; i<matrix->num_pixels; i++){
		 matrix->pixels[i].g = 0;
		 matrix->pixels[i].r = 0;
		 matrix->pixels[i].b = 0;
	 }
 }


