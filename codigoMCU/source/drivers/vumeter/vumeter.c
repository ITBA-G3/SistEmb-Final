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
 * INITIALIZATION FUNCTION
 ******************************************************************************/
bool vumeter_init(void)
{
    static bool initialized = false;
    if (!initialized)
    {
        // tengo que inicializar la matriz, la concha del pato
        //matrix_init();
        //matrix_full_screen();

        initialized = true;
    }
    return initialized;
}

/*******************************************************************************
 * FUNCTION DEFINITIONS
 ******************************************************************************/

void vumeter_set_frequency(vumeter_frequency_t frequency, uint8_t value){

}

void vumeter_set_all_band(uint8_t value){

}


void vumeter_clear(void){

}


void vumeter_set_brightness(uint8_t brightness){
    
}
