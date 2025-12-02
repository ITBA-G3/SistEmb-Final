/***************************************************************************/ /**
   @file     main.c
   @brief    MP3 Player main
   @author   Grupo 3
  ******************************************************************************/

/*******************************************************************************
 * INCLUDE HEADER FILES
 ******************************************************************************/
#include "LEDmatrix.h"
#include "drivers/PIT.h"
/*******************************************************************************
 * CONSTANT AND MACRO DEFINITIONS USING #DEFINE
 ******************************************************************************/

/*******************************************************************************
 * FUNCTION PROTOTYPES FOR PRIVATE FUNCTIONS WITH FILE LEVEL SCOPE
 ******************************************************************************/
static LEDM_t *dev;
static bool start_new_frame;
static uint8_t row = 0;
static uint8_t column = 0;


//static void delay_ms(uint32_t ms)
//{
//    volatile uint32_t i, j;
//    for (i = 0; i < ms; ++i) {
//        for (j = 0; j < 3000U; ++j) {
//            __asm("nop");
//        }
//    }
//}

static void PIT_cb(void){
	start_new_frame = true;
}

/*******************************************************************************
 *******************************************************************************
						GLOBAL FUNCTION DEFINITIONS
 *******************************************************************************
 ******************************************************************************/

/* Función que se llama 1 vez, al comienzo del programa */
void App_Init(void)
{
	dev = LEDM_Init(8, 8);
	PIT_Init(PIT_1, 10);
	PIT_SetCallback(PIT_cb, PIT_1);
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

    bool ok = 1;
    LEDM_color_t color;

    if(row%3 == 0) color = red;
    if(row%3 == 1) color = blue;
    if(row%3 == 2) color = green;


    if(start_new_frame){

    	LEDM_SetPixel(dev, row, column, color);

    	ok = LEDM_Show(dev);
    	if(ok){
    		while(LEDM_TransferInProgress());
    	}
    	LEDM_Clear(dev);

    	column++;

    	if(column%8 == 0){
    		row++;
    		column=0;
    	}
    	if(row%8 == 0){
    		row =0;
    	}
    	start_new_frame = 0;
    }
}


/*******************************************************************************
 *******************************************************************************
						LOCAL FUNCTION DEFINITIONS
 *******************************************************************************
 ******************************************************************************/

/*******************************************************************************
 ******************************************************************************/
