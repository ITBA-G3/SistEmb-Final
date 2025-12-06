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

/*******************************************************************************
 * CONSTANT AND MACRO DEFINITIONS
 ******************************************************************************/
// Button states for the encoder switch
typedef enum {
    BTN_NOT,           // Not pressed
    BTN_CLICK,          // Short press (normal click)
    BTN_LONG_CLICK      // Long press (hold)
} encoder_btn_event_t;

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