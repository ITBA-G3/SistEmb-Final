/***************************************************************************//**
  @file     LCD.h
  @brief    Function declarations for LCD display manipulation
  @author   Grupo 3 
 ******************************************************************************/

#ifndef _LCD_H_
#define _LCD_H_

/*******************************************************************************
 * INCLUDE HEADER FILES
 ******************************************************************************/

#include <stddef.h>
#include <stdint.h>
#include "../I2C/I2C.h"
#include "../PIT.h"
#include "os.h"
#include "../gpio.h"

#define DISPLAY_WIDTH 16
#define PCF8574T_SLAVE_ADDR 0x27  // 0b0100111

#define EN_MASK   0x04
#define COMMAND_NIBBLE(RW, RS) (((RW!=0)?(0b1<<1):(0)) | ((RS!=0)?(0b1):(0)) | (0x08))
//  p7 p6 p5 p4 p3 p2 p1 p0   --> PCF8574T
//  |  |  |  |  |  |  |  | 
//  D7 D6 D5 D4 BL E  RW Rs   --> Display LCM1602

#define TICKS_PER_SECOND  PISR_FREQUENCY_HZ
#define TICKS_PER_US      (TICKS_PER_SECOND / 1000000UL)

/********************************************
 * FUNCTION PROTOTYPES WITH GLOBAL SCOPE
 *******************************************/

/************************
 *   User functions
 ***********************/
void init_LCD(void);

void write_LCD(char* str, uint8_t line);

void shift_LCD(uint8_t line);

void clear_LCD();

void clear_line(uint8_t line);

/************************
 *   Internal functions
 ***********************/

void return_home(void);

void set_cursor_line(uint8_t line);

void write_char();

void write_byte(uint8_t data, uint8_t RW, uint8_t RS);

void write_nibble(uint8_t nibble, uint8_t RW, uint8_t RS);


#endif // _LCD_H_
