/***************************************************************************/ /**
   @file     main.c
   @brief    MP3 Player main
   @author   Grupo 3
  ******************************************************************************/

/*******************************************************************************
 * INCLUDE HEADER FILES
 ******************************************************************************/
#include "source/drivers/Encoder/encoder.h"
#include "source/drivers/gpio.h"
#include "fsl_debug_console.h"
#include <stdio.h>
#include "source/board.h"
#include "source/drivers/pisr.h"
/*******************************************************************************
 * CONSTANT AND MACRO DEFINITIONS USING #DEFINE
 ******************************************************************************/

/*******************************************************************************
 * FUNCTION PROTOTYPES FOR PRIVATE FUNCTIONS WITH FILE LEVEL SCOPE
 ******************************************************************************/

/*******************************************************************************
 *******************************************************************************
						GLOBAL FUNCTION DEFINITIONS
 *******************************************************************************
 ******************************************************************************/

/* Función que se llama 1 vez, al comienzo del programa */
void App_Init(void)
{
//	BOARD_InitDebugConsole();
	SysTick_Init();
//	__enable_irq();
	encoderInit();
	gpioMode(PIN_LED_GREEN, OUTPUT);
	gpioWrite(PIN_LED_GREEN, HIGH);
}

/* Función que se llama constantemente en un ciclo infinito */
void App_Run(void)
{
	if (getSwitchState() == BTN_LONG_CLICK)
	{
//		gpioToggle(PIN_LED_GREEN);
		int16_t turns = getTurns();
//		printf("giros entre clicks: %d", turns);
		if(turns>3)
		{
			gpioWrite(PIN_LED_GREEN, LOW);
		}
		else
		{
			gpioWrite(PIN_LED_GREEN, HIGH);
		}
	}
	for(int i = 0; i < 100000; i++);
}

/*******************************************************************************
 *******************************************************************************
						LOCAL FUNCTION DEFINITIONS
 *******************************************************************************
 ******************************************************************************/

/*******************************************************************************
 ******************************************************************************/
