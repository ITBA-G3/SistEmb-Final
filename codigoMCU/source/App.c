/***************************************************************************/ /**
   @file     main.c
   @brief    MP3 Player main (SDHC minimal access - polling)
   @author   Grupo 3 (corregido)
  ******************************************************************************/

#include <stdint.h>
#include <stddef.h>
#include <string.h>
#include "MK64F12.h"
#include "pin_mux.h"
#include "gpio.h"
#include "board.h"
#include "drivers/SDHC/sdhc.h"
/*******************************************************************************
 * CONSTANT AND MACRO DEFINITIONS
 ******************************************************************************/


/*******************************************************************************
 * FUNCTION PROTOTYPES
 ******************************************************************************/


/*******************************************************************************
 * GLOBAL DATA
 ******************************************************************************/

void App_Init(void)
{
	// SDHC driver initialization
	__enable_irq();
	sdhc_enable_clocks_and_pins();
//	NVIC_SetPendingIRQ(SDHC_IRQn);
	sdhc_command_t command;
	sdhc_data_t data;
	bool success;

	// Send 80 clocks to the card, to initialize internal operations
	sdhc_reset(SDHC_RESET_CMD);
	sdhc_initialization_clocks();

	// GO_IDLE_STATE: Send CMD0, to reset all MMC and SD cards.
	success = false;
	command.index = 0;
	command.argument = 0;
	command.commandType = SDHC_COMMAND_TYPE_NORMAL;
	command.responseType = SDHC_RESPONSE_TYPE_NONE;
	if (sdhc_transfer(&command, NULL) == SDHC_ERROR_OK)
	{
		success = true;
	}

	// SEND_IF_COND: Send CMD8, asks the SD card if works with the given voltage range.
	if (success)
	{
		success = false;
		command.index = 8;
		command.argument = 0x100 | 0xAA;
		command.commandType = SDHC_COMMAND_TYPE_NORMAL;
		command.responseType = SDHC_RESPONSE_TYPE_R7;
		if (sdhc_transfer(&command, NULL) == SDHC_ERROR_OK)
		{
			if ((command.response[0] & 0xFF) == 0xAA)
			{
				success = true;
			}
		}
	}
}

/*******************************************************************************
 * App_Run - loop principal (vacío para este PoC)
 ******************************************************************************/
void App_Run(void)
{
    /* vacio */
}

/*******************************************************************************
 * LOCAL FUNCTION DEFINITIONS
 ******************************************************************************/

