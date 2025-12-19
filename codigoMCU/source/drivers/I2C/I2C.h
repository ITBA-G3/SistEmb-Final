/***************************************************************************//**
  @file     I2C.h
  @brief    I2C communication functions prototypes
  @author   Grupo 3 - Jose Iván Hertter, Ezequiel Diaz Guzmán
 ******************************************************************************/

#ifndef _I2C_H_
#define _I2C_H_

/*******************************************************************************
 * INCLUDE HEADER FILES
 ******************************************************************************/

 #include "../../SDK/startup/hardware.h"
 #include "../../SDK/CMSIS/MK64F12.h"
 #include "../../pisr.h"
 #include <stdbool.h>
 #include <stdarg.h>

/*******************************************************************************
* INCLUDE TYPEDEFS AND ENUMS
*******************************************************************************/

enum RX_Status {RX_READY, RX_NOT_READY};

/*******************************************************************************
 * FUNCTION PROTOTYPES WITH GLOBAL SCOPE
 ******************************************************************************/

void init_I2C (void);

void kill_I2C (void);

bool write_I2C (uint8_t writeAddress, uint8_t* writePayload, uint8_t writeLength);

bool read_I2C (uint8_t slaveAddress, uint8_t dataSize);

void I2C_subroutine (void);

bool isBusBusy(void);

void getData_I2C (unsigned char *arr);

unsigned char getUserQueueLength (void);

#endif//_I2C_H_
