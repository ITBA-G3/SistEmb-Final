/***************************************************************************/ /**
   @file     encoder.c
   @brief    encoder driver functions
   @author   Grupo 3
  ******************************************************************************/

/*******************************************************************************
 * INCLUDE HEADER FILES
 ******************************************************************************/
#include "encoder.h"
#include <stdbool.h>
#include "gpio.h"
#include "pisr.h"
#include "board.h"
#include "../SDK/CMSIS/MK64F12.h"

/*******************************************************************************
 * INTERRUPT SERVICE ROUTINES
 ******************************************************************************/
static void SwitchPressed_IRQ(void);
static void RotateEncoder_IRQ(void);

/*******************************************************************************
 * VARIABLES WITH GLOBAL SCOPE
 ******************************************************************************/
static int8_t turns; // Number of turns (positive or negative, depending on the direction)
static int8_t button;
static int8_t encoderLastState;

/*******************************************************************************
 * INITIALIZATION FUNCTION
 ******************************************************************************/
void encoderInit(void)
{
    PORTB->PCR[20] = 0x0;
    PORTB->PCR[19] = 0x0;
    PORTB->PCR[18] = 0x0;
    gpioMode(PIN_ENCODER_A, INPUT);
    gpioMode(PIN_ENCODER_B, INPUT);
    gpioMode(PIN_ENCODER_SWITCH, INPUT);

    pisrRegister(SwitchPressed_IRQ, 5);
    pisrRegister(RotateEncoder_IRQ, 1);

    turns = 0;
    button = 0;
    encoderLastState = (gpioRead(PIN_ENCODER_A) << 1) + gpioRead(PIN_ENCODER_B); // Read the initial state of the encoder
}

/*******************************************************************************
 * FUNCTION DEFINITIONS
 ******************************************************************************/
int8_t getTurns(void)
{
    if (turns >= TOTAL_STATES || turns <= -TOTAL_STATES)
    {                                                // this is a complete turn
        int8_t completeTurns = turns / TOTAL_STATES; // Calculate the number of complete turns
        turns = 0;                                   // Reset the turns variable
        return completeTurns;                        // Return the number of complete turns
    }
    else
    {
        return 0; // if no complete turns have been made
    }
}

uint8_t getSwitchState(void)
{
    uint8_t buttonPress = button; // Store the quantity of times the button has been pressed
    button = 0;                   // Reset the button state for the next iteration
    return buttonPress;           // Return the state of the encoder switch
}

/*******************************************************************************
 * INTERRUPTIONS DEFINITIONS
 ******************************************************************************/

// @brief This function is called when the encoder switch is pressed, it increments the button state
static void SwitchPressed_IRQ(void)
{
    static uint8_t lastState = 0;
    uint8_t switchState = gpioRead(PIN_ENCODER_SWITCH);

    if (switchState != lastState && lastState == SW_PRESSED)
    {             // SW_PRESSED available in board.h
        button++; // Increment the button state (remember to reset it in the app)
    }

    lastState = switchState; // for the next iteration
}

// @brief This function is called when the encoder is rotated, it increments or decrements the turns variable
// depending on the direction of the rotation (decrement if counter-clockwise, increment if clockwise)
static void RotateEncoder_IRQ(void)
{
    uint8_t currentState = (gpioRead(PIN_ENCODER_A) << 1) + gpioRead(PIN_ENCODER_B); // Read the current state of the encoder

    switch (currentState)
    { // remember that the encoder sequence is 11, 01, 00, 10
      // when moving in clockwise
    case STATE_00:
        if (encoderLastState == STATE_01)
        { // clockwise
            turns++;
        }
        else if (encoderLastState == STATE_10)
        { // counter clockwise
            turns--;
        }
        break;
    case STATE_01:
        if (encoderLastState == STATE_11)
        { // clockwise
            turns++;
        }
        else if (encoderLastState == STATE_00)
        { // counter clockwise
            turns--;
        }
        break;
    case STATE_10:
        if (encoderLastState == STATE_00)
        { // clockwise
            turns++;
        }
        else if (encoderLastState == STATE_11)
        { // counter clockwise
            turns--;
        }
        break;
    case STATE_11:
        if (encoderLastState == STATE_10)
        { // clockwise
            turns++;
        }
        else if (encoderLastState == STATE_01)
        { // counter clockwise
            turns--;
        }
        break;
    }

    encoderLastState = currentState; // for the next iteration
}

void buzzerStart(void)
{
    gpioWrite(PIN_BUZZER, HIGH);
}

void buzzerStop(void)
{
    gpioWrite(PIN_BUZZER, LOW);
}