/***************************************************************************//**
  @file     encoder.c
  @brief    encoder driver functions
  @author   Grupo 3
 ******************************************************************************/

/*******************************************************************************
 * INCLUDE HEADER FILES
 ******************************************************************************/
#include "encoder.h"
#include <stdbool.h>
#include "gpio.h"
#include "board.h"
#include "../SDK/CMSIS/MK64F12.h"
#include "../../tick.h"
/*******************************************************************************
 * CONSTANT AND MACRO DEFINITIONS USING #DEFINE
 ******************************************************************************/

#define PORT_INTERRUPT_FALLING 1010 //1010 ISF flag and Interrupt on falling-edge.

/*******************************************************************************
 * STATIC VARIABLES DEFINITIONS
 ******************************************************************************/

static uint8_t encAction = ENC_STILL;
static uint8_t switchState = HIGH;

OS_Q* QSwitch = 0;
OS_Q* QEncoder = 0;

/*******************************************************************************
 * FUNCTION PROTOTYPES FOR PRIVATE FUNCTIONS WITH FILE LEVEL SCOPE
 ******************************************************************************/

void switchCallback (void);

/*******************************************************************************
 *******************************************************************************
                        GLOBAL FUNCTION DEFINITIONS
 *******************************************************************************
 ******************************************************************************/
//RCHA = 18, RCHB = 19, RSWITCH = 20 (PORTB)
void encoderInit(OS_Q* queueENC) //tiene que inicializar los pines y las interrupciones por flanco de las mismas.
{
	PORTB->PCR[20] = 0x0;
	PORTB->PCR[19] = 0x0;
	PORTB->PCR[18] = 0x0;
	gpioMode(PIN_ENCODER_SWITCH,INPUT);
	gpioMode(PIN_ENCODER_B,INPUT);
	gpioMode(PIN_ENCODER_A,INPUT);

	QEncoder = queueENC;

	tickAdd(encoderCallback,1);
	tickAdd(switchCallback,5);
}

/*
 * La idea es que esto actualiza encAction con el ultimo giro, "es trabajo de la app"
 * volver esa variable a STILL una vez que se usó el valor leído, que simboliza que el
 * encoder está quieto. No tiene sentido que la interrupción vuelva el valor a STILL.
 */
void encoderCallback(void)
{
	OS_ERR os_err;
	static uint8_t firstEdge = 0; //En esta variable se almacena el canal(A=1,B=2) que tuvo el primer flanco descendente
	static uint8_t statusChannelA, statusChannelB;
	statusChannelA = gpioRead(PIN_ENCODER_A);
	statusChannelB = gpioRead(PIN_ENCODER_B);
	if(firstEdge == 0)
	{
		if(statusChannelA == LOW && statusChannelB == HIGH)
		{
			firstEdge = 1;
			encAction = ENC_RIGHT;
		}
		else if(statusChannelA == HIGH && statusChannelB == LOW)
		{
			firstEdge = 2;
			encAction = ENC_LEFT;
		}
	}
	else
	{
		if(statusChannelA == HIGH && statusChannelB == HIGH)
		{
			firstEdge = 0;
			OSQPost(QEncoder, &encAction, sizeof(encAction), OS_OPT_POST_FIFO, &os_err);
		}
	}
}

void switchCallback (void)
{
	OS_ERR os_err;
	static uint8_t lastSWstate;

	lastSWstate = switchState;
	switchState = gpioRead(PIN_ENCODER_SWITCH);
	
	if ((lastSWstate == LOW) && (switchState == HIGH))
	{
		encAction = SW_FLANK;
		OSQPost(QEncoder, &encAction, sizeof(switchState), OS_OPT_POST_FIFO, &os_err);
	}
}

uint8_t getEncDir (void)
{
	return encAction;
}

void resetEncDir (void)
{
	encAction = ENC_STILL;
}

uint8_t getSwitchState (void)
{
	return switchState;
}

void buzzerStart(void)
{
	gpioWrite(PIN_BUZZER, HIGH);
}
void buzzerStop(void)
{
	gpioWrite(PIN_BUZZER, LOW);
}
//void resetSwitchState (void)
//{
//	switchState = SW_RELEASED; //0 = released
//}

/*******************************************************************************
 *******************************************************************************
                        LOCAL FUNCTION DEFINITIONS
 *******************************************************************************
 ******************************************************************************/


/*******************************************************************************
 ******************************************************************************/
