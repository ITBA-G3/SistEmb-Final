/**
 * @file     LEDmatrix.c
 * @brief    LED matrix driver implementation.
 *
 * This file contains the implementation of the LED matrix driver:
 * - Logical matrix abstraction over a WS2812 LED strip
 * - Pixel buffer management and coordinate mapping
 * - Brightness scaling
 * - Frame preparation and transmission via WS2812 transport
 *
 * @author   Grupo 3
 *           - Ezequiel Díaz Guzmán
 *           - José Iván Hertter
 *           - Cristian Damián Meichtry
 *           - Lucía Inés Ruiz
 */


#include "LEDmatrix.h"
#include "drivers/ws2812_ftm.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

uint16_t LEDM_GetPhysicalIndex(uint8_t x, uint8_t y);

static LEDM_t g_matrix;
static LEDM_color_t g_pixels[LEDM_MAX_PIXELS];

/**
 * @brief Initializes the LED matrix driver and underlying WS2812 transport.
 *
 * Validates dimensions, sets up the global matrix instance, clears pixel memory,
 * and initializes the WS2812 transport layer.
 *
 * @param width Matrix width in pixels.
 * @param height Matrix height in pixels.
 * @return Pointer to the initialized LED matrix instance, or NULL on error.
 */
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

/**
 * @brief Deinitializes the LED matrix driver and underlying WS2812 transport.
 *
 * @param matrix Pointer to the LED matrix instance.
 */
void LEDM_Deinit(LEDM_t* matrix)
{
    if (!matrix) return;
    WS2_TransportDeinit();
}


/**
 * @brief Sets a pixel color at logical coordinates (x,y).
 *
 * Converts logical coordinates to the physical LED index and updates the pixel buffer.
 *
 * @param matrix Pointer to the LED matrix instance.
 * @param y Y coordinate (row).
 * @param x X coordinate (column).
 * @param color Color to assign to the pixel.
 * @return true if the pixel was set successfully, false on invalid parameters/out-of-range coordinates.
 */
bool LEDM_SetPixel(LEDM_t* matrix, uint8_t y, uint8_t x, LEDM_color_t color){

    if (!matrix) return false;

    if (x >= matrix->width || y >= matrix->height) return false;

    uint16_t led_index = LEDM_GetPhysicalIndex(x,y);

    matrix->pixels[led_index] = color;

    return true;
}

/**
 * @brief Returns the physical LED index for the given logical coordinates.
 *
 * Current mapping assumes a fixed row-major 8-column layout.
 *
 * @param x X coordinate (column).
 * @param y Y coordinate (row).
 * @return Physical LED index.
 */
uint16_t LEDM_GetPhysicalIndex(uint8_t x, uint8_t y)
{
    return (y) * 8 + (x);
}

/**
 * @brief Reports whether a WS2812 transfer is currently in progress.
 *
 * This is a thin wrapper around the WS2812 transport status.
 *
 * @return true if a transfer is ongoing, false otherwise.
 */
bool LEDM_TransferInProgress()
{
	return (WS2_TransferInProgress());
}


/**
 * @brief Sends the current pixel buffer to the LED matrix (non-blocking).
 *
 * Builds a GRB byte stream (with brightness scaling) and starts a WS2812 transport transfer.
 *
 * @param matrix Pointer to the LED matrix instance.
 * @return true if the transfer was started successfully, false otherwise.
 */
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


/**
 * @brief Sets global brightness scaling for the LED matrix.
 *
 * @param dev Pointer to the LED matrix instance.
 * @param brightness Brightness value (0-255).
 */
 void LEDM_SetBrightness(LEDM_t* dev, uint8_t brightness)
 {
     if (!dev) return;
     dev->brightness = brightness;
}


 /**
  * @brief Clears the matrix pixel buffer (sets all pixels to off/black).
  *
  * @param matrix Pointer to the LED matrix instance.
  */
 void LEDM_Clear(LEDM_t* matrix){
	 for(int i=0; i<matrix->num_pixels; i++){
		 matrix->pixels[i].g = 0;
		 matrix->pixels[i].r = 0;
		 matrix->pixels[i].b = 0;
	 }
 }


