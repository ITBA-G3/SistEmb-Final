/***************************************************************************//**
  @file     encoder.h
  @brief    Encoder
  @author   Group 3
 ******************************************************************************/

#ifndef ENCODER_H
#define ENCODER_H

/*******************************************************************************
 * INCLUDE HEADER FILES
 ******************************************************************************/

#include <stdint.h>
#include <stdbool.h>
#include "drivers/gpio.h"
#include "../TICKS/ticks.h"
#include "source/board.h"
#include "../SDK/CMSIS/MK64F12.h"

/*******************************************************************************
 * CONSTANT AND MACRO DEFINITIONS
 ******************************************************************************/
// Button states for the encoder switch
typedef enum {
    BTN_NOT,            // Not pressed
    BTN_CLICK,          // Short press (normal click)
    BTN_LONG_CLICK      // Long press (hold)
} encoder_btn_event_t;

// Timing constants
#define ISR_PERIOD_MS       1
#define DEBOUNCE_MS         50      // Minimum time to validate a press
#define LONG_CLICK_MS       1000    // Time threshold for a long click

#define STATE_00    0b00
#define STATE_01    0b01
#define STATE_10    0b10
#define STATE_11    0b11

static uint8_t encoderLastState = 0;
static int16_t turns = 0;

static uint16_t btn_counter = 0;
static encoder_btn_event_t btn_status = BTN_NOT;

static void Encoder_Periodic_ISR(void);

/*******************************************************************************
 * FUNCTION PROTOTYPES
 ******************************************************************************/

/**
 * @brief Initializes the encoder driver.
 */
void encoderInit(void);

/**
 * @brief Returns the number of counts accumulated since the last read.
 * @return int16_t: Movement delta (+ right, - left).
 */
int16_t getTurns(void);

/**
 * @brief Returns if there was an event on the button (Click or Long Click).
 * * @return encoder_btn_event_t: The detected event.
 */
encoder_btn_event_t getSwitchState(void);

#endif /* ENCODER_H */
