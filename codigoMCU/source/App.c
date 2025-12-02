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

//    LEDM_Clear(dev);
    LEDM_SetBrightness(dev, 32);            // Seteo brillo a 25% (64/255) Entonces byte del color verde sera 0x40 (0x04) ????

    LEDM_SetPixel(dev, 1, 1, green);        // Prendo led de fila 1 comulna 2 verde en 3*64 array con formato GRB es el 4to elemento

    LEDM_SetPixel(dev,1, 2,blue);

    LEDM_SetPixel(dev,2,3,red);
    LEDM_SetPixel(dev,3,4,green);
    LEDM_SetPixel(dev,4,5,blue);
    LEDM_SetPixel(dev,5,6,red);
    LEDM_SetPixel(dev,6,7,green);
//    LEDM_SetPixel(dev,7,8,blue);
//    LEDM_SetPixel(dev,8,7,red);


	LEDM_Show(dev);

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
