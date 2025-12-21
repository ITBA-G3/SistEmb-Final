/***************************************************************************/ /**
   @file     DAC.c
   @brief    Driver ADC
   @author   Grupo 3
  ******************************************************************************/

#include "DAC.h"

static unsigned char DACbuffer[QSIZE]; 
static uint16_t news;         
static unsigned char *pin, *pout;   

uint8_t DACReady[2] = {false, false};

void DAC_Init (DAC_t dac)
{
	if(dac == DAC0){
		SIM->SCGC2 |= SIM_SCGC2_DAC0_MASK;
	}
	else if (dac == DAC1){
		SIM->SCGC2 |= SIM_SCGC2_DAC1_MASK;
	}

	if(dac == DAC0){
		dac->C0 = DAC_C0_DACEN_MASK | DAC_C0_DACRFS_MASK ;
//		dac->C0 = DAC_C0_DACEN_MASK | DAC_C0_DACRFS_MASK  | DAC_C0_DACTRGSEL_MASK;
//		dac->C1 &= ~(DAC_C1_DMAEN_MASK | DAC_C1_DACBFMD_MASK | DAC_C1_DACBFEN_MASK);
	}
	else if(dac == DAC1)			
		dac->C0 &= ~DAC_C0_DACEN_MASK;

}

void DAC_SetData (DAC_t dac, DACData_t data)
{
	dac->DAT[0].DATL = DAC_DATL_DATA0(data);
	dac->DAT[0].DATH = DAC_DATH_DATA1(data >> DAC_DATL_DATA0_WIDTH);
	uint8_t DACid = 0;
	if(dac == DAC1){
		DACid = 1;
	}
	DACReady[DACid] = false;
}

//void DAC_SetHardwareTrigger(DAC_t dac){
//	dac->C0 &= DAC_C0_DACTRGSEL_MASK; // DACTRGSEL en 0 para indicar hardware trigger
//}

uint8_t DAC_Ready(DAC_t dac){
	uint8_t DACid = 0;
	if(dac == DAC1){
		DACid = 1;
	}
	return DACReady[DACid];
}


/*
 Initialize queue
*/
void DACQueueInit(void)

{
    pin = DACbuffer; // set up pin,pout and "news" counter
    pout = DACbuffer;
    news = 0;
}

/*
  Push data on queue
*/
unsigned char DACPushQueue(unsigned char data)

{

    if (news > QSIZE - 1) // test for Queue overflow
    {
        news = QOVERFLOW; // inform queue has overflowed
        return (news);
    }

    *pin++ = data; // pull data
    news++;        // update "news" counter

    if (pin == DACbuffer + QSIZE) // if queue size is exceded reset pointer
        pin = DACbuffer;

    return (news); // inform Queue state
}

/*
  Retrieve data from queue
*/
unsigned char DACPullQueue(void)
{
	if(news>0){
		unsigned char data;

		data = *pout++; // pull data
		news--;         // update "news" counter

		if (pout == DACbuffer + QSIZE) // Check for Queue boundaries
			pout = DACbuffer;          // if queue size is exceded reset pointer

		return (data);              // rerturn retrieved data
	}
	return 0;
}

/*
  Get queue Status
*/
unsigned char DACQueueStatus(void)

{
    return (news); // Retrieve "news" counter
}
