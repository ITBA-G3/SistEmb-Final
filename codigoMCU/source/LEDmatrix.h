/**
 * @file     LEDmatrix.h
 * @brief    LED matrix driver interface.
 *
 * This file defines the data structures and public API for the LED matrix:
 * - Matrix and color data types
 * - Pixel manipulation functions
 * - Brightness control
 * - Frame transmission control
 *
 * @author   Grupo 3
 *           - Ezequiel Díaz Guzmán
 *           - José Iván Hertter
 *           - Cristian Damián Meichtry
 *           - Lucía Inés Ruiz
 */

#ifndef LEDMATRIX_H_
#define LEDMATRIX_H_

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

#define LEDM_MAX_WIDTH   8
#define LEDM_MAX_HEIGHT  8
#define LEDM_MAX_PIXELS  (LEDM_MAX_WIDTH * LEDM_MAX_HEIGHT)

typedef struct {
	uint8_t g;
	uint8_t r;
	uint8_t b;
} LEDM_color_t;

typedef struct {
	LEDM_color_t *pixels;		// Puntero a estructura que contiene los 64 * 24 bits a enviar
	uint16_t width;	
	uint16_t height;
	uint32_t num_pixels;
	uint8_t brightness;
} LEDM_t;


/**
 * @brief Initializes the LED matrix driver and WS2812 transport.
 *
 * Validates the requested dimensions, clears the internal pixel buffer,
 * and initializes the underlying WS2812 transport layer.
 *
 * @param width Matrix width in pixels.
 * @param height Matrix height in pixels.
 * @return Pointer to the initialized LED matrix instance, or NULL on error.
 */
LEDM_t* LEDM_Init(uint16_t width, uint16_t height);

/**
 * @brief Deinitializes the LED matrix driver.
 *
 * Stops and deinitializes the underlying WS2812 transport.
 *
 * @param matrix Pointer to the LED matrix instance.
 */
void LEDM_Deinit(LEDM_t* matrix);

/**
 * @brief Sets a pixel color at logical coordinates (x,y).
 *
 * Writes the given color into the internal pixel buffer, using the matrix's
 * logical-to-physical mapping.
 *
 * @param dev Pointer to the LED matrix instance.
 * @param y Y coordinate (row).
 * @param x X coordinate (column).
 * @param color Color to assign to the pixel.
 * @return true if the pixel was set successfully, false on invalid parameters or out-of-range coordinates.
 */
bool LEDM_SetPixel(LEDM_t* dev, uint8_t y, uint8_t x, LEDM_color_t color);

/**
 * @brief Sets global brightness scaling for subsequent renders.
 *
 * Brightness is applied when preparing the transmit buffer for WS2812 output.
 *
 * @param dev Pointer to the LED matrix instance.
 * @param brightness Brightness value (0-255).
 */
void LEDM_SetBrightness(LEDM_t* dev, uint8_t brightness);

/**
 * @brief Sends the current pixel buffer to the LED matrix (non-blocking).
 *
 * Converts internal pixel data into the GRB byte stream expected by WS2812,
 * applies brightness scaling, and starts the transport transfer.
 *
 * @param dev Pointer to the LED matrix instance.
 * @return true if the transfer was started successfully, false otherwise.
 */
bool LEDM_Show(LEDM_t* dev);

/**
 * @brief Reports whether an LED matrix transfer is currently in progress.
 *
 * This reflects the status of the underlying WS2812 transport transfer.
 *
 * @return true if a transfer is ongoing, false otherwise.
 */
bool LEDM_TransferInProgress(void);

/**
 * @brief Clears the matrix pixel buffer (sets all pixels to off/black).
 *
 * @param matrix Pointer to the LED matrix instance.
 */
void LEDM_Clear(LEDM_t* matrix);

#endif
