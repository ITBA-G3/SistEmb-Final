/***************************************************************************/ /**
   @file     FTM.c
   @brief    FTM functions
   @author   Grupo 3
  ******************************************************************************/

#include "FTM.h"

void FTM_Init(void)
{
	SIM->SCGC6 |= SIM_SCGC6_FTM0_MASK;
	SIM->SCGC6 |= SIM_SCGC6_FTM1_MASK;
	SIM->SCGC6 |= SIM_SCGC6_FTM2_MASK;
	SIM->SCGC3 |= SIM_SCGC3_FTM2_MASK;
	SIM->SCGC3 |= SIM_SCGC3_FTM3_MASK;

	NVIC_EnableIRQ(FTM0_IRQn);
	NVIC_EnableIRQ(FTM1_IRQn);
	NVIC_EnableIRQ(FTM2_IRQn);
	NVIC_EnableIRQ(FTM3_IRQn);
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

void FTM_SetPulseWidthModulationLogic(FTM_Type *base, FTMChannel_t channel, FTMLogic_t logic)
{
	base->CONTROLS[channel].CnSC = (base->CONTROLS[channel].CnSC & ~(FTM_CnSC_ELSB_MASK | FTM_CnSC_ELSA_MASK)) |
								   (FTM_CnSC_ELSB((logic >> 1) & 0X01) | FTM_CnSC_ELSA((logic >> 0) & 0X01));
}
FTMLogic_t FTM_GetPulseWidthModulationLogic(FTM_Type *base, FTMChannel_t channel)
{
	return (base->CONTROLS[channel].CnSC & (FTM_CnSC_ELSB_MASK | FTM_CnSC_ELSA_MASK)) >> FTM_CnSC_ELSA_SHIFT;
}

void FTM_SetCounter(FTM_Type *base, FTMChannel_t channel, uint16_t value)
{
	base->CONTROLS[channel].CnV = FTM_CnV_VAL(value);
}
uint16_t FTM_GetCounter(FTM_Type *base, FTMChannel_t channel)
{
	return base->CONTROLS[channel].CnV;
}

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
