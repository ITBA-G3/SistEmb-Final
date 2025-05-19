/***************************************************************************//**
  @file     encoder.h
  @brief    encoder driver functions
  @author   Grupo 3
 ******************************************************************************/

#ifndef ENCODER_H
#define ENCODER_H

/*******************************************************************************
 * INCLUDE HEADER FILES
 ******************************************************************************/
#include <stdint.h>

/*******************************************************************************
 * CONSTANT AND MACRO DEFINITIONS USING #DEFINE
 ******************************************************************************/
#define STATE_00 0b00
#define STATE_01 0b01
#define STATE_10 0b10
#define STATE_11 0b11

/*******************************************************************************
 * FUNCTION PROTOTYPES WITH GLOBAL SCOPE
 ******************************************************************************/

 /**
  * @brief initializes the encoder pins and the interrupt for the encoder switch.
  * 
  */
void encoderInit(void);

/**
 * @brief returns the direction of the encoder (ENC_LEFT, ENC_RIGHT, ENC_STILL).
 * 
 * @return int8_t direction of the encoder. Negative values indicate left turns, positive values indicate right turns.
 */
int8_t getEncDir(void);

/**
 * @brief returns quantity of turns of the encoder.
 *
 * @return int8_t quantity of turns of the encoder. Negative values indicate left turns, positive values indicate right turns.
 */
int8_t getEncTurns(void);
 

/**
 * @brief returns the state of the encoder switch (SW_PRESSED, SW_RELEASED).
 * 
 * @return uint8_t state of the encoder switch.
 */
uint8_t getSwitchState(void);

#endif /* ENCODER_H */