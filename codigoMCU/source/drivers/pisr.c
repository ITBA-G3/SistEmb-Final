#include "pisr.h"
#include "../SDK/startup/hardware.h"
#include "gpio.h"
#include "board.h"

static pisr_callback_type pisr_array [PISR_CANT] = {0};

bool pisrRegister (pisr_callback_t fun, unsigned int period)
{
	if (fun != 0)
	{
		static unsigned int counter = 0;
		pisr_array[counter].callback_ptr = fun;
		pisr_array[counter].period = period;
		counter++;
		return true;
	}
	return false;
}

void SysTick_Handler (void)
{
	static unsigned int counter = 0;
	for (unsigned int i = 0; i<PISR_CANT; i++)
	{
		if((counter%(pisr_array[i].period) == 0)  && (pisr_array[i].callback_ptr!=0))
		{
			pisr_array[i].callback_ptr();
		}
	}
	counter++;
}

void SysTick_Init ()
{
	SysTick->CTRL = 0x00;
	SysTick->LOAD = (100000000L/PISR_FREQUENCY_HZ)-1; //10000L; // <= 1us @ 4Mhz //100000000/PISR_FREQUENCY_HZ
	SysTick->VAL  = 0x00;
	SysTick->CTRL = SysTick_CTRL_CLKSOURCE_Msk | SysTick_CTRL_TICKINT_Msk | SysTick_CTRL_ENABLE_Msk;
	NVIC_EnableIRQ(SysTick_IRQn);
}
