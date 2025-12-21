
/***************************************************************************/ /**
   @file     BTN.c
   @brief    User button functions
   @author   Grupo 3
  ******************************************************************************/

#include "BTN.h"

uint8_t BTN_states[3] = {0};

void init_user_buttons(void)
{
    gpioMode(PIN_PLAY, INPUT_PULLDOWN);
    gpioMode(PIN_PREV, INPUT_PULLDOWN);
    gpioMode(PIN_NEXT, INPUT_PULLDOWN);

    PIT_Init(PIT_2, 200);
    PIT_SetCallback(btn_cb, PIT_2);
}

static void btn_cb(void)
{
    static uint8_t prevState[3] = {0};
    static uint8_t state[3] = {0};

    state[PLAY_BTN] = gpioRead(PIN_PLAY);
    state[NEXT_BTN] = gpioRead(PIN_NEXT);
    state[PREV_BTN] = gpioRead(PIN_PREV);

    if((state[PLAY_BTN] == prevState[PLAY_BTN]) && (state[PLAY_BTN]==HIGH))
        BTN_states[PLAY_BTN] = 1;
    if((state[NEXT_BTN] == prevState[NEXT_BTN]) && (state[NEXT_BTN]==HIGH))
        BTN_states[NEXT_BTN] = 1;
    if((state[PREV_BTN] == prevState[PREV_BTN]) && (state[PREV_BTN]==HIGH))
        BTN_states[PREV_BTN] = 1;

    prevState[PLAY_BTN] = state[PLAY_BTN];
    prevState[NEXT_BTN] = state[NEXT_BTN];
    prevState[PREV_BTN] = state[PREV_BTN];
}

uint8_t get_BTN_state(btn_state_t btn)
{
    static uint8_t state = 0;
    switch(btn)
    {
        case PLAY_BTN:
            state = BTN_states[PLAY_BTN];
            BTN_states[PLAY_BTN] = 0;
            break;
        case NEXT_BTN:
            state = BTN_states[NEXT_BTN];
            BTN_states[NEXT_BTN] = 0;
            break;
        case PREV_BTN:
            state = BTN_states[PREV_BTN];
            BTN_states[PREV_BTN] = 0;
            break;
        default:
    }
    return state;
}
