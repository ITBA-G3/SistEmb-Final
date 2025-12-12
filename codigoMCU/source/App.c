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
#include "sdhc_init_test.h"
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
}

/*******************************************************************************
 * App_Run - loop principal (vacío para este PoC)
 ******************************************************************************/
void App_Run(void)
{
	sdhc_enable_clocks_and_pins();
//	sdhc_command_t command;
//	sdhc_data_t data;
//	bool success;

	// Send 80 clocks to the card, to initialize internal operations
	sdhc_reset(SDHC_RESET_CMD);
	sdhc_reset(SDHC_RESET_DATA);
	sdhc_initialization_clocks();

    sd_card_t card = {0};
    sdhc_error_t e = sd_card_init_full(&card);
    if(e == SDHC_ERROR_OK){}

    // ver e, card.rca, card.sdhc, card.ocr
	while(true);
}

/*******************************************************************************
 * LOCAL FUNCTION DEFINITIONS
 ******************************************************************************/

