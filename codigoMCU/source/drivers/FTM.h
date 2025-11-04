/***************************************************************************/ /**
   @file     FTM.h
   @brief    FTM functions
   @author   Grupo 3
  ******************************************************************************/

#ifndef FTM_H_
#define FTM_H_

/*******************************************************************************
 * INCLUDE HEADER FILES
 ******************************************************************************/

#include <stdint.h>
#include <stdbool.h>
#include <stdlib.h>
#include "hardware.h"
#include "MK64F12.h"


/*------------------------------------------------------------
 * 2) tH (ns) → CnV  dado f_FTM (Hz)
 *    CnV = round( tH * f_FTM )
 *    Útil para WS2812: t0h≈350ns, t1h≈900ns
 *-----------------------------------------------------------*/
#define FTM_NS_2_CNV(x) ((uint16_t) ( ((uint64_t)(x) * (uint64_t)(50000000UL) + 500000000ULL) / 1000000000ULL ))

#define CNV_0 (FTM_NS_2_CNV(350)) // 18
#define CNV_1 (FTM_NS_2_CNV(900)) // 45



/*******************************************************************************
 * ENUMERATIONS AND STRUCTURES AND TYPEDEFS
 ******************************************************************************/
typedef enum
{
  FTM_CH_0 = 0,
  FTM_CH_1,
  FTM_CH_2,
  FTM_CH_3,
  FTM_CH_4,
  FTM_CH_5,
  FTM_CH_6,
  FTM_CH_7,
} FTM_Channel_t;

typedef enum
{
  FTM_SC_CLKS_DISABLED = 0b00,
  FTM_SC_CLKS_SYSTEM_CLOCK = 0b01,
  FTM_SC_CLKS_FIXED_FREQUENCY = 0b10,
  FTM_SC_CLKS_EXTERNAL_CLOCK = 0b11,
} FTM_SC_CLKS_t;

typedef enum
{
  FTM_mInputCapture,
  FTM_mOutputCompare,
  FTM_mPulseWidthModulation,
} FTMMode_t;

typedef enum
{
  FTM_eRising = 0x01,
  FTM_eFalling = 0x02,
  FTM_eEither = 0x03,
} FTMEdge_t;

typedef enum
{
  FTM_eToggle = 0x01,
  FTM_eClear = 0x02,
  FTM_eSet = 0x03,
} FTMEffect_t;

typedef enum
{
  FTM_lAssertedHigh = 0x02,
  FTM_lAssertedLow = 0x03,
} FTMLogic_t;

typedef enum
{
  FTM_PSC_x1 = 0x00,
  FTM_PSC_x2 = 0x01,
  FTM_PSC_x4 = 0x02,
  FTM_PSC_x8 = 0x03,
  FTM_PSC_x16 = 0x04,
  FTM_PSC_x32 = 0x05,
  FTM_PSC_x64 = 0x06,
  FTM_PSC_x128 = 0x07,

} FTM_Prescal_t;

typedef enum
{
  FTMx_CH0_SIGNAL = 0x00,
  CMP0_OUTPUT = 0x01,
  CMP1_OUTPUT = 0x02,
} FTM_InputCaptureSource_t;

typedef uint16_t FTMData_t;
typedef uint32_t FTMChannel_t;

/*******************************************************************************
 * FUNCTION PROTOTYPES WITH GLOBAL SCOPE
 ******************************************************************************/

void FTM_Init(void);

void FTM_SetPrescaler(FTM_Type *base, FTM_Prescal_t prescaler);

void FTM_SetMod(FTM_Type *base, uint16_t mod);
uint16_t FTM_GetMod(FTM_Type *base);

void FTM_StartClock(FTM_Type *base);
void FTM_StopClock(FTM_Type *base);

void FTM_SetWorkingMode(FTM_Type *base, FTMChannel_t channel, FTMMode_t mode);
FTMMode_t FTM_GetWorkingMode(FTM_Type *base, FTMChannel_t channel);

void FTM_SetPulseWidthModulationLogic(FTM_Type *base, FTMChannel_t channel, FTMLogic_t logic);
FTMLogic_t FTM_GetPulseWidthModulationLogic(FTM_Type *base, FTMChannel_t channel);

void FTM_SetCnV(FTM_Type *base, FTMChannel_t channel, uint16_t value);
uint16_t FTM_GetCnV(FTM_Type *base, FTMChannel_t channel);

void FTM_SetInterruptMode(FTM_Type *base, FTMChannel_t channel, bool enable);
bool FTM_IsInterruptPending(FTM_Type *base, FTMChannel_t channel);
void FTM_ClearInterruptFlag(FTM_Type *base, FTMChannel_t channel);

void FTM_DMAMode(FTM_Type *base, FTMChannel_t channel, bool enable);



void FTM_SetOverflowMode(FTM_Type *base, bool enable);
bool FTM_IsOverflowPending(FTM_Type *base);
void FTM_ClearOverflowFlag(FTM_Type *base);

void FTM_SetInputCaptureEdge(FTM_Type *base, FTMChannel_t channel, FTMEdge_t edge);
FTMEdge_t FTM_GetInputCaptureEdge(FTM_Type *base, FTMChannel_t channel);

void FTM_SetOutputCompareEffect(FTM_Type *base, FTMChannel_t channel, FTMEffect_t effect);
FTMEffect_t FTM_GetOutputCompareEffect(FTM_Type *base, FTMChannel_t channel);

void FTM_SetInputCaptureChannelSource(FTM_Type *base, FTM_InputCaptureSource_t source);
FTM_InputCaptureSource_t FTM_GetInputCaptureChannelSource(FTM_Type *base);

#endif /* FTM_H_ */
