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

	// Send 80 clocks to the card, to initialize internal operations
	sdhc_reset(SDHC_RESET_CMD);
	sdhc_reset(SDHC_RESET_DATA);
	sdhc_initialization_clocks();

    sd_card_t card = {0};
    sdhc_error_t e = sd_card_init_full(&card);
    sdhc_error_t e2;

	uint8_t sector0[512] __attribute__((aligned(4)));
	static uint32_t sector_w[512/4] __attribute__((aligned(4)));
	static uint32_t tx_block[512/4] __attribute__((aligned(4)));
    if(e == SDHC_ERROR_OK){		//
//    	sdhcSetClock(100000);  // 100 kHz


//    	static uint32_t read_buf[512/4] __attribute__((aligned(4)));

    	uint32_t lba = 0;          // por ejemplo, sector 0
		bool is_sdhc = false;       // lo determinás en tu init (ACMD41 -> CCS)

		for(uint8_t i = 0; i < 512/4; i++)
		{
			tx_block[i] = i;
		}
		sdhc_error_t err = sdhc_write_block_cpu(lba, is_sdhc, tx_block);
		if (err != SDHC_ERROR_OK) {
			while (1) {}
		}

		err = sdhc_read_block_cpu(lba, is_sdhc, sector_w);

		if (err != SDHC_ERROR_OK)
		{
			/* Acá poné breakpoint y mirá sdhc_status.current_error y SDHC->IRQSTAT */
			while (1) {}
		}

//    	e2 = sd_read_single_block_adma2(35, sector0);
		if(e2==SDHC_ERROR_DMA){}
    }

    // ver e, card.rca, card.sdhc, card.ocr
	while(true);
}



/*******************************************************************************
 * LOCAL FUNCTION DEFINITIONS
 ******************************************************************************/

