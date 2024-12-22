#ifndef DISPLAY_H
#define DISPLAY_H

#include <stdint.h>

/*******************************************************************************
 * CONSTANT AND MACRO DEFINITIONS USING #DEFINE
 ******************************************************************************/

#define TOTAL_LEDS 8 //number of LEDs on an 8-segment display
#define TOTAL_DISP 4 //number of 8-segment displays

#define MAX_DIGITS 8 //longest word to show in 4 displays
#define MAX_INT 8 //maximum value for display brightness

/*******************************************************************************
 * FUNCTION PROTOTYPES WITH GLOBAL SCOPE
 ******************************************************************************/
/**
 * @brief initialize the display
 */
void dispInit(void);

/**
 * @brief turns on a selected digit
 * @param simb: symbol to display
 * @param dig: a specific digit for show 
 */
void writeDig(uint8_t simb, uint8_t dig);

/**
 * @brief shows four selected symbols (numbers o letters)
 * @param data: pointer to array containing the sorted symbols
 */
void writeWord(uint8_t* data);

/**
 * @brief activate or not the LED pin of the selected digit
 * @param state: turn on or not the LED, LED_ON or LED_OFF respectively
 * @param dig: digit to edit
 */
void writeDot(uint8_t state, uint8_t dig);

/**
 * @brief causes a digit to blink or not
 * @param dig: selected digit
 */
void blinkDig(uint8_t dig);

/**
 * @brief stops the blinking
 */
void stopBlink(void);

/**
 * @brief sets brightness for all digits
 * @param value: value for brightness level, from 0 to MAX_INT (4) 
 */
void setBrightness(uint8_t value);

/**
 * @brief rotates a sequence of symbols in the displays
 * @param word: array with 8 symbols to rotate
 */
void beginRotation(uint8_t* word, uint8_t times);

/**
 * @brief stops rotation of the word on displays 
 */
void stopRotation(void);

uint8_t * getSymbArr(void);

void clearDisplay(void);

/**
 * @brief turns on one stateLed
 *
 * @param led: led to turn on, if 0 received, turns off all leds  
 */
void writeInfoLed(uint8_t led);

#endif /*DISPLAY_H*/
