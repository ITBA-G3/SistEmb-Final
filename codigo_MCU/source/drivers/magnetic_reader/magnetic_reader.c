/*
 * magnetic_reader.c
 *
 *  Created on: Sep 2, 2024
 *      Author: ediazguzman
 */

/*******************************************************************************
 * INCLUDE HEADER FILES
 ******************************************************************************/

#include "magnetic_reader.h"

/*******************************************************************************
 * CONSTANT AND MACRO DEFINITIONS USING #DEFINE
 ******************************************************************************/

// P b3 b2 b1 b0
#define SS 		0b01011
#define ES 		0b11111
#define FS 		0b01101
#define MR_0	0b10000
#define MR_1	0b00001
#define MR_2	0b00010
#define MR_3	0b10011
#define MR_4	0b00100
#define MR_5	0b10101
#define MR_6	0b10110
#define MR_7	0b00111
#define MR_8	0b01000
#define MR_9	0b11001


/*******************************************************************************
 * ENUMERATIONS AND STRUCTURES AND TYPEDEFS
 ******************************************************************************/

typedef struct {
    bool status;
    uint8_t size;
    uint64_t primary_account;
    uint64_t additional_data;

	// Analog2Digital
	uint16_t bit;
    uint8_t raw_data[MAX_CARD_NUMBERS*DIGIT_SIZE];
    uint8_t data[MAX_PRIMARY];
} card_data_t;

/*******************************************************************************
 * STATIC VARIABLES DEFINITIONS
 ******************************************************************************/

static bool enable = false;
static card_data_t card;

/*******************************************************************************
 * FUNCTION PROTOTYPES FOR PRIVATE FUNCTIONS WITH FILE LEVEL SCOPE
 ******************************************************************************/

void updateEnable(void);
void storeData(void);
bool organizeRawData(void);
void reconstructCardNumber(void);
void clearBuffer(void);

OS_Q* QMagReader = 0;

/*******************************************************************************
 *******************************************************************************
                        GLOBAL FUNCTION DEFINITIONS
 *******************************************************************************
 ******************************************************************************/

void magnetic_reader_init (OS_Q* queue) 
{
    gpioMode(PIN_LECTOR_ENABLE, INPUT);
    gpioMode(PIN_LECTOR_CLK, INPUT);
    gpioMode(PIN_LECTOR_DATA, INPUT);
    
    gpioIRQ(PIN_LECTOR_ENABLE, GPIO_IRQ_MODE_BOTH_EDGES, updateEnable);
    gpioIRQ(PIN_LECTOR_CLK, GPIO_IRQ_MODE_FALLING_EDGE, storeData);

	QMagReader = queue;

    card.bit=0;
    card.status=false;
}

bool getCardStatus (void)
{
    // if (validateData() && organizeRawData() && reconstructCardNumber())   // validateData: not neccessary for now
    if (organizeRawData())
    {
        reconstructCardNumber();
	    card.status = true;
    }
    else
	    card.status = false;
  
    return card.status;
}

uint64_t getPrimaryAccount (void)
{
    if (card.status)
    {
        card.status=false;
        return card.primary_account;
    }

    // Return ERROR
    return 0;
}

/*******************************************************************************
 *******************************************************************************
                        LOCAL FUNCTION DEFINITIONS
 *******************************************************************************
 ******************************************************************************/

/**
 * @brief Enable Callback. Updates enabled state.
 * 
 */
void updateEnable(void)
{
	OS_ERR os_err;

    if(gpioRead(PIN_LECTOR_ENABLE)==LOW)
	{
        enable = true;
	}
    else
    {
    	enable = false;
		card.bit = 0;
		OSQPost(QMagReader, &enable, sizeof(bool), OS_OPT_POST_FIFO, &os_err);
    }
}

/**
* @brief CLK Callback. Stores the received bit each clock
*
*
*/
void storeData(void)
{
	if (enable)
	{
		card.raw_data[card.bit] = !gpioRead(PIN_LECTOR_DATA);
	    card.bit++;
	}
}

/**
 * @brief Organizes the raw data into the card struct
 *
 * @return false if error ocurred
 */
bool organizeRawData(void)      
{
	 uint16_t start_index = 0;
	 unsigned int digit;
	 card.size = 0;

	 // Look for SS
	 for (int i=0; i<MAX_CARD_NUMBERS*DIGIT_SIZE; i++)
	 {
		 digit=0;
		 for (int j=0; j < DIGIT_SIZE; j++)
		 {
			 digit |= (card.raw_data[i+j] << j);
		 }
		 if (digit==SS)
		 {
			 start_index=i;
			 card.size++;		// For validation purposes
			 break;
		 }
	 }

	 if (card.size==0)
	 {
		 // Invalid card
		 return false;
	 }

	 // Build card data
	 for (int i=0; i<MAX_CARD_NUMBERS && digit!=ES; i++)	
	 {
		 digit=0;
		 for (int j=0; j<DIGIT_SIZE; j++)
		 {
			 digit |= (card.raw_data[(i+1)*DIGIT_SIZE+j+start_index] << j);		// Remember card.raw_data[start_index] == SS
		 }
		 switch(digit)
		 {
		 	 case MR_0:
		 	 	 card.data[i] = 0;
		 	 	 break;
		 	 case MR_1:
		 	 	 card.data[i] = 1;
		 	 	 break;
		 	 case MR_2:
		 	 	 card.data[i] = 2;
		 	 	 break;
		 	 case MR_3:
		 	 	 card.data[i] = 3;
		 	 	 break;
		 	 case MR_4:
		 	 	 card.data[i] = 4;
		 	 	 break;
		 	 case MR_5:
		 	 	 card.data[i] = 5;
		 	 	 break;
		 	 case MR_6:
		 	 	 card.data[i] = 6;
		 	 	 break;
		 	 case MR_7:
		 	 	 card.data[i] = 7;
		 	 	 break;
		 	 case MR_8:
		 	 	 card.data[i] = 8;
		 	 	 break;
		 	 case MR_9:
		 	 	 card.data[i] = 9;
		 	 	 break;
		 	 case FS:
		 		 card.data[i] = 0b01101;
		 		 break;
		 	 case ES:
		 		 card.size = (i-1);	
		 		 break;
		 	 default:
		 		 // Invalid card
		 		 return false;
		 }
	 }
	 if (digit!=ES)
	 {
		 // Invalid card
		 return false;
	 }

    clearBuffer();

	return true;
}

void clearBuffer(void)
{
    for (int i=0; i<MAX_CARD_NUMBERS*DIGIT_SIZE; i++)
        card.raw_data[i]=0;
}

void reconstructCardNumber(void)
{
    bool primary = true;
    card.primary_account = 0;
    card.additional_data = 0;
    for (int i=0; i<card.size; i++)
    {
        if (card.data[i]==FS)
            primary = false;
        else if (primary)
            card.primary_account = card.primary_account*10 + card.data[i];
        else
            card.additional_data = card.additional_data*10 + card.data[i];
    }
}
