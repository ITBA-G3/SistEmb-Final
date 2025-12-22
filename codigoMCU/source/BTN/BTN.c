
/***************************************************************************/ /**
   @file     BTN.c
   @brief    User button functions
   @author   Grupo 3
  ******************************************************************************/

#include "BTN.h"

#define BTN_COUNT          3
#define DEBOUNCE_TICKS     3   // 3 * 20ms = 60ms de estabilidad requerida (ajustable)

static void btn_cb(void);

// IMPORTANTE: si se lee desde otra tarea/ISR, que sea volatile
static volatile uint8_t btn_event_pressed[BTN_COUNT] = {0};

// estado estable (debounced)
static uint8_t debounced[BTN_COUNT] = {0};
// último raw leído
static uint8_t last_raw[BTN_COUNT] = {0};
// contador de estabilidad por botón
static uint8_t stable_cnt[BTN_COUNT] = {0};

static void btn_cb();

uint8_t BTN_states[3] = {0};


void init_user_buttons(void)
{
    gpioMode(PIN_PLAY, INPUT_PULLDOWN);
    gpioMode(PIN_PREV, INPUT_PULLDOWN);
    gpioMode(PIN_NEXT, INPUT_PULLDOWN);

    tickAdd(btn_cb, 2);
}

static inline uint8_t read_btn_raw(btn_state_t b)
{
    switch (b) {
        case PLAY_BTN: return gpioRead(PIN_PLAY);
        case NEXT_BTN: return gpioRead(PIN_NEXT);
        case PREV_BTN: return gpioRead(PIN_PREV);
        default:       return 0;
    }
}

static void btn_cb(void)
{
    for (uint8_t b = 0; b < BTN_COUNT; b++)
    {
        uint8_t raw = read_btn_raw((btn_state_t)b);

        if (raw == last_raw[b]) {
            if (stable_cnt[b] < DEBOUNCE_TICKS) stable_cnt[b]++;
        } else {
            stable_cnt[b] = 0;
            last_raw[b] = raw;
        }

        // si estuvo estable suficientes ticks y difiere del debounced, acepto cambio
        if ((stable_cnt[b] >= DEBOUNCE_TICKS) && (debounced[b] != raw))
        {
            debounced[b] = raw;

            // Evento SOLO en flanco de subida (pressed)
            if (debounced[b] == 1) {
                btn_event_pressed[b] = 1;
            }
        }
    }
}

uint8_t get_BTN_state(btn_state_t btn)
{
    __disable_irq();
    uint8_t ev = btn_event_pressed[btn];
    btn_event_pressed[btn] = 0;
    __enable_irq();
    return ev;
}
