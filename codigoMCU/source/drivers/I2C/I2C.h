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

#include <stdint.h>
#include <stdbool.h>

/*******************************************************************************
 * FUNCTION PROTOTYPES WITH GLOBAL SCOPE
 ******************************************************************************/

void init_I2C(void);

bool write_I2C(uint8_t address, char *payload, uint8_t size);

#endif // _I2C_H
