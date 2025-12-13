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
#include "drivers/FAT/ff.h"



#include "drivers/FAT/diskio.h"
/*******************************************************************************
 * CONSTANT AND MACRO DEFINITIONS
 ******************************************************************************/


/*******************************************************************************
 * FUNCTION PROTOTYPES
 ******************************************************************************/


/*******************************************************************************
 * GLOBAL DATA
 ******************************************************************************/

static FATFS fs;

void App_Init(void)
{
	// SDHC driver initialization
	sdhc_enable_clocks_and_pins();
	sdhc_reset(SDHC_RESET_CMD);
	sdhc_reset(SDHC_RESET_DATA);
	__enable_irq();
}

/*******************************************************************************
 * App_Run - loop principal (vacío para este PoC)
 ******************************************************************************/
void App_Run(void)
{

//	static BYTE s0[512];
//
//	DSTATUS st = disk_initialize(0);      // o 1 si tu SD está en pdrv=1
//	DRESULT dr = disk_read(0, s0, 0, 1);
//
//	if (st & STA_NOINIT) while(1) {}
//	if (dr != RES_OK) while(1) {}


    FRESULT fr;
    FIL fil;
    UINT bw, br;
    char readbuf[64];
    char buffer[128];

    fr = f_mount(&fs, "0:", 1);      // SD en pdrv=1 según tu diskio.c
    if (fr != FR_OK) {
        while (1) {}                 // breakpoint: fr
    }

//    fr = f_open(&fil, "0:/test.txt", FA_CREATE_ALWAYS | FA_WRITE);
//    if (fr != FR_OK)
//    {
//    	while (1) {}
//    }
    fr = f_open(&fil, "0:/hola.txt", FA_READ);
	if (fr != FR_OK)
	{
		while (1) {}
	}
//
//    const char *msg = "hola FATFS\r\n";
//    fr = f_write(&fil, msg, strlen(msg), &bw);
//    if (fr != FR_OK || bw != strlen(msg))
//    {
//    	while (1) {}
//    }

	/* Leer contenido */
	do {
		fr = f_read(&fil, buffer, sizeof(buffer) - 1, &br);
		if (fr != FR_OK) {
			while (1) {}
		}

		buffer[br] = '\0';   // para tratarlo como string
		/* acá usás buffer:
		   - imprimir por UART
		   - parsear
		   - copiar a otro lado
		*/

	} while (br > 0);   // EOF cuando br == 0

    fr = f_close(&fil);
    if (fr != FR_OK)
    {
    	while (1) {}
    }

    memset(readbuf, 0, sizeof(readbuf));
    fr = f_open(&fil, "0:/test.txt", FA_READ);
    if (fr != FR_OK)
    {
    	while (1) {}
    }

    fr = f_read(&fil, readbuf, sizeof(readbuf)-1, &br);
    if (fr != FR_OK)
    {
    	while (1) {}
    }

    fr = f_close(&fil);
    if (fr != FR_OK)
    {
    	while (1) {}
    }

    /* Si llegaste acá, FATFS funciona. `readbuf` debería contener "hola FATFS\r\n" */
    while (1) {}
}



/*******************************************************************************
 * LOCAL FUNCTION DEFINITIONS
 ******************************************************************************/

