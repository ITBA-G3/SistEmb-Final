/***************************************************************************//**
  @file     BTN.h
  @brief    User button functions
  @author   Group 3
 ******************************************************************************/

#ifndef BTN_H
#define BTN_H

#include "../drivers/TICKS/ticks.h"
#include "../board.h"
#include "../drivers/gpio.h"
#include <stdbool.h>

typedef enum {
    PLAY_BTN,
    PREV_BTN,
    NEXT_BTN
} btn_state_t;


void init_user_buttons(void);
uint8_t get_BTN_state(btn_state_t btn);

static void btn_cb();

#endif // BTN
