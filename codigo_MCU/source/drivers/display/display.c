/***************************************************************************/ /**
   @file     display.c
   @brief    Display driver
   @author   Grupo 3
  ******************************************************************************/

#include "display.h"
#include "gpio.h"
#include "../../tick.h"
#include "display_board.h"
#include <stdbool.h>

/*** CONSTANT AND MACRO DEFINITIONS ***/
#define BIT2LED(p, n) ((p[aux] >> n) & 0x01)
#define READY 1
/**************************************/

/************ PROTOTYPES **************/
static void show(void); // show data in display
static void blink(void);
static void brightness(void); // brightness control
static void rotateWord(void);

/**************************************/

/********** STATIC VARIABLES **********/
static pin_t const led_pin[TOTAL_LEDS] = LEDS_BOARD; // pins for LEDs
//{PIN_SSEG_A,PIN_SSEG_B,PIN_SSEG_C,PIN_SSEG_D,PIN_SSEG_E,PIN_SSEG_F,PIN_SSEG_G,PIN_SSEG_DP};
static pin_t const mux_pin[TOTAL_MUX] = MUX_BOARD; // pins for selectors
//{PIN_SSEG_MUX_0,PIN_SSEG_MUX_1};
static uint8_t const dig_addr[TOTAL_DISP][TOTAL_MUX] = ADDR_MUX; // output for display selection
static uint8_t select_symbol[TOTAL_SIMB] = SIMB;				 // symbol selection
static uint8_t dot_control[TOTAL_DISP];							 // dot control
static uint8_t data_dig[TOTAL_DISP];							 // data to load
static uint8_t wordToRotate[MAX_DIGITS + 7] = {0};				 // word to rotate (more than 4 digits that will "rotate" through displays)

static uint8_t display[TOTAL_DISP]; // indicate if a digit is ready for blinking
static uint8_t dig2blink;			// used for blinking

static uint32_t intensity = MAX_INT; // value for display intensity

static uint8_t timesToRotate = 1;
// flags for interruptions
static uint8_t show_flag;
static uint8_t blink_flag;
static uint8_t rotateFlag;

/**************************************/

/********** GLOBAL FUNCTION ***********/
void dispInit(void)
{
	uint8_t i;

	for (i = 0; i < TOTAL_LEDS; i++) // led pins configuration (PCR level)
	{
		gpioMode(led_pin[i], OUTPUT);
		gpioWrite(led_pin[i], LED_ON);
	}

	for (i = 0; i < TOTAL_MUX; i++) // mux pins configuration (PCR level)
	{ 
		gpioMode(mux_pin[i], OUTPUT);
		gpioWrite(mux_pin[i], LOW);
	}

	for (i = 0; i < TOTAL_DISP; i++)
	{
		data_dig[i] = select_symbol[32]; // set an "8" in each display for initialization
		dot_control[i] = 1;		// turns off all dots
		display[i] = READY;				// digit is ready for blinking
	}

	show_flag = 0;
	blink_flag = 0;

	tickAdd(show, 2); // 10 = 1ms
	// tickAdd(blink, 1)
	tickAdd(brightness, 1);	//
	tickAdd(rotateWord, 500); // 5000 = 500ms
}

/*********** DIGIT CONTROL ************/
void writeDig(uint8_t simb, uint8_t dig)
{
	if (simb < TOTAL_SIMB && dig < TOTAL_DISP)
	{
		data_dig[dig] = select_symbol[simb]; // sets symbol "simb" to show in the display number "dig"
	}
}

void writeWord(uint8_t *data)
{
	uint8_t i;
	for (i = 0; i < TOTAL_DISP; i++)
	{
		data_dig[i] = select_symbol[data[i]]; // sets the displays with data from a received array (data)
	}
}

void writeDot(uint8_t state, uint8_t dig)
{
	if (dig < TOTAL_DISP)
	{
		dot_control[dig] = state; // Turns on or off (state, LED_ON or LED_OFF) the dot of a digit (dig)
	}
}

/*********** BLINK CONTROL ************/
void blinkDig(uint8_t dig)
{
	if (!blink_flag)
	{
		uint8_t i;
		for (i = 0; i < TOTAL_DISP; i++)
		{
			display[i] = READY; // prepares digits for blinking
		}
	}
	if (dig < TOTAL_DISP)
	{
		dig2blink = dig; // selected digit to blink
		blink_flag = 1;	 // activates the flag to blink
	}
}

void stopBlink(void)
{
	blink_flag = 0; // disable the flag to blink
	uint8_t i;
	for (i = 0; i < TOTAL_DISP; i++)
	{
		display[i] = READY; // prepares digits for blinking
	}
}

void setBrightness(uint8_t value)
{
	if (0 <= intensity && intensity <= MAX_INT)
	{
		intensity = value;
	}
}
/**************************************/

/*********** LOCAL FUNCTION ***********/
static void show(void)
{
	static uint8_t aux = 0, i = 0;

	show_flag = 1;							 // activates the flag to show
	gpioWrite(mux_pin[0], dig_addr[aux][0]); // configures display selector
	gpioWrite(mux_pin[1], dig_addr[aux][1]);
	if (display[aux] != 0)
	{
		for (i = 0; i < TOTAL_LEDS - 1; i++)
		{
			gpioWrite(led_pin[i], BIT2LED(data_dig, i)); // turns on LEDs
		}
	}
	else
	{
		for (i = 0; i < TOTAL_LEDS - 1; i++)
		{
			gpioWrite(led_pin[i], LED_OFF); // turns off lEDSs
		}
	}
	gpioWrite(led_pin[7], dot_control[aux]);
	aux = (aux < TOTAL_DISP) ? aux + 1 : 0; // update display number to be modified
}

static void blink(void)
{
	static uint32_t time = BLINK_TIME; // display blinks every BLINK_TIME (ms)
	static uint8_t i;

	if (blink_flag != 0)
	{
		if (time > 0)
		{
			time--;
		}
		else
		{
			time = BLINK_TIME; // reset timer counter
			for (i = 0; i < TOTAL_DISP; i++)
			{
				if (i == dig2blink)
				{
					display[dig2blink] = !display[dig2blink]; // blink to selected digit
				}
				else
				{
					display[i] = READY; // prepares digits for blinking
				}
			}
		}
	}
	else
	{
		for (i = 0; i < TOTAL_DISP; i++)
		{
			display[i] = READY; // prepares digits for blinking
		}
	}
}

static void brightness(void)
{
	static uint32_t aux = 0;
	static uint8_t i;
	if (intensity < MAX_INT && show_flag != 0)
	{
		if (intensity <= aux)
		{
			for (i = 0; i < TOTAL_LEDS; i++)
			{
				gpioWrite(led_pin[i], LED_OFF); // turns off the display every few miliseconds, more brightness, more miliseconds
			}
			show_flag = 0;
			aux = 0;
		}
		else
		{
			aux++;
		}
	}
}

void beginRotation(uint8_t *word, uint8_t times) // recibe un arreglo de 8 elementos, con lo que quiero hacer girar
{
	uint8_t i = 0;
	for (i = 0; i < 3; i++) // complete the first 3 digits with zeros so that the displays will turn off
	{
		wordToRotate[i] = 0;
	}
	for (i = i; i < MAX_DIGITS + 3; i++) // complete the next 8 digits of word with data
	{
		wordToRotate[i] = select_symbol[word[i - 3]];
	}
	for (i = i; i < MAX_DIGITS + 7; i++) // the last 4 digits are 0 to improve visual experience
	{
		wordToRotate[i] = 0;
	}
	rotateFlag = true;
	timesToRotate = times;
}

void stopRotation(void)
{
	rotateFlag = false;
}

void rotateWord(void) // TODO Creo que es más fácil hacerlo con buffers circulares
{
	static uint8_t index = 0; // variable that traverses the wordToRotate array.
	static uint8_t i;
	static uint8_t times = 0;
	if (rotateFlag == true)
	{
		for (i = 0; i < TOTAL_DISP; i++)
		{
			data_dig[i] = wordToRotate[i + index]; // Charge on displays the current rotation
		}
		if (index < MAX_DIGITS + 3)
		{
			index++;
		}
		else
		{
			index = 0;
			times++;
			if(times == timesToRotate)
			{
				rotateFlag = false;
				times = 0;
			}
		}
	}
}

uint8_t * getSymbArr(void)
{
	uint8_t * symbPtr = &select_symbol[0];
	return symbPtr;
}

void clearDisplay(void){
	writeDig(32, 0);
	writeDig(32, 1);
	writeDig(32, 2);
	writeDig(32, 3);
	writeDot(LED_OFF, 0);
	writeDot(LED_OFF, 1);
	writeDot(LED_OFF, 2);
	writeDot(LED_OFF, 3);
}

void writeInfoLed(uint8_t led)
{
	switch (led)
	{
	case 1:
		gpioWrite(PIN_INFO_LED_0, HIGH);
		gpioWrite(PIN_INFO_LED_1, LOW);
		break;
	case 2:
		gpioWrite(PIN_INFO_LED_1, HIGH);
		gpioWrite(PIN_INFO_LED_0, LOW);
		break;
	case 3:
		gpioWrite(PIN_INFO_LED_1, HIGH);
		gpioWrite(PIN_INFO_LED_1, HIGH);
		break;
	default:
		break;
	}
}



