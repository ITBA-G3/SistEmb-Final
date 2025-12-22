/***************************************************************************//**
  @file     board.h
  @brief    Board management
  @author   Grupo 3
 ******************************************************************************/

#ifndef _BOARD_H_
#define _BOARD_H_

/*******************************************************************************
 * INCLUDE HEADER FILES
 ******************************************************************************/

#include "drivers/gpio.h"


/*******************************************************************************
 * CONSTANT AND MACRO DEFINITIONS USING #DEFINE
 ******************************************************************************/

/***** BOARD defines **********************************************************/

// On Board User LEDs
#define PIN_LED_RED     PORTNUM2PIN(PB,22)
#define PIN_LED_GREEN   PORTNUM2PIN(PE,26)
#define PIN_LED_BLUE    PORTNUM2PIN(PB,21) // PTB21

#define LED_ACTIVE      LOW
#define LED_DISABLE		HIGH

// Lector Magnético
#define PIN_LECTOR_ENABLE	PORTNUM2PIN(PB,3)
#define PIN_LECTOR_CLK		PORTNUM2PIN(PB,10)
#define PIN_LECTOR_DATA		PORTNUM2PIN(PB,11)

// Display 7 segmentos
#define PIN_SSEG_A		PORTNUM2PIN(PC,3)
#define PIN_SSEG_B		PORTNUM2PIN(PC,2)
#define PIN_SSEG_C		PORTNUM2PIN(PC,4)
#define PIN_SSEG_D		PORTNUM2PIN(PB,23)
#define PIN_SSEG_E		PORTNUM2PIN(PB,18)
#define PIN_SSEG_F		PORTNUM2PIN(PB,9)
#define PIN_SSEG_G		PORTNUM2PIN(PC,17)
#define PIN_SSEG_DP		PORTNUM2PIN(PC,16)

#define	PIN_SSEG_MUX_0	PORTNUM2PIN(PC,5)
#define	PIN_SSEG_MUX_1	PORTNUM2PIN(PC,7)

// Encoder
#define	PIN_ENCODER_A		PORTNUM2PIN(PC,9)
#define	PIN_ENCODER_B		PORTNUM2PIN(PC,8)

#define	PIN_ENCODER_SWITCH	PORTNUM2PIN(PB,18)

// LED's Informativos
#define	PIN_INFO_LED_0	PORTNUM2PIN(PC,1)
#define	PIN_INFO_LED_1	PORTNUM2PIN(PB,19)

// USER Switches
#define PIN_PLAY        PORTNUM2PIN(PB, 11)
#define PIN_PREV        PORTNUM2PIN(PC, 0)
#define PIN_NEXT        PORTNUM2PIN(PB, 20)

// On Board User Switches
#define PIN_SW2         PORTNUM2PIN(PC,6)
#define PIN_SW3         PORTNUM2PIN(PA,4)

//#define SW_PRESSED      LOW
//#define SW_INPUT_TYPE   // ???

/*******************************************************************************
 ******************************************************************************/

#endif // _BOARD_H_
