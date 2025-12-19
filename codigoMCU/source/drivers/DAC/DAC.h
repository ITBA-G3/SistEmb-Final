/***************************************************************************/ /**
   @file     DAC.c
   @brief    Driver ADC
   @author   Grupo 3
  ******************************************************************************/
#ifndef DAC_H_
#define DAC_H_

#include "hardware.h"

typedef DAC_Type *DAC_t;
typedef uint16_t DACData_t;

void DAC_Init (DAC_t dac);
void DAC_SetData (DAC_t dac, DACData_t data);

//void DAC_SetHardwareTrigger(DAC_t dac);

uint8_t DAC_Ready(DAC_t dac);

// void DACcb(void);

void DACQueueInit(void);

unsigned char DACPushQueue(unsigned char data);

unsigned char DACPullQueue(void);

unsigned char DACQueueStatus(void);

#endif
