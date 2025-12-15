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
#include "Visualizer.h"
#include "Audio.h"

/*******************************************************************************
 * CONSTANT AND MACRO DEFINITIONS USING #DEFINE
 ******************************************************************************/

/*******************************************************************************
 * FUNCTION PROTOTYPES FOR PRIVATE FUNCTIONS WITH FILE LEVEL SCOPE
 ******************************************************************************/
static LEDM_t *dev;
static bool start_new_frame;

volatile bool PIT_trigger;
volatile bool DMA_trigger;


static void PIT_cb(void){
	start_new_frame = true;
}

/*******************************************************************************
 *******************************************************************************
						GLOBAL FUNCTION DEFINITIONS
 *******************************************************************************
 ******************************************************************************/

void App_Init(void)
{
//	LEDMATRIX TEST
//	dev = LEDM_Init(8, 8);
//	PIT_Init(PIT_0, 10);		// PIT 0 para controlar FPS y refrescar matrix de leds,
//	PIT_SetCallback(PIT_cb, PIT_0);

	// AUDIO TEST

	 //Enable port clock for DAC pin
	SIM->SCGC5 |= SIM_SCGC5_PORTB_MASK;

	 //Put DAC pin in pure analog mode
	PORTB->PCR[2] = 0;   // adjust pin number to your board

//	build_ramp();
	build_sine_table();
	Audio_Init();
	__enable_irq();
}

void App_Run(void)
{
//// LED MATRIX TEST
//    if (!dev) {
//        while (1);
//    }
//
//    bool ok;
//
//
//    LEDM_SetBrightness(dev, 8);
//
//    if(start_new_frame){
//    	start_new_frame = 0;
//    	Visualizer_UpdateFrame(dev);
//
//    	ok = LEDM_Show(dev);
//    	if(ok){
//    		while(LEDM_TransferInProgress());
//    	}
//    	LEDM_Clear(dev);
//    }

    // AUDIO TEST
    if(PIT_trigger){
    	PIT_trigger = false;
    }
    if(DMA_trigger){
    	DMA_trigger = false;
    }
}


/*******************************************************************************
 *******************************************************************************
						LOCAL FUNCTION DEFINITIONS
 *******************************************************************************
 ******************************************************************************/

/*******************************************************************************
 ******************************************************************************/
