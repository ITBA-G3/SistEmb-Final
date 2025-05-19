/*
 * pwm.c
 *
 *  Created on: 5 nov 2024
 *      Author: asolari
 */


#include "PWM.h"

uint16_t ticksPerPeriod = 100;
uint16_t *waveform = 0;
uint32_t waveform_lenght = 0;
uint32_t waveform_offset = 0;


void PWM_Init(void)
{
	//Initialize the FTM and DMA modules
	DMA_Init();
	FTM_Init();

	//Set PTC1 as PWM output
	PORTC->PCR[1] = PORT_PCR_MUX(PORT_mAlt4) | PORT_PCR_DSE(true) | PORT_PCR_IRQC(PORT_eDisabled);

	//  Set FTM configuration and prescaler x1
	FTM_SetPrescaler(FTM0, FTM_PSC_x1);

	//Disable FTM interrupt, waitng finish configuration
	FTM_SetInterruptMode(FTM0, FTM_CH_0, false);

	//	Set FTM as PWM mode
	FTM_SetWorkingMode(FTM0, FTM_CH_0, FTM_mPulseWidthModulation);
	FTM_SetPulseWidthModulationLogic(FTM0, FTM_CH_0, FTM_lAssertedHigh);

	//  Set PWWM Modulus and initial Duty -> 50%
	FTM_SetMod(FTM0, ticksPerPeriod);
	FTM_SetCounter(FTM0, FTM_CH_0, ticksPerPeriod / 2);
}


void PWM_SetDC(float DC)
{
	//Set the duty cycle of the PWM signal modifing the counter value CnV
	FTM_SetCounter(FTM0, FTM_CH_0, ticksPerPeriod * (DC / 100.0f));
}

float PWM_GetDC(void) {
	//Get the current duty cycle of the PWM signal
	return (FTM_GetCounter(FTM0, FTM_CH_0) / (float) ticksPerPeriod) * 100.0f;
}




void PWM_SetTickPerPeriod(uint16_t tPerPeriod)
{
    // Stop the FTM clock to safely change the configuration
    FTM_StopClock(FTM0);

    // Get the current duty cycle percentage
    float DC = PWM_GetDC();

    // Check if the new ticks per period is less than the current value
    if (ticksPerPeriod > tPerPeriod)
    {
        // Update the ticks per period to the new value
        ticksPerPeriod = tPerPeriod;
        // Adjust the duty cycle based on the new ticks per period
        PWM_SetDC(DC);
        // Set the new modulus value for the FTM
        FTM_SetMod(FTM0, ticksPerPeriod);
    }
    else
    {
        // Update the ticks per period to the new value
        ticksPerPeriod = tPerPeriod;
        // Set the new modulus value for the FTM
        FTM_SetMod(FTM0, ticksPerPeriod);
        // Adjust the duty cycle based on the new ticks per period
        PWM_SetDC(DC);
    }

    // Restart the FTM clock with the new configuration
    FTM_StartClock(FTM0);
}

uint16_t PWM_GetTickPerPeriod(void) {
	// Get the number of ticks per period for the PWM signal
	return ticksPerPeriod;
}

// Generates a PWM waveform using DMA to transfer data to the FlexTimer (FTM) for real-time waveform updates.
void PWM_GenWaveform(uint16_t *waveform_pointer, uint32_t wave_length, uint32_t waveTable_offset)
{
	// Initialize waveform parameters.
	waveform = waveform_pointer;             // Pointer to the waveform data array
	waveform_lenght = wave_length;           // Total length of the waveform
	waveform_offset = waveTable_offset;      // Offset used for downsampling to control frequency

	// Stop the FlexTimer to configure DMA transfers without interruption.
	FTM_StopClock(FTM0);

	// Enable DMA mode and interrupts for the FTM channel to handle data transfers.
	FTM_DMAMode(FTM0, FTM_CH_0, true);
	FTM_SetInterruptMode(FTM0, FTM_CH_0, true);

	// Configure DMA circular buffers for source and destination.
	DMA_SetSourceModulo(DMA_CH0, 0);         // Source address modulo set to 0 (no circular buffer)
	DMA_SetDestModulo(DMA_CH0, 0);           // Destination address modulo set to 0 (no circular buffer)

	// Set DMA source and destination addresses.
	DMA_SetSourceAddr(DMA_CH0, (uint32_t)waveform);  // Source address is the waveform array's start
	DMA_SetDestAddr(DMA_CH0, (uint32_t) & (FTM0->CONTROLS[FTM_CH_0].CnV));  // Destination address is FTM CnV register

	// Set the offset to step through the waveform array, simulating downsampling.
	DMA_SetSourceAddrOffset(DMA_CH0, waveform_offset * 2);  // Increment source address by `waveform_offset * 2` (bytes) per transfer
	DMA_SetDestAddrOffset(DMA_CH0, 0);                      // No offset for the destination, always writes to the same register

	// Set source and destination last address offsets to handle end-of-transfer adjustments.
	DMA_SetSourceLastAddrOffset(DMA_CH0, -2 * (int32_t)(waveform_lenght - waveform_offset));  // Reset source to start of waveform
	DMA_SetDestLastAddrOffset(DMA_CH0, 0);  // Destination remains unchanged

	// Configure data transfer sizes for source and destination.
	DMA_SetSourceTransfSize(DMA_CH0, DMA_TransSize_16Bit);  // Set source transfer size to 16 bits
	DMA_SetDestTransfSize(DMA_CH0, DMA_TransSize_16Bit);    // Set destination transfer size to 16 bits

	// Set the minor loop transfer count to 2 bytes (16 bits) per minor cycle.
	DMA_SetMinorLoopTransCount(DMA_CH0, 2);

	// Set major loop counters to control the full transfer cycle based on the waveform length and offset.
	DMA_SetCurrMajorLoopCount(DMA_CH0, waveform_lenght / waveform_offset - 1);  // Sets current major loop count
	DMA_SetStartMajorLoopCount(DMA_CH0, waveform_lenght / waveform_offset - 1); // Sets starting major loop count

	// Enable DMA requests for channel 0.
	DMA_SetEnableRequest(DMA_CH0, true);

	// Configure DMAMUX to link DMA channel 0 to FTM0 channel 0 and start transfer.
	DMAMUX_ConfigChannel(DMA_CH0, true, false, kDmaRequestMux0FTM0Channel0);
	DMA_StartTransfer(DMA_CH0);

	// Clear any pending flags to prepare FTM for the upcoming transfer.
	FTM_ClearInterruptFlag(FTM0, FTM_CH_0);
	FTM_ClearOverflowFlag(FTM0);

	// Start the FTM clock, initiating the PWM output.
	FTM_StartClock(FTM0);
}


void PWM_SetWaveformOffset(uint32_t waveTable_offset)
{
	waveform_offset = waveTable_offset;
	FTM_StopClock(FTM0);

	uint32_t newCount = ((int32_t)DMA_GetSourceAddr(DMA_CH0) - (int32_t)waveform) / (2 * waveform_offset);
	if (newCount > 109)
		waveform = waveform;
	if (newCount < 0)
		waveform = waveform;
	DMA_SetSourceAddr(DMA_CH0, (uint32_t)waveform + newCount * waveform_offset * 2);
	DMA_SetSourceAddrOffset(DMA_CH0, waveform_offset * 2);
	DMA_SetCurrMajorLoopCount(DMA_CH0, waveform_lenght / waveform_offset - 1 - newCount);
	DMA_SetStartMajorLoopCount(DMA_CH0, waveform_lenght / waveform_offset - 1);
	DMA_SetSourceLastAddrOffset(DMA_CH0, -2 * (int32_t)(waveform_lenght - waveform_offset));
	FTM_StartClock(FTM0);
}

