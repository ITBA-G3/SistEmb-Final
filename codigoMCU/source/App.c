/***************************************************************************/ /**
   @file     main.c
   @brief    MP3 Player main
   @author   Grupo 3
  ******************************************************************************/

/*******************************************************************************
 * INCLUDE HEADER FILES
 ******************************************************************************/
#include "LEDmatrix.h"
/*******************************************************************************
 * CONSTANT AND MACRO DEFINITIONS USING #DEFINE
 ******************************************************************************/

/*******************************************************************************
 * FUNCTION PROTOTYPES FOR PRIVATE FUNCTIONS WITH FILE LEVEL SCOPE
 ******************************************************************************/


static void delay_ms(uint32_t ms)
{
    volatile uint32_t i, j;
    for (i = 0; i < ms; ++i) {
        for (j = 0; j < 3000U; ++j) {
            __asm("nop");
        }
    }
}

/*******************************************************************************
 *******************************************************************************
						GLOBAL FUNCTION DEFINITIONS
 *******************************************************************************
 ******************************************************************************/
static LEDM_t *dev;

/* Función que se llama 1 vez, al comienzo del programa */
void App_Init(void)
{
	dev = LEDM_Init(8, 8);
}

/* Función que se llama constantemente en un ciclo infinito */
void App_Run(void)
{
    if (!dev) {
        while (1);
    }

    LEDM_color_t red = { .r = 0xFF, .g = 0x00, .b = 0x00 };
    LEDM_color_t green = { .r = 0x00, .g = 0xFF, .b = 0x00 };
    LEDM_color_t blue = { .r = 0x00, .g = 0x00, .b = 0xFF };


    LEDM_SetBrightness(dev, 8);            // Seteo brillo a 25% (64/255) Entonces byte del color verde sera 0x40 (0x04) ????

    bool ok;
    LEDM_color_t color;

    for(int fila=0; fila<8; fila++){

    	for(int i=0; i<8; i++){
			LEDM_Clear(dev);
			if(fila%3 == 0){
				color = red;
			}
			if(fila%3 == 1){
				color = green;
			}
			if(fila%3 == 2){
				color = blue;
			}
			LEDM_SetPixel(dev, fila, i, color);
			ok = LEDM_Show(dev);
			if(ok){
				while(LEDM_TransferInProgress())
				{
				}
			}
			delay_ms(1000);
    	}
    }
//    for(int i=0; i<8; i++){
//		LEDM_Clear(dev);
//		LEDM_SetPixel(dev,2, i,blue);
//		LEDM_Show(dev);
//		while(LEDM_TransferInProgress())
//		{
//		}
//		delay_ms(2000);
//    }
//    for(int i=0; i<8; i++){
//		LEDM_Clear(dev);
//		LEDM_SetPixel(dev,3, i, red);
//		LEDM_Show(dev);
//		while(LEDM_TransferInProgress())
//		{
//		}
//		delay_ms(2000);
//    }


//    LEDM_SetPixel(dev,7,8,blue);
//    LEDM_SetPixel(dev,8,7,red);



//	for (int y = 1; y < 8; y++) {
//		for (int x = 1; x < 8; x++){
//			LEDM_SetPixel(dev,x,y,green);
//			LEDM_Show(dev);
//			delay_ms(10);
//			LEDM_Clear(dev);
//		}
//	}



}

/*******************************************************************************
 *******************************************************************************
						LOCAL FUNCTION DEFINITIONS
 *******************************************************************************
 ******************************************************************************/

/*******************************************************************************
 ******************************************************************************/
