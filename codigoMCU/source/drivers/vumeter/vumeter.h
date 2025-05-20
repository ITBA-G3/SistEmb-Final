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

/*******************************************************************************
 * ENUMERATIONS AND STRUCTURES AND TYPEDEFS
 ******************************************************************************/
typedef enum
{
    VUMETER_FREQUENCY_20HZ = 20,
    VUMETER_FREQUENCY_50HZ = 50,
    VUMETER_FREQUENCY_100HZ,
    VUMETER_FREQUENCY_200HZ,
    VUMETER_FREQUENCY_400HZ,
    VUMETER_FREQUENCY_800HZ,
    VUMETER_FREQUENCY_1600HZ,
    VUMETER_FREQUENCY_3200HZ
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
void vumeter_set_frequency(vumeter_frequency_t frequency, uint8_t value);

/**
 * @brief Set the vumeter value for all bands
 * 
 * @param value The value to set for the frequency (between VUMETER_MIN and VUMETER_MAX)
 */
void vumeter_set_all_band(uint8_t value);

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
