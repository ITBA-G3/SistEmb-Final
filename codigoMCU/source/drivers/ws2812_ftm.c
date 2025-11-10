#include "ws2812_ftm.h"
#include "FTM.h"
#include <stdint.h>
#include <stdbool.h>
#include <stdlib.h>


bool WS2_TransportInit(void)
{
    FTM_Init();
    return true;
}

void WS2_TransportDeinit(void)
{
    FTM_StopClock(FTM0);
}


bool WS2_TransportSend(uint8_t *buf, uint32_t len)
{
    if (!buf || len == 0) return false;

    uint16_t mod = FTM_GetMod(FTM0);

    // PRE-RESET: ensure > 50 us low BEFORE the frame
    FTM_SetCnV(FTM0, FTM_CH_0, 0);   // hold line low
    // ~80 us reset: 64 periods × 1.25 us = 80 us
    for (int i = 0; i < 64; ++i) {
        while (!FTM_IsOverflowPending(FTM0)) {}
        FTM_ClearOverflowFlag(FTM0);
    }

    // SYNC: start first bit on a fresh PWM period
    FTM_ClearOverflowFlag(FTM0);
    FTM0->CNT = 0;                   // restart counter so first bit is full-length

    for (uint32_t i = 0; i < len; ++i) {

        uint8_t byte = *(buf + i);
        for (int bit = 7; bit >= 0; --bit) {
            bool one = (byte >> bit) & 0x01;
            if(one)
            {
            	mod = one;
            }
            FTM_SetCnV(FTM0, FTM_CH_0, one ? CNV_1 : CNV_0);
            // Wait end of period (TOF)
            while (!FTM_IsOverflowPending(FTM0)) { }
            FTM_ClearOverflowFlag(FTM0);
        }
    }

//POST-RESET: latch the frame (>50 us low)
	FTM_SetCnV(FTM0, FTM_CH_0, 0);
	for (int i = 0; i < 64; ++i) {
		while (!FTM_IsOverflowPending(FTM0)) {}
		FTM_ClearOverflowFlag(FTM0);
	}
	return true;
}
