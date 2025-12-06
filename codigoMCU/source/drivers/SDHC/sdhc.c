///*******************************************************************************
//  @file     sdhc.c
//  @brief    Secure Digital Host Controller driver
//  @author   Grupo 3
// ******************************************************************************/
//
///*******************************************************************************
// * INCLUDE HEADER FILES
// ******************************************************************************/
//
//#include "hardware.h"
//#include "MK64F12.h"
//#include "sdhc.h"
//#include <stdio.h>
//#include "gpio.h"
//
///*******************************************************************************
// * CONSTANT AND MACRO DEFINITIONS USING #DEFINE
// ******************************************************************************/
//
//// SDHC pins
//#define SDHC_D0_PIN			PORTNUM2PIN(PE, 1)
//#define SDHC_D1_PIN			PORTNUM2PIN(PE, 0)
//#define SDHC_D2_PIN			PORTNUM2PIN(PE, 5)
//#define SDHC_D3_PIN			PORTNUM2PIN(PE, 4)
//#define SDHC_CMD_PIN		PORTNUM2PIN(PE, 3)
//#define SDHC_DCLK_PIN		PORTNUM2PIN(PE, 2)
//#define SDHC_SWITCH_PIN		PORTNUM2PIN(PE, 6)
//
//// SDHC internal parameters
//#define SDHC_MAXIMUM_BLOCK_SIZE		4096
//#define SDHC_RESET_TIMEOUT			100000
//#define SDHC_CLOCK_FREQUENCY		(96000000U)
//
//// SDHC possible register values
//#define SDHC_RESPONSE_LENGTH_NONE	SDHC_XFERTYP_RSPTYP(0b00)
//#define SDHC_RESPONSE_LENGTH_48		SDHC_XFERTYP_RSPTYP(0b10)
//#define SDHC_RESPONSE_LENGTH_136	SDHC_XFERTYP_RSPTYP(0b01)
//#define SDHC_RESPONSE_LENGTH_48BUSY	SDHC_XFERTYP_RSPTYP(0b11)
//
//#define SDHC_COMMAND_CHECK_CCR		SDHC_XFERTYP_CCCEN(0b1)
//#define SDHC_COMMAND_CHECK_INDEX	SDHC_XFERTYP_CICEN(0b1)
//
//// SDHC flags
//#define SDHC_COMMAND_COMPLETED_FLAG		(SDHC_IRQSTAT_CC_MASK)
//#define SDHC_TRANSFER_COMPLETED_FLAG	(SDHC_IRQSTAT_TC_MASK)
//#define SDHC_CARD_DETECTED_FLAGS		(SDHC_IRQSTAT_CINS_MASK | SDHC_IRQSTAT_CRM_MASK)
//#define SDHC_DATA_FLAG					(SDHC_IRQSTAT_BRR_MASK | SDHC_IRQSTAT_BWR_MASK)
//#define SDHC_DATA_TIMEOUT_FLAG			(SDHC_IRQSTAT_DTOE_MASK)
//#define SDHC_ERROR_FLAG					(																										 \
//											SDHC_IRQSTAT_DMAE_MASK | SDHC_IRQSTAT_AC12E_MASK | SDHC_IRQSTAT_DEBE_MASK |  SDHC_IRQSTAT_DCE_MASK | \
//											SDHC_IRQSTAT_CIE_MASK | SDHC_IRQSTAT_CEBE_MASK | SDHC_IRQSTAT_CCE_MASK |    \
//											SDHC_IRQSTAT_CTOE_MASK														 \
//										)
//
///*******************************************************************************
// * ENUMERATIONS AND STRUCTURES AND TYPEDEFS
// ******************************************************************************/
//
//// SDHC driver context variables
//typedef struct {
//	// Buffers
//	sdhc_adma1_descriptor_t	ADMA1Table[SDHC_ADMA1_MAX_DESCRIPTORS];
//	sdhc_adma2_descriptor_t	ADMA2Table[SDHC_ADMA2_MAX_DESCRIPTORS];
//
//	// Peripheral capabilities
//	bool			lowVoltageSupported;	// If the peripheral support 3.3V voltage
//	bool			suspendResumeSupported;	// If supports suspend/resume functionalities
//	bool			dmaSupported;			// If supports using DMA
//	bool			highSpeedSupported;		// If supports clocking frequency higher than 25MHz
//	bool			admaSupported;			// If supports using Advanced DMA
//	uint16_t		maxBlockSize;			// Maximum size of block supported by the peripheral
//	uint32_t		readWatermarkLevel;		// Read watermark level
//	uint32_t		writeWatermarkLevel;	// Write watermark level
//
//	// Current status
//	sdhc_command_t*	currentCommand;			// Current command being transfered
//	sdhc_data_t*	currentData;			// Current data being transfered
//	size_t			transferedWords;		// Amount of transfered bytes
//	sdhc_error_t	currentError;			// Current error status of the driver
//
//	// Callback
//	sdhc_callback_t	onCardInserted;			// Callback to be called when card is inserted
//	sdhc_callback_t	onCardRemoved;			// Callback to be called when card is removed
//	sdhc_callback_t	onTransferCompleted;	// Callback to be called when a transfer is completed
//	sdhc_error_callback_t onTransferError;	// Callback to be called when a transfer error occurs
//
//	// Flags
//	bool			alreadyInit;			// true: driver was already initialized
//	bool			cardStatus;				// true: is inserted, false: is removed
//	bool			transferCompleted;		// true: last transfer was completed
//	bool			available;				// true: if driver is available or not busy
//} sdhc_context_t;
//
///*******************************************************************************
// * STATIC VARIABLES AND CONST VARIABLES WITH FILE LEVEL SCOPE
// ******************************************************************************/
//
//static sdhc_context_t	context;
//
//
///*******************************************************************************
// *******************************************************************************
//                        GLOBAL FUNCTION DEFINITIONS
// *******************************************************************************
// ******************************************************************************/
//
//void sdhcInit(sdhc_config_t config)
//{
//	if (!context.alreadyInit)
//	{
//		// Disables the memory protection unit
//		SYSMPU->CESR = 0;
//
//		// Initialization of GPIO peripheral to detect switch
//		gpioMode(SDHC_SWITCH_PIN, INPUT_PULLDOWN);
//		gpioIRQ(SDHC_SWITCH_PIN, GPIO_IRQ_MODE_INTERRUPT_BOTH_EDGES, SDHC_CardDetectedHandler);
//
//		// Configuration of the clock gating for both SDHC, PORTE peripherals
//		SIM->SCGC3 = (SIM->SCGC3 & ~SIM_SCGC3_SDHC_MASK)  | SIM_SCGC3_SDHC(1);
//		SIM->SCGC5 = (SIM->SCGC5 & ~SIM_SCGC5_PORTE_MASK) | SIM_SCGC5_PORTE(1);
//
//		// Setting the corresponding value to each pin PCR register
//		PORTE->PCR[PIN2NUM(SDHC_CMD_PIN)] = SDHC_CMD_PCR;
//		PORTE->PCR[PIN2NUM(SDHC_DCLK_PIN)] = SDHC_DCLK_PCR;
//		PORTE->PCR[PIN2NUM(SDHC_D0_PIN)] = SDHC_D0_PCR;
//		PORTE->PCR[PIN2NUM(SDHC_D1_PIN)] = SDHC_D1_PCR;
//		PORTE->PCR[PIN2NUM(SDHC_D2_PIN)] = SDHC_D2_PCR;
//		PORTE->PCR[PIN2NUM(SDHC_D3_PIN)] = SDHC_D3_PCR;
//
//		// Reset the SDHC peripheral
//		sdhcReset(SDHC_RESET_ALL | SDHC_RESET_CMD | SDHC_RESET_DATA);
//
//		// Set the watermark
//		context.writeWatermarkLevel = config.writeWatermarkLevel;
//		context.readWatermarkLevel = config.readWatermarkLevel;
//		SDHC->WML = (SDHC->WML & ~SDHC_WML_WRWML_MASK) | SDHC_WML_WRWML(config.writeWatermarkLevel);
//		SDHC->WML = (SDHC->WML & ~SDHC_WML_RDWML_MASK) | SDHC_WML_RDWML(config.readWatermarkLevel);
//
//		// Disable the automatically gating off of the peripheral's clock, hardware and other
//		SDHC->SYSCTL = (SDHC->SYSCTL & ~SDHC_SYSCTL_PEREN_MASK) | SDHC_SYSCTL_PEREN(1);
//		SDHC->SYSCTL = (SDHC->SYSCTL & ~SDHC_SYSCTL_HCKEN_MASK) | SDHC_SYSCTL_HCKEN(1);
//		SDHC->SYSCTL = (SDHC->SYSCTL & ~SDHC_SYSCTL_IPGEN_MASK) | SDHC_SYSCTL_IPGEN(1);
//
//		// Disable the peripheral clocking, sets the divisor and prescaler for the new target frequency
//		// and configures the new value for the time-out delay. Finally, enables the clock again.
//		SDHC->SYSCTL = (SDHC->SYSCTL & ~SDHC_SYSCTL_DTOCV_MASK)   | SDHC_SYSCTL_DTOCV(0b1110);
//		sdhcSetClock(config.frequency);
//
//		// Disable interrupts and clear all flags
//		SDHC->IRQSTATEN = 0;
//		SDHC->IRQSIGEN = 0;
//		SDHC->IRQSTAT = 0xFFFFFFFF;
//
//		// Enable interrupts, signals and NVIC
//		SDHC->IRQSTATEN = (
//				SDHC_IRQSTATEN_CCSEN_MASK 		| SDHC_IRQSTATEN_TCSEN_MASK 	| SDHC_IRQSTATEN_BGESEN_MASK 	| SDHC_IRQSTATEN_DMAESEN_MASK 	|
//			 	SDHC_IRQSTATEN_CTOESEN_MASK 	| SDHC_IRQSTATEN_CCESEN_MASK 	| SDHC_IRQSTATEN_CEBESEN_MASK	| SDHC_IRQSTATEN_CIESEN_MASK	|
//				SDHC_IRQSTATEN_DTOESEN_MASK 	| SDHC_IRQSTATEN_DCESEN_MASK 	| SDHC_IRQSTATEN_DEBESEN_MASK 	| SDHC_IRQSTATEN_AC12ESEN_MASK
//
//		);
//		SDHC->IRQSIGEN = (
//				SDHC_IRQSIGEN_CCIEN_MASK 	| SDHC_IRQSIGEN_TCIEN_MASK 		| SDHC_IRQSIGEN_BGEIEN_MASK 	| SDHC_IRQSIGEN_CTOEIEN_MASK 	|
//				SDHC_IRQSIGEN_CCEIEN_MASK 	| SDHC_IRQSIGEN_CEBEIEN_MASK 	| SDHC_IRQSIGEN_CIEIEN_MASK 	| SDHC_IRQSIGEN_DTOEIEN_MASK 	|
//				SDHC_IRQSIGEN_DCEIEN_MASK 	| SDHC_IRQSIGEN_DEBEIEN_MASK 	| SDHC_IRQSIGEN_AC12EIEN_MASK	| SDHC_IRQSIGEN_DMAEIEN_MASK
//		);
//		NVIC_EnableIRQ(SDHC_IRQn);
//
//		// Initialization successful
//		contextInit();
//	}
//}
