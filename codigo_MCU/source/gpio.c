#include "gpio.h"
#include "../SDK/CMSIS/MK64F12.h"
#include <stdbool.h>
#include "board.h"

typedef struct
{
  pinIrqFun_t irqFun;
  pin_t pin;
} pin_callback_t;

static PORT_Type* puertos[] = {PORTA,PORTB,PORTC,PORTD,PORTE};
static GPIO_Type* gpios[] = {GPIOA,GPIOB,GPIOC,GPIOD,GPIOE};

static pin_callback_t IRQ_Callbacks[5][IRQ_CANT] = {0};

static uint8_t lastPin;

void gpioMode (pin_t pin, uint8_t mode) //tengo que poder modificar el PCR y el PDDR del pin que me dan
{
	PORT_Type* puerto 	= puertos[PIN2PORT(pin)];
	GPIO_Type* gpio	 	= gpios[PIN2PORT(pin)];
	puerto->PCR[PIN2NUM(pin)] |= PORT_PCR_MUX(0x1U); // Tengo que "colocar" al pin en la alternativa GPIO
	switch (mode)
	{
		case INPUT:
		{
			gpio->PDDR &= ~(0x1<<PIN2NUM(pin)); //coloco un 0 en el bit que quiero que sea una entrada
			break;
		}
		case OUTPUT:
		{
			gpio->PDDR |= (0x1<<PIN2NUM(pin)); //coloco un 1 en el pin que quiero que sea una salida
			break;
		}
		case INPUT_PULLUP:
		{
			gpio->PDDR &= ~(0x1<<PIN2NUM(pin));
			puerto->PCR[PIN2NUM(pin)] |= (PORT_PCR_PS(0x1U) | PORT_PCR_PE(0x1U));//Activo el PE y seteo PS en 1 para que sea PULLUP
			break;
		}
		case INPUT_PULLDOWN:
		{
			gpio->PDDR &= ~(0x1<<PIN2NUM(pin));
			puerto->PCR[PIN2NUM(pin)] |= PORT_PCR_PE(1U);//Activo el PE y seteo PS en 0 para que sea PULLDOWN
			puerto->PCR[PIN2NUM(pin)] &= PORT_PCR_PS(0U);
			break;
		}
	}
}

void gpioWrite (pin_t pin, bool value)
{
	GPIO_Type* gpio	 	= gpios[PIN2PORT(pin)];
	if (value)
	{
		gpio->PSOR 	= 0x1 << PIN2NUM(pin);
	}
	else
	{
		gpio->PCOR 	= 0x1 << PIN2NUM(pin);
	}
}

void gpioToggle (pin_t pin)
{
	GPIO_Type* gpio	 	= gpios[PIN2PORT(pin)];
	gpio->PTOR 			= 0x1 << PIN2NUM(pin);
}

bool gpioRead (pin_t pin)
{
	GPIO_Type* gpio	 	= gpios[PIN2PORT(pin)];

	return ((gpio->PDIR & (1U<<PIN2NUM(pin))) != 0)? HIGH:LOW;
}


/**
 * @brief Configures how the pin reacts when an IRQ event ocurrs
 * @param pin the pin whose IRQ mode you wish to set (according PORTNUM2PIN)
 * @param irqMode disable, risingEdge, fallingEdge or bothEdges
 * @param irqFun function to call on pin event
 * @return Registration succeed
 */
bool gpioIRQ (pin_t pin, uint8_t irqMode, pinIrqFun_t irqFun)
{
	PORT_Type* puerto 	= puertos[PIN2PORT(pin)];
	GPIO_Type* gpio	 	= gpios[PIN2PORT(pin)];
	static bool nvicFlags[5] = {0}; //son 5 porque hay 5 puertos
	static uint8_t counter = 0;
	if (counter < IRQ_CANT)
	{
		if((gpio->PDDR & (0x1<<PIN2NUM(pin))) != 0x00) //coloco un 1 en el pin que quiero que sea una salida
		{
			return false;
		}
		else if(irqMode >= GPIO_IRQ_MODE_DISABLE && irqMode < GPIO_IRQ_CANT_MODES)
		{
			puerto->PCR[PIN2NUM(pin)] |= PORT_PCR_IRQC(irqMode);
			IRQ_Callbacks[PIN2PORT(pin)][counter].irqFun = irqFun;
			IRQ_Callbacks[PIN2PORT(pin)][counter].pin = PIN2NUM(pin);
			counter++;
			if(nvicFlags[PIN2PORT(pin)] == false)
			{
				NVIC_EnableIRQ(PORTA_IRQn + PIN2PORT(pin));
				switch (PIN2PORT(pin))
				{
					case PA:
					{
						SIM->SCGC5 |= SIM_SCGC5_PORTA_MASK;
					}
					case PB:
					{
						SIM->SCGC5 |= SIM_SCGC5_PORTB_MASK;
					}
					case PC:
					{
						SIM->SCGC5 |= SIM_SCGC5_PORTC_MASK;
					}
					case PD:
					{
						SIM->SCGC5 |= SIM_SCGC5_PORTD_MASK;
					}
					case PE:
					{
						SIM->SCGC5 |= SIM_SCGC5_PORTE_MASK;
					}
				}
				nvicFlags[PIN2PORT(pin)] = true;
			}
		}
	}
	return true;
}

void PORTA_IRQHandler(void)
{
	gpioWrite(PIN_DISR, HIGH);
	for (int i=0; i<IRQ_CANT;i++)
	{
		if (IRQ_Callbacks[PA][i].irqFun != 0 && (PORTA->PCR[IRQ_Callbacks[PA][i].pin] & PORT_PCR_ISF_MASK))
		{
			PORTA->PCR[IRQ_Callbacks[PA][i].pin] |= PORT_PCR_ISF_MASK;
			IRQ_Callbacks[PA][i].irqFun();
		}
	}
	gpioWrite(PIN_DISR, LOW);
}

void PORTB_IRQHandler(void)
{
	gpioWrite(PIN_DISR, HIGH);
	for (int i=0; i<IRQ_CANT; i++)
	{
		if ((IRQ_Callbacks[PB][i].irqFun != 0) && ((PORTB->PCR[IRQ_Callbacks[PB][i].pin]) & PORT_PCR_ISF_MASK))
		{
				PORTB->PCR[IRQ_Callbacks[PB][i].pin] |= PORT_PCR_ISF_MASK;
				IRQ_Callbacks[PB][i].irqFun();
		}
	}
	gpioWrite(PIN_DISR, LOW);
}

void PORTC_IRQHandler(void)
{
	gpioWrite(PIN_DISR, HIGH);
	for (int i=0; i<IRQ_CANT;i++)
	{
		if (IRQ_Callbacks[PC][i].irqFun != 0 && (PORTC->PCR[IRQ_Callbacks[PC][i].pin] & PORT_PCR_ISF_MASK))
		{
			PORTC->PCR[IRQ_Callbacks[PC][i].pin] |= PORT_PCR_ISF_MASK;
			IRQ_Callbacks[PC][i].irqFun();
		}
	}
	gpioWrite(PIN_DISR, LOW);
}

void PORTD_IRQHandler(void)
{
	gpioWrite(PIN_DISR, HIGH);
	for (int i=0; i<IRQ_CANT;i++)
	{
		if (IRQ_Callbacks[PD][i].irqFun != 0 && (PORTD->PCR[IRQ_Callbacks[PD][i].pin] & PORT_PCR_ISF_MASK))
		{
			PORTD->PCR[IRQ_Callbacks[PD][i].pin] |= PORT_PCR_ISF_MASK;
			IRQ_Callbacks[PD][i].irqFun();
		}
	}
	gpioWrite(PIN_DISR, LOW);
}

void PORTE_IRQHandler(void)
{
	gpioWrite(PIN_DISR, HIGH);
	for (int i=0; i<IRQ_CANT;i++)
	{
		if (IRQ_Callbacks[PE][i].irqFun != 0 && (PORTE->PCR[IRQ_Callbacks[PE][i].pin] & PORT_PCR_ISF_MASK))
		{
			PORTE->PCR[IRQ_Callbacks[PE][i].pin] |= PORT_PCR_ISF_MASK;
			IRQ_Callbacks[PE][i].irqFun();
		}
	}
	gpioWrite(PIN_DISR, LOW);
}
