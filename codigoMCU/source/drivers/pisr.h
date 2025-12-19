///***************************************************************************//**
//  @file     pisr.h
//  @brief    Periodic Interrupt (PISR) driver
//  @author   Nicolás Magliola
// ******************************************************************************/
//
//#ifndef _PISR_H_
//#define _PISR_H_
//
///*******************************************************************************
// * INCLUDE HEADER FILES
// ******************************************************************************/
//
//#include <stdbool.h>
//#include <stdint.h>
//
//
///*******************************************************************************
// * CONSTANT AND MACRO DEFINITIONS USING #DEFINE
// ******************************************************************************/
//
//#define PISR_FREQUENCY_HZ 100000U
//
//#define PISR_CANT        8
//
//
///*******************************************************************************
// * ENUMERATIONS AND STRUCTURES AND TYPEDEFS
// ******************************************************************************/
//
//typedef void (*pisr_callback_t) (void);
//
//typedef struct
//{
//	pisr_callback_t callback_ptr;
//	unsigned int period;
//} pisr_callback_type;
//
///*******************************************************************************
// * VARIABLE PROTOTYPES WITH GLOBAL SCOPE
// ******************************************************************************/
//
///*******************************************************************************
// * FUNCTION PROTOTYPES WITH GLOBAL SCOPE
// ******************************************************************************/
//
///**
// * @brief Register PISR callback
// * @param fun PISR function to be call periodically
// * @param period PISR period in ticks
// * @return Registration succeed
// */
//bool pisrRegister (pisr_callback_t fun, unsigned int period);
//
//void SysTick_Init (void);
//
//
///*******************************************************************************
// ******************************************************************************/
//
//#endif // _PISR_H_
