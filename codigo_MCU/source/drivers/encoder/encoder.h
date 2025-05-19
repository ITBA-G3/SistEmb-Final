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

#define TOTAL_STATES 4
/*******************************************************************************
 * FUNCTION PROTOTYPES WITH GLOBAL SCOPE
 ******************************************************************************/

 /**
  * @brief initializes the encoder pins and the interrupt for the encoder switch.
  * 
  */
void encoderInit(void);

/**
 * @brief returns quantity of turns of the encoder.
 *
 * @return int8_t quantity of turns of the encoder. 4 turns = 1 complete turn.
 * Negative values indicate counter clockwise turns, positive values indicate clockwise turns.
 */
int8_t getTurns(void);
 

/**
 * @brief returns the state of the encoder switch (SW_PRESSED, SW_RELEASED).
 * 
 * @return uint8_t state of the encoder switch.
 */
uint8_t getSwitchState(void);

/**
 * @brief turns on the buzzer.
 * 
 */
void buzzerStart(void);

/**
 * @brief turns off the buzzer.
 * 
 */
void buzzerStop(void);

#endif /* ENCODER_H */