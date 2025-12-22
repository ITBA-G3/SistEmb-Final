/***************************************************************************/ /**
   @file     DAC.c
   @brief    Driver DAC
   @author   Grupo 3
  ******************************************************************************/
#ifndef DAC_H_
#define DAC_H_

#include "hardware.h"
#include "MK64F12.h"

#define DAC_DATL_DATA0_WIDTH 8
#define QSIZE 254
#define QOVERFLOW 0xFF



typedef DAC_Type *DAC_t;
typedef uint16_t DACData_t;

void DAC_Init (DAC_t dac);
void DAC_SetData (DAC_t dac, DACData_t data);

uint8_t DAC_Ready(DAC_t dac);

void DACQueueInit(void);

unsigned char DACPushQueue(unsigned char data);

unsigned char DACPullQueue(void);

unsigned char DACQueueStatus(void);

#endif
