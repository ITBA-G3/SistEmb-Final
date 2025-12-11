///*******************************************************************************
//  @file     sdhc.c
//  @brief    Secure Digital Host Controller driver
//  @author   Grupo 3
// ******************************************************************************/
//
///*******************************************************************************
// * INCLUDE HEADER FILES
// ******************************************************************************/
#include "hardware.h"
#include "MK64F12.h"

#include "sdhc.h"
#include "gpio.h"
#include "board.h"
///*******************************************************************************
// * CONSTANT AND MACRO DEFINITIONS USING #DEFINE
// ******************************************************************************/
#define SDHC_D0_PIN			PORTNUM2PIN(PE, 1)
#define SDHC_D1_PIN			PORTNUM2PIN(PE, 0)
#define SDHC_D2_PIN			PORTNUM2PIN(PE, 5)
#define SDHC_D3_PIN			PORTNUM2PIN(PE, 4)
#define SDHC_CMD_PIN		PORTNUM2PIN(PE, 3)
#define SDHC_DCLK_PIN		PORTNUM2PIN(PE, 2)
#define SDHC_SWITCH_PIN		PORTNUM2PIN(PE, 6)

// SDHC declaring PCR configuration for each pin
#define SDHC_CMD_PCR		PORT_PCR_MUX(4)
#define SDHC_DCLK_PCR		PORT_PCR_MUX(4)
#define SDHC_D0_PCR			PORT_PCR_MUX(4)
#define SDHC_D1_PCR			PORT_PCR_MUX(4)
#define SDHC_D2_PCR			PORT_PCR_MUX(4)
#define SDHC_D3_PCR			PORT_PCR_MUX(4)

// SDHC possible values for the register fields
#define SDHC_RESPONSE_LENGTH_NONE	SDHC_XFERTYP_RSPTYP(0b00)
#define SDHC_RESPONSE_LENGTH_48		SDHC_XFERTYP_RSPTYP(0b10)
#define SDHC_RESPONSE_LENGTH_136	SDHC_XFERTYP_RSPTYP(0b01)
#define SDHC_RESPONSE_LENGTH_48BUSY	SDHC_XFERTYP_RSPTYP(0b11)

#define SDHC_COMMAND_CHECK_CCR		SDHC_XFERTYP_CCCEN(0b1)
#define SDHC_COMMAND_CHECK_INDEX	SDHC_XFERTYP_CICEN(0b1)

// SDHC flag shortcuts
#define SDHC_COMMAND_COMPLETED_FLAG		(SDHC_IRQSTAT_CC_MASK)
#define SDHC_TRANSFER_COMPLETED_FLAG	(SDHC_IRQSTAT_TC_MASK)
#define SDHC_CARD_DETECTED_FLAGS		(SDHC_IRQSTAT_CINS_MASK | SDHC_IRQSTAT_CRM_MASK)
#define SDHC_DATA_FLAG					(SDHC_IRQSTAT_BRR_MASK | SDHC_IRQSTAT_BWR_MASK)
#define SDHC_DATA_TIMEOUT_FLAG			(SDHC_IRQSTAT_DTOE_MASK)
#define SDHC_ERROR_FLAG					(																										 \
											SDHC_IRQSTAT_DMAE_MASK | SDHC_IRQSTAT_AC12E_MASK | SDHC_IRQSTAT_DEBE_MASK |  SDHC_IRQSTAT_DCE_MASK | \
											SDHC_IRQSTAT_CIE_MASK | SDHC_IRQSTAT_CEBE_MASK | SDHC_IRQSTAT_CCE_MASK |    \
											SDHC_IRQSTAT_CTOE_MASK														 \
										)

///*******************************************************************************
// * ENUMERATIONS AND STRUCTURES AND TYPEDEFS
// ******************************************************************************/
typedef struct {
	bool card_inserted;
	bool low_voltage_supported;
	bool suspend_supported;
	bool dma_supported;
	bool adma_supported;
	bool high_speed_supported;
	bool transfer_completed;
	bool is_available;
	uint16_t maxBlockSize;
	sdhc_error_t current_error;


	sdhc_data_t* current_data;
	sdhc_command_t*	current_command;

} sd_status_t;
///*******************************************************************************
// * STATIC VARIABLES AND CONST VARIABLES WITH FILE LEVEL SCOPE
// ******************************************************************************/
__ISR__ SDHC_IRQHandler(void);

static void SDHC_CardDetectedHandler(void);
static void SDHC_setCardDetectInterrupt(void);
static void SDHC_disableCardDetectInterrupt(void);
static void sdhcSetClock(uint32_t frequency);
static void	getSettingsByFrequency(uint32_t frequency, uint8_t* sdclks, uint8_t* dvs);
static uint32_t computeFrequency(uint8_t prescaler, uint8_t divisor);
static void SDHC_TransferErrorHandler(uint32_t status);
static void status_init(void);

static sd_status_t sdhc_status = {0};
///*******************************************************************************
// *******************************************************************************
//                        GLOBAL FUNCTION DEFINITIONS
// *******************************************************************************
// ******************************************************************************/

void sdhc_enable_clocks_and_pins(void)
{
	SYSMPU->CESR = 0; //disable memory protection unit
	// Initialization of GPIO peripheral to detect switch
	gpioMode(SDHC_SWITCH_PIN, INPUT_PULLUP);
	gpioIRQ(SDHC_SWITCH_PIN, GPIO_IRQ_MODE_BOTH_EDGES, SDHC_CardDetectedHandler);
    /* habilitar reloj al SDHC y al PORT E */
	SIM->SCGC3 = (SIM->SCGC3 & ~SIM_SCGC3_SDHC_MASK)  | SIM_SCGC3_SDHC(1);
	SIM->SCGC5 = (SIM->SCGC5 & ~SIM_SCGC5_PORTE_MASK) | SIM_SCGC5_PORTE(1);

	PORTE->PCR[PIN2NUM(SDHC_CMD_PIN)] = SDHC_CMD_PCR;
	PORTE->PCR[PIN2NUM(SDHC_DCLK_PIN)] = SDHC_DCLK_PCR;
	PORTE->PCR[PIN2NUM(SDHC_D0_PIN)] = SDHC_D0_PCR;
	PORTE->PCR[PIN2NUM(SDHC_D1_PIN)] = SDHC_D1_PCR;
	PORTE->PCR[PIN2NUM(SDHC_D2_PIN)] = SDHC_D2_PCR;
	PORTE->PCR[PIN2NUM(SDHC_D3_PIN)] = SDHC_D3_PCR;

    // Disable the automatically gating off of the peripheral's clock, hardware and other
//	SDHC->SYSCTL = (SDHC->SYSCTL & ~SDHC_SYSCTL_PEREN_MASK) | SDHC_SYSCTL_PEREN(1);
//	SDHC->SYSCTL = (SDHC->SYSCTL & ~SDHC_SYSCTL_HCKEN_MASK
//			) | SDHC_SYSCTL_HCKEN(1);
//	SDHC->SYSCTL = (SDHC->SYSCTL & ~SDHC_SYSCTL_IPGEN_MASK) | SDHC_SYSCTL_IPGEN(1);


	/* Disable all clock auto gated off feature because of DAT0 line logic(card buffer full status) can't be updated
	    correctly when clock auto gated off is enabled. */
	SDHC->SYSCTL |= (SDHC_SYSCTL_PEREN_MASK | SDHC_SYSCTL_HCKEN_MASK | SDHC_SYSCTL_IPGEN_MASK);

	// Disable the peripheral clocking, sets the divisor and prescaler for the new target frequency
	// and configures the new value for the time-out delay. Finally, enables the clock again.
	SDHC->SYSCTL = (SDHC->SYSCTL & ~SDHC_SYSCTL_DTOCV_MASK)   | SDHC_SYSCTL_DTOCV(0b1110);

	sdhcSetClock(400000U);

	// Disable interrupts and clear all flags
	SDHC->IRQSTATEN = 0;
	SDHC->IRQSIGEN = 0;
	SDHC->IRQSTAT = 0xFFFFFFFF;

	// Enable interrupts, signals and NVIC
	SDHC->IRQSTATEN = (
			SDHC_IRQSTATEN_CCSEN_MASK 		| SDHC_IRQSTATEN_TCSEN_MASK 	| SDHC_IRQSTATEN_BGESEN_MASK 	| SDHC_IRQSTATEN_DMAESEN_MASK 	|
			SDHC_IRQSTATEN_CTOESEN_MASK 	| SDHC_IRQSTATEN_CCESEN_MASK 	| SDHC_IRQSTATEN_CEBESEN_MASK	| SDHC_IRQSTATEN_CIESEN_MASK	|
			SDHC_IRQSTATEN_DTOESEN_MASK 	| SDHC_IRQSTATEN_DCESEN_MASK 	| SDHC_IRQSTATEN_DEBESEN_MASK 	| SDHC_IRQSTATEN_AC12ESEN_MASK

	);
	SDHC->IRQSIGEN = (
			SDHC_IRQSIGEN_CCIEN_MASK 	| SDHC_IRQSIGEN_TCIEN_MASK 		| SDHC_IRQSIGEN_BGEIEN_MASK 	| SDHC_IRQSIGEN_CTOEIEN_MASK 	|
			SDHC_IRQSIGEN_CCEIEN_MASK 	| SDHC_IRQSIGEN_CEBEIEN_MASK 	| SDHC_IRQSIGEN_CIEIEN_MASK 	| SDHC_IRQSIGEN_DTOEIEN_MASK 	|
			SDHC_IRQSIGEN_DCEIEN_MASK 	| SDHC_IRQSIGEN_DEBEIEN_MASK 	| SDHC_IRQSIGEN_AC12EIEN_MASK	| SDHC_IRQSIGEN_DMAEIEN_MASK
	);
	// Endianness + bus width = 1-bit fijo
	SDHC->PROCTL &= ~(SDHC_PROCTL_EMODE_MASK | SDHC_PROCTL_DTW_MASK);
	SDHC->PROCTL |=  SDHC_PROCTL_EMODE(2)     // 10b = little endian
	               | SDHC_PROCTL_DTW(0);      // 00b = 1-bit bus
	SDHC->PROCTL |= SDHC_PROCTL_CDSS_MASK | SDHC_PROCTL_CDTL_MASK;  // “card inserted” por soft
	SDHC->PROCTL &= ~SDHC_PROCTL_D3CD_MASK;
	NVIC_EnableIRQ(SDHC_IRQn);
	status_init();
}

void sdhc_reset(sdhc_reset_t reset_type)
{
	uint32_t mask = 0;
	uint32_t timeout = 0xFFFF;
	switch(reset_type)
	{
		case SDHC_RESET_DATA:
			mask |= SDHC_SYSCTL_RSTD_MASK;
			break;
		case SDHC_RESET_CMD:
			mask |= SDHC_SYSCTL_RSTC_MASK;
			break;
		case SDHC_RESET_ALL:
			mask |= SDHC_SYSCTL_RSTA_MASK;
			break;
	}
	SDHC->SYSCTL |= mask;
	while(timeout && (SDHC->SYSCTL & mask))
	{
		timeout--;
	}

	if (reset_type == SDHC_RESET_CMD)
	{
		SDHC->IRQSIGEN = SDHC->IRQSTATEN;
	}
}


bool sdhc_start_transfer(sdhc_command_t* command, sdhc_data_t* data)
{

	uint32_t	flags = 0;

	if(sdhc_status.is_available)
	{
		if (!(SDHC->PRSSTAT & SDHC_PRSSTAT_CDIHB_MASK) && !(SDHC->PRSSTAT & SDHC_PRSSTAT_CIHB_MASK))
		{
			sdhc_status.is_available = false;
			sdhc_status.transfer_completed = false;
			sdhc_status.current_error = SDHC_ERROR_OK;

			sdhc_status.current_command = command;
			sdhc_status.current_data    = data;

			if(command)
			{
				switch (command->responseType)
				{
					case SDHC_RESPONSE_TYPE_NONE:
						flags |= SDHC_RESPONSE_LENGTH_NONE;
						break;
					case SDHC_RESPONSE_TYPE_R1:
						flags |= (SDHC_RESPONSE_LENGTH_48 | SDHC_COMMAND_CHECK_CCR | SDHC_COMMAND_CHECK_INDEX);
						break;
					case SDHC_RESPONSE_TYPE_R1b:
						flags |= (SDHC_RESPONSE_LENGTH_48BUSY | SDHC_COMMAND_CHECK_CCR | SDHC_COMMAND_CHECK_INDEX);
						break;
					case SDHC_RESPONSE_TYPE_R2:
						flags |= (SDHC_RESPONSE_LENGTH_136 | SDHC_COMMAND_CHECK_CCR);
						break;
					case SDHC_RESPONSE_TYPE_R3:
						flags |= (SDHC_RESPONSE_LENGTH_48);
						break;
					case SDHC_RESPONSE_TYPE_R4:
						flags |= (SDHC_RESPONSE_LENGTH_48);
						break;
					case SDHC_RESPONSE_TYPE_R5:
						flags |= (SDHC_RESPONSE_LENGTH_48 | SDHC_COMMAND_CHECK_CCR | SDHC_COMMAND_CHECK_INDEX);
						break;
					case SDHC_RESPONSE_TYPE_R5b:
						flags |= (SDHC_RESPONSE_LENGTH_48BUSY | SDHC_COMMAND_CHECK_CCR | SDHC_COMMAND_CHECK_INDEX);
						break;
					case SDHC_RESPONSE_TYPE_R6:
						flags |= (SDHC_RESPONSE_LENGTH_48 | SDHC_COMMAND_CHECK_CCR | SDHC_COMMAND_CHECK_INDEX);
						break;
					case SDHC_RESPONSE_TYPE_R7:
						flags |= (SDHC_RESPONSE_LENGTH_48 | SDHC_COMMAND_CHECK_CCR | SDHC_COMMAND_CHECK_INDEX);
						break;
					default:
						break;
				}
				// Set the command type, index and argument
				flags |= SDHC_XFERTYP_CMDINX(command->index);
				flags |= SDHC_XFERTYP_CMDTYP(command->commandType);
				SDHC->CMDARG = command->argument;
			}

			if(data)
			{
				// WORD ALIGNMENT
				if (data->blockSize % sizeof(uint32_t) != 0U)
				{
					data->blockSize += sizeof(uint32_t) - (data->blockSize % sizeof(uint32_t));
				}

				// Set block size and block count
				SDHC->BLKATTR = (SDHC->BLKATTR & ~(SDHC_BLKATTR_BLKSIZE_MASK | SDHC_BLKATTR_BLKCNT_MASK)) | SDHC_BLKATTR_BLKCNT(data->blockCount)  | SDHC_BLKATTR_BLKSIZE(data->blockSize);

				// Sets the transferring mode selected by the user
				SDHC->IRQSTATEN = (SDHC->IRQSTATEN & ~SDHC_IRQSTATEN_BRRSEN_MASK) | SDHC_IRQSTATEN_BRRSEN(data->transferMode == SDHC_TRANSFER_MODE_CPU ? 0b1 : 0b0);
				SDHC->IRQSTATEN = (SDHC->IRQSTATEN & ~SDHC_IRQSTATEN_BWRSEN_MASK) | SDHC_IRQSTATEN_BWRSEN(data->transferMode == SDHC_TRANSFER_MODE_CPU ? 0b1 : 0b0);
				SDHC->IRQSTATEN = (SDHC->IRQSTATEN & ~SDHC_IRQSTATEN_DINTSEN_MASK) | SDHC_IRQSTATEN_DINTSEN(data->transferMode == SDHC_TRANSFER_MODE_CPU ? 0b0 : 0b1);
				SDHC->IRQSIGEN = (SDHC->IRQSIGEN & ~SDHC_IRQSIGEN_BRRIEN_MASK) | SDHC_IRQSIGEN_BRRIEN(data->transferMode == SDHC_TRANSFER_MODE_CPU ? 0b1 : 0b0);
				SDHC->IRQSIGEN = (SDHC->IRQSIGEN & ~SDHC_IRQSIGEN_BWRIEN_MASK) | SDHC_IRQSIGEN_BWRIEN(data->transferMode == SDHC_TRANSFER_MODE_CPU ? 0b1 : 0b0);
				SDHC->IRQSIGEN = (SDHC->IRQSIGEN & ~SDHC_IRQSIGEN_DINTIEN_MASK) | SDHC_IRQSIGEN_DINTIEN(data->transferMode == SDHC_TRANSFER_MODE_CPU ? 0b0 : 0b1);
				if (data->transferMode != SDHC_TRANSFER_MODE_CPU)
				{
					SDHC->PROCTL = (SDHC->PROCTL & ~SDHC_PROCTL_DMAS_MASK) | SDHC_PROCTL_DMAS(data->transferMode);
				}

				// Set the data present flag
				flags |= SDHC_XFERTYP_DPSEL_MASK;
				flags |= SDHC_XFERTYP_DTDSEL(data->readBuffer ? 0b1 : 0b0);
				flags |= SDHC_XFERTYP_MSBSEL(data->blockCount > 1 ? 0b1 : 0b0);
				flags |= SDHC_XFERTYP_AC12EN(data->blockCount > 1 ? 0b1 : 0b0);
				flags |= SDHC_XFERTYP_BCEN(data->blockCount > 1 ? 0b1 : 0b0);
				flags |= SDHC_XFERTYP_DMAEN(data->transferMode == SDHC_TRANSFER_MODE_CPU ? 0b0 : 0b1);
			}

			//todo: FALTA HACER LA PARTE DE DMA
		}
	}

//	base->BLKATTR = ((base->BLKATTR & ~(SDHC_BLKATTR_BLKSIZE_MASK | SDHC_BLKATTR_BLKCNT_MASK)) |
//	                     (SDHC_BLKATTR_BLKSIZE(config->dataBlockSize) | SDHC_BLKATTR_BLKCNT(config->dataBlockCount)));
//	    base->CMDARG  = config->commandArgument;
//	    base->XFERTYP = (((config->commandIndex << SDHC_XFERTYP_CMDINX_SHIFT) & SDHC_XFERTYP_CMDINX_MASK) |
//	                     (config->flags & (SDHC_XFERTYP_DMAEN_MASK | SDHC_XFERTYP_MSBSEL_MASK | SDHC_XFERTYP_DPSEL_MASK |
//	                                       SDHC_XFERTYP_CMDTYP_MASK | SDHC_XFERTYP_BCEN_MASK | SDHC_XFERTYP_CICEN_MASK |
//	                                       SDHC_XFERTYP_CCCEN_MASK | SDHC_XFERTYP_RSPTYP_MASK | SDHC_XFERTYP_DTDSEL_MASK |
//	                                       SDHC_XFERTYP_AC12EN_MASK)));

	if(data)
	{
		SDHC->BLKATTR = ((SDHC->BLKATTR & ~(SDHC_BLKATTR_BLKSIZE_MASK | SDHC_BLKATTR_BLKCNT_MASK)) |
				(SDHC_BLKATTR_BLKSIZE(data->blockSize) | SDHC_BLKATTR_BLKCNT(data->blockCount)));
	}
	else
	{
		SDHC->BLKATTR = ((SDHC->BLKATTR & ~(SDHC_BLKATTR_BLKSIZE_MASK | SDHC_BLKATTR_BLKCNT_MASK)) |
						(SDHC_BLKATTR_BLKSIZE(0x00U) | SDHC_BLKATTR_BLKCNT(0x00U)));
	}
	SDHC->CMDARG = command->argument;
	SDHC->XFERTYP = (((command->index << SDHC_XFERTYP_CMDINX_SHIFT) & SDHC_XFERTYP_CMDINX_MASK) |
					 (flags & (SDHC_XFERTYP_DMAEN_MASK | SDHC_XFERTYP_MSBSEL_MASK | SDHC_XFERTYP_DPSEL_MASK |
									   SDHC_XFERTYP_CMDTYP_MASK | SDHC_XFERTYP_BCEN_MASK | SDHC_XFERTYP_CICEN_MASK |
									   SDHC_XFERTYP_CCCEN_MASK | SDHC_XFERTYP_RSPTYP_MASK | SDHC_XFERTYP_DTDSEL_MASK |
									   SDHC_XFERTYP_AC12EN_MASK)));
	return true;
}

sdhc_error_t sdhc_transfer(sdhc_command_t* command, sdhc_data_t* data)
{
	sdhc_error_t error = SDHC_ERROR_OK;
	bool forceExit = false;
	SDHC->IRQSTAT = 0xFFFFFFFF; //intento de limpiar las flags
    SDHC->PROCTL = ((SDHC->PROCTL & ~SDHC_PROCTL_DTW_MASK) | SDHC_PROCTL_DTW(0b00));


	if (sdhc_start_transfer(command, data))
	{
		while (!forceExit && !sdhc_status.transfer_completed)
		{
			if (sdhc_status.current_error != SDHC_ERROR_OK)
			{
				error = sdhc_status.current_error;
				forceExit = true;
			}
		}
	}
	else
	{
		error = SDHC_ERROR_CMD_BUSY;
	}

	return error;
}

void sdhc_initialization_clocks(void)
{
	uint32_t timeout = 0xFFFFFF;
	SDHC->SYSCTL |= SDHC_SYSCTL_INITA_MASK;
	while(timeout && (SDHC->SYSCTL & SDHC_SYSCTL_INITA_MASK))
	{
		timeout--;
	}
}
///*******************************************************************************
// *******************************************************************************
//                        LOCAL FUNCTION DEFINITIONS
// *******************************************************************************
// ******************************************************************************/
static void SDHC_setCardDetectInterrupt(void)
{
	SDHC->IRQSIGEN |= SDHC_IRQSIGEN_CINSIEN_MASK;
}

static void SDHC_disableCardDetectInterrupt(void)
{
	SDHC->IRQSIGEN &= ~SDHC_IRQSIGEN_CINSIEN_MASK;
}
static void SDHC_CardDetectedHandler(void)
{
	sdhc_status.card_inserted = SDHC->IRQSTAT & SDHC_IRQSTAT_CINS_MASK;
	if(gpioRead(SDHC_SWITCH_PIN))
	{
		sdhc_status.card_inserted = true;
	}
	else
	{
		sdhc_status.card_inserted = false;
	}
}

void sdhcSetClock(uint32_t frequency)
{
	uint8_t sdcklfs, dvs;
	getSettingsByFrequency(frequency, &sdcklfs, &dvs);
	SDHC->SYSCTL = (SDHC->SYSCTL & ~SDHC_SYSCTL_SDCLKEN_MASK) | SDHC_SYSCTL_SDCLKEN(0);
	SDHC->SYSCTL = (SDHC->SYSCTL & ~SDHC_SYSCTL_SDCLKFS_MASK) | SDHC_SYSCTL_SDCLKFS(sdcklfs);
	SDHC->SYSCTL = (SDHC->SYSCTL & ~SDHC_SYSCTL_DVS_MASK)     | SDHC_SYSCTL_DVS(dvs);
	SDHC->SYSCTL = (SDHC->SYSCTL & ~SDHC_SYSCTL_SDCLKEN_MASK) | SDHC_SYSCTL_SDCLKEN(1);
}

static void	getSettingsByFrequency(uint32_t frequency, uint8_t* sdclks, uint8_t* dvs)
{
	uint32_t currentFrequency = 0;
	uint32_t currentError = 0;
	uint32_t bestFrequency = 0;
	uint32_t bestError = 0;
	uint16_t prescaler;
	uint16_t divisor;
	for (prescaler = 0x0002 ; prescaler <= 0x0100 ; prescaler = prescaler << 1)
	{
		for (divisor = 1 ; divisor <= 16 ; divisor++)
		{
			currentFrequency = computeFrequency(prescaler, divisor);
			currentError = frequency > currentFrequency ? frequency - currentFrequency : currentFrequency - frequency;
			if ((bestFrequency == 0) || (bestError > currentError))
			{
				bestFrequency = currentFrequency;
				bestError = currentError;
				*sdclks = prescaler >> 1;
				*dvs = divisor - 1;
			}
		}
	}
}

static uint32_t computeFrequency(uint8_t prescaler, uint8_t divisor)
{
	return SDHC_CLOCK_FREQUENCY / (prescaler * divisor);
}

/* Soft reset y configuración básica del SDHC (setea clocks en SYSCTL para identificación).
   Ajustá SDCLKFS/DVS si querés valores específicos (RM muestra la fórmula). */
void sdhc_soft_reset_all(void)
{
    // Reset y limpiar IRQ
    SDHC->SYSCTL |= SDHC_SYSCTL_RSTA_MASK;
    while (SDHC->SYSCTL & SDHC_SYSCTL_RSTA_MASK) {}
    SDHC->IRQSTAT = 0xFFFFFFFFu;
}

static void SDHC_TransferErrorHandler(uint32_t status)
{
	sdhc_error_t error = SDHC_ERROR_OK;

	sdhc_status.is_available = true;

	if (status & SDHC_IRQSTAT_DMAE_MASK)
	{
		error |= SDHC_ERROR_DMA;
	}
	if (status & SDHC_IRQSTAT_AC12E_MASK)
	{
		error |= SDHC_ERROR_AUTO_CMD12;
	}
	if (status & SDHC_IRQSTAT_DEBE_MASK)
	{
		error |= SDHC_ERROR_DATA_END;
	}
	if (status & SDHC_IRQSTAT_DCE_MASK)
	{
		error |= SDHC_ERROR_DATA_CRC;
	}
	if (status & SDHC_IRQSTAT_DTOE_MASK)
	{
		error |= SDHC_ERROR_DATA_TIMEOUT;
	}
	if (status & SDHC_IRQSTAT_CIE_MASK)
	{
		error |= SDHC_ERROR_CMD_INDEX;
	}
	if (status & SDHC_IRQSTAT_CEBE_MASK)
	{
		error |= SDHC_ERROR_CMD_END;
	}
	if (status & SDHC_IRQSTAT_CCE_MASK)
	{
		error |= SDHC_ERROR_CMD_CRC;
	}
	if (status & SDHC_IRQSTAT_CTOE_MASK)
	{
		error |= SDHC_ERROR_CMD_TIMEOUT;
	}

	sdhc_status.current_error = error;
	SDHC->IRQSTAT = 0xFFFFFFFF;
//	if (context.onTransferError)
//	{
//		context.onTransferError(error);
//	}
}

static void SDHC_CommandCompletedHandler(uint32_t status)
{
	// The command transfer has been completed, fetch the response if there is one
	if (sdhc_status.current_command->responseType != SDHC_RESPONSE_TYPE_NONE)
	{
		sdhc_status.current_command->response[0] = SDHC->CMDRSP[0];
		sdhc_status.current_command->response[1] = SDHC->CMDRSP[1];
		sdhc_status.current_command->response[2] = SDHC->CMDRSP[2];
		sdhc_status.current_command->response[3] = SDHC->CMDRSP[3] & 0xFFFFFF;
	}

	if (sdhc_status.current_data == NULL)
	{
		// Notify or raise the transfer completed flag
		sdhc_status.is_available = true;
		sdhc_status.transfer_completed = true;
	}
}

static void SDHC_TransferCompletedHandler(uint32_t status)
{
	// Notify or raise the transfer completed flag
	sdhc_status.is_available = true;
	sdhc_status.transfer_completed = true;
}

static void status_init(void)
{
	sdhc_status.adma_supported = ( SDHC->HTCAPBLT & SDHC_HTCAPBLT_ADMAS_MASK ) ? true : false;
	sdhc_status.card_inserted = gpioRead(SDHC_SWITCH_PIN) == HIGH ? true : false;
	sdhc_status.low_voltage_supported = ( SDHC->HTCAPBLT & SDHC_HTCAPBLT_VS33_MASK ) ? true : false;
	sdhc_status.suspend_supported = ( SDHC->HTCAPBLT & SDHC_HTCAPBLT_SRS_MASK ) ? true : false;
	sdhc_status.dma_supported = ( SDHC->HTCAPBLT & SDHC_HTCAPBLT_DMAS_MASK ) ? true : false;
	sdhc_status.high_speed_supported = ( SDHC->HTCAPBLT & SDHC_HTCAPBLT_HSS_MASK ) ? true : false;
	sdhc_status.transfer_completed = false;
	sdhc_status.is_available = true;
	sdhc_status.current_error = SDHC_ERROR_OK;
	switch ((SDHC->HTCAPBLT & SDHC_HTCAPBLT_MBL_MASK) >> SDHC_HTCAPBLT_MBL_SHIFT)
	{
		case 0b00:
			sdhc_status.maxBlockSize = 512;
			break;
		case 0b01:
			sdhc_status.maxBlockSize = 1024;
			break;
		case 0b10:
			sdhc_status.maxBlockSize = 2048;
			break;
		case 0b11:
			sdhc_status.maxBlockSize = 4096;
			break;
	}

}

/*******************************************************************************
 *******************************************************************************
					    INTERRUPT SERVICE ROUTINES
 *******************************************************************************
 ******************************************************************************/


__ISR__ SDHC_IRQHandler(void)
{
	// Get the current status of all interrupt status flags
	uint32_t status = SDHC->IRQSTAT;

	// Dispatches each flag detected
	if (status & SDHC_ERROR_FLAG)
	{
		SDHC_TransferErrorHandler(status & SDHC_ERROR_FLAG);
	}
	else if ((status & SDHC_DATA_TIMEOUT_FLAG) && !(status & SDHC_TRANSFER_COMPLETED_FLAG))
	{
		SDHC_TransferErrorHandler(status & SDHC_ERROR_FLAG);
	}
	else
	{
//		if (status & SDHC_DATA_FLAG)
//		{
//			SDHC_DataHandler(status & SDHC_DATA_FLAG);
//		}
		if (status & SDHC_COMMAND_COMPLETED_FLAG)
		{
			SDHC_CommandCompletedHandler(status & SDHC_COMMAND_COMPLETED_FLAG);
		}
		if (status & SDHC_TRANSFER_COMPLETED_FLAG)
		{
			SDHC_TransferCompletedHandler(status & SDHC_TRANSFER_COMPLETED_FLAG);
		}
	}

	// Clear all flags raised when entered the service routine
	SDHC->IRQSTAT = status;
}
