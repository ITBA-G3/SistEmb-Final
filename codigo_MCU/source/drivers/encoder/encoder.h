/***************************************************************************//**
  @file     encoder.h
  @brief    Mechanic Encoder Driver
  @author   Grupo 3
 ******************************************************************************/

#ifndef ENCODER_H
#define ENCODER_H

/*******************************************************************************
 * INCLUDE HEADER FILES
 ******************************************************************************/

#include <stdint.h>
#include "../../rtos/uCOSIII/src/uCOS-III/Source/os.h"

/*******************************************************************************
 * CONSTANT AND MACRO DEFINITIONS USING #DEFINE
 ******************************************************************************/

/*******************************************************************************
 * ENUMERATIONS AND STRUCTURES AND TYPEDEFS
 ******************************************************************************/

enum {ENC_STILL, ENC_LEFT, ENC_RIGHT, SW_FLANK};

/*******************************************************************************
 * VARIABLE PROTOTYPES WITH GLOBAL SCOPE
 ******************************************************************************/


/*******************************************************************************
 * FUNCTION PROTOTYPES WITH GLOBAL SCOPE
 ******************************************************************************/

/**
 * @brief Inits the encoder pins.
 * @param void
 */
void encoderInit(OS_Q* queueENC);

/**
 * @brief Refresh the encTurnDir variable with the last turn direction of the encoder.
 * @param void
 */
void encoderCallback (void);

/**
 * @brief (Getter) Returns in which direction the encoder has been turned (ENC_LEFT, ENC_RIGHT, ENC_STILL)
 */
uint8_t getEncDir (void);
/**
 * @brief (Setter) Resets the status of the encoder direction once it has been used (sets it to ENC_STILL)
 */
void resetEncDir(void);
//void resetSwitchState (void);
/**
 * @brief (Getter) Returns the status of the switch pin (SW_PRESSED, SW_RELEASED)
 */
uint8_t getSwitchState (void);

void buzzerStart(void);
void buzzerStop(void);
/*******************************************************************************
 ******************************************************************************/
#endif // ENCODER_H
