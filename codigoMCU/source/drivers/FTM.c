/***************************************************************************/ /**
   @file     FTM.c
   @brief    FTM functions
   @author   Grupo 3
  ******************************************************************************/

#include "FTM.h"
#include "MK64F12.h"
#include "gpio.h"

#define PWM_CLK 800E3
#define FTM_CLK 50E6
#define FTM_MOD (FTM_CLK/PWM_CLK - 1)      // x es frecuencia del reloj del FTM

// Set FTM as PWM to drive WS2812
void FTM_Init(void)
{
    // Enable FTM modules clock
	SIM->SCGC6 |= SIM_SCGC6_FTM0_MASK;

	// NVIC_EnableIRQ(FTM0_IRQn);		// OFF FOR FTM+bitbanging test

    // PTC 1 as PWM output
	PORTC->PCR[1] = PORT_PCR_MUX(PORT_mAlt4) | PORT_PCR_DSE(true) | PORT_PCR_IRQC(PORT_eDisabled);

    // Set default prescaler to 1
    FTM_SetPrescaler(FTM0, FTM_PSC_x1);     // Periodo = 1/60MHz * (MOD + 1) * Prescaler

    FTM_SetInterruptMode(FTM0, FTM_CH_0, false);
    FTM_SetWorkingMode(FTM0, FTM_CH_0, FTM_mPulseWidthModulation);  

    FTM_SetPulseWidthModulationLogic(FTM0, FTM_CH_0, FTM_lAssertedHigh);

    // Set MOD value for 800kHz frequency
    FTM_SetMod(FTM0, FTM_MOD);

    FTM_SetCnV(FTM0, FTM_CH_0, CNV_0);

    // Enable DMA Request
//    FTM_DMAMode(FTM0, FTM_CH_0, true);
    FTM0->CONTROLS[0].CnSC |= FTM_CnSC_DMA_MASK;

    // Ensure channel flag is enabled to generate events
    FTM0->CONTROLS[0].CnSC |= FTM_CnSC_CHIE_MASK;  // optional but helps debugging
//    FTM_StartClock(FTM0);
}


void FTM_SetPrescaler(FTM_Type *base, FTM_Prescal_t prescaler)
{
	base->SC = (base->SC & ~FTM_SC_PS_MASK) | FTM_SC_PS(prescaler);
}

void FTM_SetMod(FTM_Type *base, uint16_t mod)
{
	base->CNTIN = 0;
	base->CNT = 0;
	base->MOD = FTM_MOD_MOD(mod);
}

uint16_t FTM_GetMod(FTM_Type *base)
{
	if (!(SIM->SCGC6 & SIM_SCGC6_FTM0_MASK)) return false;
	return base->MOD;
}

void FTM_StartClock(FTM_Type *base)
{
	base->SC |= FTM_SC_CLKS(FTM_SC_CLKS_SYSTEM_CLOCK);
}

void FTM_StopClock(FTM_Type *base)
{
	base->SC = FTM_SC_CLKS(FTM_SC_CLKS_DISABLED);
	base->CNT = 0;
}

// CHANNEL FUNCTIONS

void FTM_SetWorkingMode(FTM_Type *base, FTMChannel_t channel, FTMMode_t mode)
{
	base->CONTROLS[channel].CnSC = (base->CONTROLS[channel].CnSC & ~(FTM_CnSC_MSB_MASK | FTM_CnSC_MSA_MASK)) |
								   (FTM_CnSC_MSB((mode >> 1) & 0X01) | FTM_CnSC_MSA((mode >> 0) & 0X01));

	if (mode == FTM_mPulseWidthModulation)
	{
		base->PWMLOAD = (FTM_PWMLOAD_LDOK_MASK | (0x01 << channel) | 0x200);
	}
}

FTMMode_t FTM_GetWorkingMode(FTM_Type *base, FTMChannel_t channel)
{
	return (base->CONTROLS[channel].CnSC & (FTM_CnSC_MSB_MASK | FTM_CnSC_MSA_MASK)) >> FTM_CnSC_MSA_SHIFT;
}

// PWM FUNCTIONS

void FTM_SetPulseWidthModulationLogic(FTM_Type *base, FTMChannel_t channel, FTMLogic_t logic)
{
	base->CONTROLS[channel].CnSC = (base->CONTROLS[channel].CnSC & ~(FTM_CnSC_ELSB_MASK | FTM_CnSC_ELSA_MASK)) |
								   (FTM_CnSC_ELSB((logic >> 1) & 0X01) | FTM_CnSC_ELSA((logic >> 0) & 0X01));
}

FTMLogic_t FTM_GetPulseWidthModulationLogic(FTM_Type *base, FTMChannel_t channel)
{
	return (base->CONTROLS[channel].CnSC & (FTM_CnSC_ELSB_MASK | FTM_CnSC_ELSA_MASK)) >> FTM_CnSC_ELSA_SHIFT;
}

// COUNTER FUNCTIONS

// Función para cambiar el valor del registro CnV de un canal específico (AKA Duty Cycle del PWM)

void FTM_SetCnV(FTM_Type *base, FTMChannel_t channel, uint16_t value)
{
	base->CONTROLS[channel].CnV = FTM_CnV_VAL(value);
}

uint16_t FTM_GetCnV(FTM_Type *base, FTMChannel_t channel)
{
	return base->CONTROLS[channel].CnV;
}

// IRQ HANDLER FUNCTIONS

void FTM_SetInterruptMode(FTM_Type *base, FTMChannel_t channel, bool enable)
{
	base->CONTROLS[channel].CnSC = (base->CONTROLS[channel].CnSC & ~FTM_CnSC_CHIE_MASK) | FTM_CnSC_CHIE(enable);
}


bool FTM_IsInterruptPending(FTM_Type *base, FTMChannel_t channel)
{
	return base->CONTROLS[channel].CnSC & FTM_CnSC_CHF_MASK;
}


void FTM_ClearInterruptFlag(FTM_Type *base, FTMChannel_t channel)
{
	base->CONTROLS[channel].CnSC &= ~FTM_CnSC_CHF_MASK;
}


void FTM_DMAMode(FTM_Type *base, FTMChannel_t channel, bool enable)
{
	base->CONTROLS[channel].CnSC = (base->CONTROLS[channel].CnSC & ~(FTM_CnSC_DMA_MASK)) |
								   (FTM_CnSC_DMA(enable));
}





// NO SE USAN


void FTM_SetOverflowMode(FTM_Type *base, bool enable)
{
	base->SC = (base->SC & ~FTM_SC_TOIE_MASK) | FTM_SC_TOIE(enable);
}

bool FTM_IsOverflowPending(FTM_Type *base)
{
	return base->SC & FTM_SC_TOF_MASK;
}

void FTM_ClearOverflowFlag(FTM_Type *base)
{
	base->SC &= ~FTM_SC_TOF_MASK;
}

void FTM_SetInputCaptureEdge(FTM_Type *base, FTMChannel_t channel, FTMEdge_t edge)
{
	base->CONTROLS[channel].CnSC = (base->CONTROLS[channel].CnSC & ~(FTM_CnSC_ELSB_MASK | FTM_CnSC_ELSA_MASK)) |
								   (FTM_CnSC_ELSB((edge >> 1) & 0X01) | FTM_CnSC_ELSA((edge >> 0) & 0X01));
}
FTMEdge_t FTM_GetInputCaptureEdge(FTM_Type *base, FTMChannel_t channel)
{
	return (base->CONTROLS[channel].CnSC & (FTM_CnSC_ELSB_MASK | FTM_CnSC_ELSA_MASK)) >> FTM_CnSC_ELSA_SHIFT;
}

void FTM_SetOutputCompareEffect(FTM_Type *base, FTMChannel_t channel, FTMEffect_t effect)
{
	base->CONTROLS[channel].CnSC = (base->CONTROLS[channel].CnSC & ~(FTM_CnSC_ELSB_MASK | FTM_CnSC_ELSA_MASK)) |
								   (FTM_CnSC_ELSB((effect >> 1) & 0X01) | FTM_CnSC_ELSA((effect >> 0) & 0X01));
}

FTMEffect_t FTM_GetOutputCompareEffect(FTM_Type *base, FTMChannel_t channel)
{
	return (base->CONTROLS[channel].CnSC & (FTM_CnSC_ELSB_MASK | FTM_CnSC_ELSA_MASK)) >> FTM_CnSC_ELSA_SHIFT;
}

void FTM_SetInputCaptureChannelSource(FTM_Type *base, FTM_InputCaptureSource_t source)
{
	switch ((uint32_t)base)
	{
	case (uint32_t)FTM1:
		SIM->SOPT4 = (SIM->SOPT4 & ~SIM_SOPT4_FTM1CH0SRC_MASK) | SIM_SOPT4_FTM1CH0SRC(source);
		break;

	case (uint32_t)FTM2:
		SIM->SOPT4 = (SIM->SOPT4 & ~SIM_SOPT4_FTM2CH0SRC_MASK) | SIM_SOPT4_FTM2CH0SRC(source);
		break;
	}
}

FTM_InputCaptureSource_t FTM_GetInputCaptureChannelSource(FTM_Type *base)
{
	switch ((uint32_t)base)
	{
	case (uint32_t)FTM1:
		return (SIM->SOPT4 & SIM_SOPT4_FTM1CH0SRC_MASK) >> SIM_SOPT4_FTM1CH0SRC_SHIFT;
		break;

	case (uint32_t)FTM2:
		return (SIM->SOPT4 & SIM_SOPT4_FTM2CH0SRC_MASK) >> SIM_SOPT4_FTM2CH0SRC_SHIFT;
		break;
	}
	return 0;
}
