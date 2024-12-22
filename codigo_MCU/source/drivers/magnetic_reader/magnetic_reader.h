/*
 * magnetic_reader.h
 *
 *  Created on: Sep 2, 2024
 *      Author: ediazguzman
 */

#ifndef MAGNETIC_READER_H_
#define MAGNETIC_READER_H_

/*******************************************************************************
 * INCLUDE HEADER FILES
 ******************************************************************************/

#include <stdbool.h>
#include <stdint.h>
#include "../../rtos/uCOSIII/src/uCOS-III/Source/os.h"
#include "../../gpio.h"
#include "../../board.h"

/*******************************************************************************
 * CONSTANT AND MACRO DEFINITIONS USING #DEFINE
 ******************************************************************************/

#define MAX_CARD_NUMBERS  54
#define MAX_PRIMARY	19
#define DIGIT_SIZE 5       

/*******************************************************************************
 * FUNCTION PROTOTYPES WITH GLOBAL SCOPE
 ******************************************************************************/

void magnetic_reader_init (OS_Q *queue);

/**
 * @brief Get the status of the driver
 * 
 * @return true if the card data is ready
 * @return false 
 */
bool getCardStatus (void);

/**
 * @brief Get the card data. Do not invoke until getCardStatus returns true
 * 
 * @return uin64_t primary account. Returns 0 if card status is false 
 */
uint64_t getPrimaryAccount (void);

uint64_t getAdditionalData (void);

#endif /* MAGNETIC_READER_H_ */
