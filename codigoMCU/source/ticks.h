/***************************************************************************//**
  @file     Tick.h
  @brief    Tick using RTOS
  @author   Grupo 3
 ******************************************************************************/

#ifndef TICK_H_
#define TICK_H_

/*******************************************************************************
 * INCLUDE HEADER FILES
 ******************************************************************************/
#include <stdbool.h>
#include "hardware.h"

#include <stdint.h>
/*******************************************************************************
 * CONSTANT AND MACRO DEFINITIONS USING #DEFINE
 ******************************************************************************/
#define MAX_FUNCTIONS 20
/*
 * @brief carga funcion a ejecutarse en cada tick
 * @param funcallback Function to be call every tick
 * @return registration succeed
 */
bool tickAdd (pinIrqFun_t funcallback, unsigned int period);

/*
 * @brief llama a todas las callback de tick
 */
void PISR(void);

void App_OS_SetAllHooks(void);

#endif /* TICK_H_ */
