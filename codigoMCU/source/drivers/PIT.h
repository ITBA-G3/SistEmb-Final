///*
// * PIT.h
// * @authors Grupo 3
// * @brief Driver PIT
// */
//
//#ifndef PIT_H_
//#define PIT_H_
//
//
//#include <stdbool.h>
//#include <stdint.h>
//
//#include "hardware.h"
//
//#define PIT_FREQ (50E6)
//
//#define PIT_TICK_PERIOD (1/PIT_FREQ)
//
//#define PIT_NS2TICK(x) ((x)/PIT_TICK_PERIOD)
//
//#define PIT_TIME(freq) ((uint32_t)((PIT_FREQ / (freq)) - 1))
//
//
//typedef enum {PIT_0, PIT_1, PIT_2, PIT_3, PIT_COUNT} PIT_MOD;
//
//typedef void (*PIT_Callback_t)(void);
//
//void PIT_Init(PIT_MOD pit, uint32_t ticks);
//
//void PITStart(PIT_MOD pit);
//
//void PIT_Enable(uint8_t channel);
//
//void PIT_Disable(uint8_t pit);
//
//void PIT_EnableInterrupt(uint8_t pit);
//void PIT_DisableInterrupt(uint8_t pit);
//
//void PIT_SetCallback(PIT_Callback_t cb, PIT_MOD pit);
//
//
//
//#endif /* PIT_H_ */
