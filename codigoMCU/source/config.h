/***************************************************************************/ /**
   @file     +Nombre del archivo (ej: template.h)+
   @brief    +Descripcion del archivo+
   @author   +Nombre del autor (ej: Salvador Allende)+
  ******************************************************************************/

#ifndef _TEMPLATE_H_
#define _TEMPLATE_H_

/*******************************************************************************
 * INCLUDE HEADER FILES
 ******************************************************************************/

#include "gpio.h"

/*******************************************************************************
 * CONSTANT AND MACRO DEFINITIONS USING #DEFINE
 ******************************************************************************/

// UART0 TX and RX
#define UART0_TX_PIN PORTNUM2PIN(PB, 17) // PTB17
#define UART0_RX_PIN PORTNUM2PIN(PB, 16) // PTB16

// UART1 TX and RX
#define UART1_TX_PIN PORTNUM2PIN(PE, 0)
#define UART1_RX_PIN PORTNUM2PIN(PE, 1)

// UART2 TX and RX
#define UART2_TX_PIN PORTNUM2PIN(PD, 2)
#define UART2_RX_PIN PORTNUM2PIN(PD, 3)

// UART3 TX and RX
#define UART3_TX_PIN PORTNUM2PIN(PE, 4)
#define UART3_RX_PIN PORTNUM2PIN(PE, 5)

// UART4 TX and RX
#define UART4_TX_PIN PORTNUM2PIN(PE, 24)
#define UART4_RX_PIN PORTNUM2PIN(PE, 25)

#define UART_PINS                                                                                                                                          \
  {                                                                                                                                                        \
    {UART0_TX_PIN, UART0_RX_PIN}, {UART1_TX_PIN, UART1_RX_PIN}, {UART2_TX_PIN, UART2_RX_PIN}, {UART3_TX_PIN, UART3_RX_PIN}, { UART4_TX_PIN, UART4_RX_PIN } \
  }

/*******************************************************************************
 * ENUMERATIONS AND STRUCTURES AND TYPEDEFS
 ******************************************************************************/

/*******************************************************************************
 * VARIABLE PROTOTYPES WITH GLOBAL SCOPE
 ******************************************************************************/

// +ej: extern unsigned int anio_actual;+

/*******************************************************************************
 * FUNCTION PROTOTYPES WITH GLOBAL SCOPE
 ******************************************************************************/

/**
 * @brief TODO: completar descripcion
 * @param param1 Descripcion parametro 1
 * @param param2 Descripcion parametro 2
 * @return Descripcion valor que devuelve
 */
// +ej: char lcd_goto (int fil, int col);+

/*******************************************************************************
 ******************************************************************************/

#endif // _TEMPLATE_H_
