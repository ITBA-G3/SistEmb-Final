/*
 * PIT.c
 * @authors Grupo 3
 * @brief Driver PIT
 */

#include "PIT.h"
#include "MK64F12.h"
#include <stddef.h>

#define NUMPITCHANNEL 3

static PIT_Callback_t PIT_Callbacks[NUMPITCHANNEL];

void PIT_Init(PIT_MOD pit, uint32_t freq){

	// Clock Gating for PIT
	SIM->SCGC6 |= SIM_SCGC6_PIT_MASK;
	// PIT Module enable
	PIT->MCR &= ~PIT_MCR_MDIS_MASK;		// PIT module activated

	/* ===================================== */
	/* Configure timer operation when in debug mode */

	//    PIT->MCR &= ~PIT_MCR_FRZ_MASK;

    PIT->MCR |= PIT_MCR_FRZ_MASK;

    PIT->CHANNEL[pit].LDVAL = PIT_TIME(freq);
    PIT->CHANNEL[pit].TCTRL |= PIT_TCTRL_TEN_MASK;

    // Enable interrupt
    PIT_EnableInterrupt(pit);
}


void PIT_Enable(uint8_t pit){
    PIT->CHANNEL[pit].TCTRL |= PIT_TCTRL_TEN_MASK;
}

void PIT_Disable(uint8_t pit){
    PIT->CHANNEL[pit].TCTRL &= ~PIT_TCTRL_TEN_MASK;
}

void PIT_SetCallback(PIT_Callback_t cb, PIT_MOD pit){
	PIT_Callbacks[pit] = cb;
}

void PIT_EnableInterrupt(uint8_t pit){
	NVIC_EnableIRQ(PIT0_IRQn + pit);
	PIT->CHANNEL[pit].TCTRL |= PIT_TCTRL_TIE_MASK;	// enable interrupt
    PIT->CHANNEL[pit].TFLG |= PIT_TFLG_TIF_MASK;	// TIF en 1 : timeout has ocurred (primera vez que interrumpo)
}

void PIT_DisableInterrupt(uint8_t pit){
	NVIC_DisableIRQ(PIT0_IRQn + pit);
	PIT ->CHANNEL[pit].TCTRL &= ~PIT_TCTRL_TIE_MASK;
}

void PIT1_IRQHandler(void){
	PIT->CHANNEL[1].TFLG = PIT_TFLG_TIF_MASK;
	PIT_Callbacks[1]();
}

void PIT0_IRQHandler(void){
	PIT->CHANNEL[0].TFLG = PIT_TFLG_TIF_MASK;
	PIT_Callbacks[0]();
}
