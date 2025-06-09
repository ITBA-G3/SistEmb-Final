/***************************************************************************/ /**
   @file     matrix.h
   @brief    matrix driver functions
   @author   Grupo 3
  ******************************************************************************/
#ifndef MATRIX_H
#define MATRIX_H

/*******************************************************************************
 * INCLUDE HEADER FILES
 ******************************************************************************/
#include <stdint.h>
#include <stdbool.h>
/*******************************************************************************
 * CONSTANT AND MACRO DEFINITIONS USING #DEFINE
 ******************************************************************************/
#define MATRIX_HEIGHT 8
#define MATRIX_WIDTH 8
#define MATRIX_PIXELS (MATRIX_WIDTH * MATRIX_HEIGHT)
#define DUT_ONE  40
#define DUT_ZERO 20
#define BITS_COLOR 8
#define BITS_TOTAL BITS_COLOR*MATRIX_PIXELS*3

/*******************************************************************************
 * ENUMERATIONS AND STRUCTURES AND TYPEDEFS
 ******************************************************************************/

typedef struct {
	uint16_t r[BITS_COLOR];
	uint16_t g[BITS_COLOR];
	uint16_t b[BITS_COLOR];
} pixel_t;

/*******************************************************************************
 * FUNCTION PROTOTYPES WITH GLOBAL SCOPE
 ******************************************************************************/
/**
 * @brief Initialize the matrix
 * @return true if initialization was successful, false otherwise
 */
bool matrix_init(void);

/**
 * @brief Converts an RGB value into a sequence of duty cycle bits for a specific LED.
 * @param pixel The position of the pixel to set
 * @param r The red component of the pixel color (0-255)
 * @param g The green component of the pixel color (0-255)
 * @param b The blue component of the pixel color (0-255)
 */
void matrix_set_pixel(uint16_t pixel, uint8_t r, uint8_t g, uint8_t b);

/**
 * @brief Clear the matrix. Sets all pixels to off (0, 0, 0).
 */
void matrix_clear(void);

/**
 * @brief Fill the matrix with a specific color.
 * @param r The red component of the color (0-255)
 * @param g The green component of the color (0-255)
 * @param b The blue component of the color (0-255)
 */
void matrix_fill(uint8_t r, uint8_t g, uint8_t b);

#endif // MATRIX_H