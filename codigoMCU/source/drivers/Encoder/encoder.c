/***************************************************************************//**
  @file     encoder.c
  @brief    Driver del encoder usando Look-Up Table (LUT) y Polling
  @author   Grupo 3
 ******************************************************************************/
#include "encoder.h"
#include "drivers/gpio.h"
#include "drivers/pisr.h"
#include "source/board.h"
#include "../SDK/CMSIS/MK64F12.h"

#include <stdbool.h>

/*******************************************************************************
 * CONSTANTS & MACROS
 ******************************************************************************/

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

void encoderInit(void) {
    gpioMode(PIN_ENCODER_A, INPUT_PULLUP);
    gpioMode(PIN_ENCODER_B, INPUT_PULLUP);
    gpioMode(PIN_ENCODER_SWITCH, INPUT_PULLUP);

    bool a = gpioRead(PIN_ENCODER_A);
    bool b = gpioRead(PIN_ENCODER_B);
    encoderLastState = (a << 1) + b; 

    turns = 0;
    btn_counter = 0;
    btn_status = BTN_NOT;

    pisrRegister(Encoder_Periodic_ISR, ISR_PERIOD_MS);
}

int16_t getTurns(void) {
    int16_t ret = turns;
    turns = 0; 
    return ret;
}

encoder_btn_event_t getSwitchState(void){
    encoder_btn_event_t event = btn_status;
    btn_status = BTN_NOT;
    return event;
}

static void Encoder_Periodic_ISR(void) {
    bool a = !gpioRead(PIN_ENCODER_A);
    bool b = !gpioRead(PIN_ENCODER_B);
    uint8_t currentState = (a << 1) + b;

    if(currentState != encoderLastState) {
        switch (currentState) {
            case STATE_00:
                if (encoderLastState == STATE_01) turns++;
                else if (encoderLastState == STATE_10) turns--;
                break;
            case STATE_11:
                if (encoderLastState == STATE_10) turns++;
                else if (encoderLastState == STATE_01) turns--;
                break;
            case STATE_01:
                if (encoderLastState == STATE_11) turns++;
                else if (encoderLastState == STATE_00) turns--;
                break;
            case STATE_10:
                if (encoderLastState == STATE_00) turns++;
                else if (encoderLastState == STATE_11) turns--;
                break;
        }
        encoderLastState = currentState;
    }

    if (!gpioRead(PIN_ENCODER_SWITCH)) {
        if (btn_counter < 0xFFFF) btn_counter++;
    } else {
        if(btn_counter > 0) {
            if (btn_counter >= LONG_CLICK_MS) {
                btn_status = BTN_LONG_CLICK;
            } else if (btn_counter >= DEBOUNCE_MS) {
                btn_status = BTN_CLICK;
            }
            btn_counter = 0;
        }
    }
}
