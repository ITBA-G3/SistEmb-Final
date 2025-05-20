/***************************************************************************/ /**
   @file     vumeter.h
   @brief    vumeter driver functions
   @author   Grupo 3
  ******************************************************************************/

#ifndef VUMETER_H
#define VUMETER_H

/*******************************************************************************
 * INCLUDE HEADER FILES
 ******************************************************************************/
#include <stdint.h>
#include <stdbool.h>

/*******************************************************************************
 * CONSTANT AND MACRO DEFINITIONS USING #DEFINE
 ******************************************************************************/

#define VUMETER_MIN 0
#define VUMETER_MAX 100
#define VUMETER_FREQUENCIES 10

#define MATRIX_HEIGHT 8
#define MATRIX_WIDTH 8
#define MATRIX_PIXELS (MATRIX_WIDTH * MATRIX_HEIGHT)
/*******************************************************************************
 * ENUMERATIONS AND STRUCTURES AND TYPEDEFS
 ******************************************************************************/
typedef enum
{
  VUMETER_FREQUENCY_00 = 20,
  VUMETER_FREQUENCY_01 = 50,
  VUMETER_FREQUENCY_02 = 100,
  VUMETER_FREQUENCY_03 = 200,
  VUMETER_FREQUENCY_04 = 400,
  VUMETER_FREQUENCY_05 = 800,
  VUMETER_FREQUENCY_06 = 1600,
  VUMETER_FREQUENCY_07 = 3200,
  VUMETER_FREQUENCY_08 = 6400,
  VUMETER_FREQUENCY_09 = 12800,
} vumeter_frequency_t;

/*******************************************************************************
 * FUNCTION PROTOTYPES WITH GLOBAL SCOPE
 ******************************************************************************/

/**
 * @brief Initialize the vumeter
 *
 * @return true if initialization was successful, false otherwise
 */
bool vumeter_init(void);

/**
 * @brief Set the vumeter frequency
 *
 * @param frequency The frequency to set
 * @param value The value to set for the frequency (between VUMETER_MIN and VUMETER_MAX)
 */
void vumeter_set_frequency(vumeter_frequency_t frequency, uint16_t value);

/**
 * @brief Set the vumeter value for all bands
 *
 * @param value The value to set for the frequency (between VUMETER_MIN and VUMETER_MAX)
 */
void vumeter_set_all_band(uint16_t value);

/**
 * @brief clear the vumeter display, turn off all LEDs
 *
 */
void vumeter_clear(void);

/**
 * @brief Set the brightness of the vumeter
 *
 * @param brightness level of brightness (0-100)
 */
void vumeter_set_brightness(uint8_t brightness);
