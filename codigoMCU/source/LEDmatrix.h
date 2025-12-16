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

LEDM_t* LEDM_Init(uint16_t width, uint16_t height);

void LEDM_Deinit(LEDM_t* matrix);

bool LEDM_SetPixel(LEDM_t* dev, uint8_t y, uint8_t x, LEDM_color_t color);



void LEDM_SetBrightness(LEDM_t* dev, uint8_t brightness);

bool LEDM_Show(LEDM_t* dev);

bool LEDM_TransferInProgress();

void LEDM_Clear(LEDM_t* matrix);

#endif
