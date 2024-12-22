#ifndef DISPLAY_BOARD_H
#define DISPLAY_BOARD_H

/*******************************************************************************
 * INCLUDE HEADER FILES
 ******************************************************************************/
#include "board.h"

/*******************************************************************************
 * CONSTANT AND MACRO DEFINITIONS USING #DEFINE
 ******************************************************************************/
#define LEDS_BOARD {PIN_SSEG_A,PIN_SSEG_B,PIN_SSEG_C,PIN_SSEG_D,PIN_SSEG_E,PIN_SSEG_F,PIN_SSEG_G,PIN_SSEG_DP}
#define MUX_BOARD {PIN_SSEG_MUX_0,PIN_SSEG_MUX_1}

#define ADDR_D0 {LOW,LOW}
#define ADDR_D1 {HIGH,LOW}
#define ADDR_D2 {LOW,HIGH}
#define ADDR_D3 {HIGH,HIGH}
#define ADDR_MUX {ADDR_D0,ADDR_D1,ADDR_D2,ADDR_D3}

#define REFRESH_TIME 2 // 5ms
#define BLINK_TIME 500 // 500ms

#define LED_ON HIGH // polarity 
#define LED_OFF LOW // polarity 

#define D_OFF 0X00 // - 32

//     define numbers     //
#define D_0 0b00111111
#define D_1 0b00000110
#define D_2 0b01011011
#define D_3 0b01001111
#define D_4 0b01100110
#define D_5 0b01101101
#define D_6 0b01111101
#define D_7 0b00000111
#define D_8 0b01111111
#define D_9 0b01101111
/**************************/

//     define letters     //   array Index
#define D_A 0b01110111  // A - 10
#define D_B 0b01111100  // b - 11
#define D_C 0b00111001  // C - 12
#define D_D 0b01011110  // d - 13
#define D_E 0b01111001  // E - 14
#define D_F 0b01110001  // F - 15
#define D_G 0b00111101  // G - 16
#define D_H 0b01110110  // H - 17
#define D_I 0b00000110  // I - 18
#define D_J 0b00011110  // J - 19
#define D_L 0b00111000  // L - 20
#define D_N 0b01010100  // n - 21
#define D_O 0b01011100  // O - 22
#define D_P 0b01110011  // P - 23
#define D_Q 0b01100111  // q - 24
#define D_R 0b01010000  // R - 25
#define D_S 0b01101101  // S - 26
#define D_U 0b00111110  // U - 27
/**************************/

//     define symbols     //
#define D_DASH 0b01000000 // -  28
#define D_UNDERSCORE 0b00001000 // _ - 31
#define D_DONE 0b00001110 // right 'arrow' - 29
#define D_RETURN 0b00110001 // left 'arrow' - 30

#define TOTAL_SIMB 33
#define SIMB {D_0,D_1,D_2,D_3,D_4,D_5,D_6,D_7,D_8,D_9,D_A,D_B,D_C,D_D,D_E,D_F,D_G,D_H,D_I,D_J,D_L,D_N,D_O,D_P,D_Q,D_R,D_S,D_U,D_DASH,D_DONE,D_RETURN, D_UNDERSCORE, D_OFF}

#define TOTAL_MUX 2

#endif /* DISPLAY_BOARD_H */
