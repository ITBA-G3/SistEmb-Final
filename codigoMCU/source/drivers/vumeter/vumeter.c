/***************************************************************************/ /**
   @file     vumeter.c
   @brief    vumeter driver functions
   @author   Grupo 3
  ******************************************************************************/

/*******************************************************************************
 * INCLUDE HEADER FILES
 ******************************************************************************/
#include "vumeter.h"
#include <stdbool.h>
#include "gpio.h"
#include "pisr.h"
#include "board.h"
#include "../SDK/CMSIS/MK64F12.h"

/*******************************************************************************
 * FUNCTION PROTOTYPES FOR PRIVATE FUNCTIONS WITH FILE LEVEL SCOPE
 ******************************************************************************/
/**
 * @brief Convert a frequency to a row number
 *
 * @param value
 * @return The row number corresponding to the frequency
 */
static uint8_t setRow(float value);

/**
 * @brief Convert a value (level) to a column number
 *
 * @param frequency The frequency to convert
 * @return The column number corresponding to the value
 */
static uint8_t setColumn(uint16_t frequency);

/*******************************************************************************
 * STATIC VARIABLES AND CONST VARIABLES WITH FILE LEVEL SCOPE
 ******************************************************************************/
uint16_t vumeter_frequencies[VUMETER_FREQUENCIES] = {
    VUMETER_FREQUENCY_00,
    VUMETER_FREQUENCY_01,
    VUMETER_FREQUENCY_02,
    VUMETER_FREQUENCY_03,
    VUMETER_FREQUENCY_04,
    VUMETER_FREQUENCY_05,
    VUMETER_FREQUENCY_06,
    VUMETER_FREQUENCY_07,
    VUMETER_FREQUENCY_08,
    VUMETER_FREQUENCY_09};

/*******************************************************************************
 * INITIALIZATION FUNCTION
 ******************************************************************************/
bool vumeter_init(void)
{
    static bool initialized = false;
    if (!initialized)
    {
        // tengo que inicializar la matriz, la concha del pato
        // matrix_init();
        // matrix_full_screen();

        initialized = true;
    }
    return initialized;
}

/*******************************************************************************
 * FUNCTION DEFINITIONS
 ******************************************************************************/

void vumeter_set_frequency(vumeter_frequency_t frequency, uint8_t value)
{
}

void vumeter_set_all_band(uint16_t value)
{
}

void vumeter_clear(void)
{
}

void vumeter_set_brightness(uint8_t brightness)
{
}

static uint8_t setRow(float value)
{
    value = value * ((float)MATRIX_HEIGHT / VUMETER_MAX) - 1; // scale to 0-7
    value = (uint8_t)(value + 0.5f); // round to nearest integer

    if (value >= MATRIX_HEIGHT)
        value = MATRIX_HEIGHT - 1;

    return value;
}

static uint8_t setColumn(uint16_t frequency)
{
    uint8_t low = 0;
    uint8_t high = VUMETER_FREQUENCIES - 1;
    uint8_t mid = (low + high) / 2;

    // If frequency is less than or equal to the lowest band, return the first column
    if (frequency <= VUMETER_FREQUENCY_00)
    {
        return = 0;
    }
    // If frequency is greater than or equal to the highest band, return the last column
    else if (frequency >= VUMETER_FREQUENCY_09)
    {
        return = VUMETER_FREQUENCIES - 1;
    }
    
    // If frequency is greater than or equal to the highest band, return the last column
    while (low <= high)
    {
        mid = (low + high) / 2;
        if (vumeter_frequencies[mid] == frequency)
        {
            return mid;
        }
        else if (vumeter_frequencies[mid] < frequency)
        {
            low = mid + 1;
        }
        else
        {
            high = mid - 1;
        }
    }

    if(low >= VUMETER_FREQUENCIES)
        return VUMETER_FREQUENCIES - 1;

    if(low == 0)
        return 0;

    if ((vumeter_frequencies[low] - frequency) < (frequency - vumeter_frequencies[low - 1]))
        return low;
    else
        return low - 1;
}