/***************************************************************************/ /**
   @file     main.c
   @brief    MP3 Player main (SDHC minimal access - polling)
   @author   Grupo 3 (corregido)
  ******************************************************************************/

#include <stdint.h>
#include <stddef.h>
#include <string.h>
#include "MK64F12.h"
#include "pin_mux.h"
#include "gpio.h"
#include "board.h"
/*******************************************************************************
 * CONSTANT AND MACRO DEFINITIONS
 ******************************************************************************/
#define SD_BLOCK_SIZE 512

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

// SDHC internal parameters
#define SDHC_MAXIMUM_BLOCK_SIZE		4096
#define SDHC_RESET_TIMEOUT			100000
#define SDHC_CLOCK_FREQUENCY		(96000000U)
/*******************************************************************************
 * FUNCTION PROTOTYPES
 ******************************************************************************/
void sdhc_enable_clocks_and_pins(void);
void sdhc_soft_reset_all(void);
int sdhc_send_command_polling(uint8_t cmd_index, uint32_t arg, uint32_t *resp, size_t resp_words);
int sdhc_write_single_block_polling(uint32_t block_addr, const uint8_t *data);
int sdhc_read_single_block_polling(uint32_t block_addr, uint8_t *out);
void sdhcInitializationClocks(void);

//cosas nuevas
void sdhcSetClock(uint32_t frequency);
static uint32_t computeFrequency(uint8_t prescaler, uint8_t divisor);
static void	getSettingsByFrequency(uint32_t frequency, uint8_t* sdclks, uint8_t* dvs);
static void SDHC_CardDetectedHandler(void);

typedef enum {
	SDHC_TRANSFER_MODE_CPU,		// Data transfer will be executed by the CPU host
	SDHC_TRANSFER_MODE_ADMA1,	// Data transfer will be executed by the advanced DMA controller v1
	SDHC_TRANSFER_MODE_ADMA2	// Data transfer will be executed by the advanced DMA controller v2
} sdhc_transfer_mode_t;

typedef struct {
	uint32_t				blockCount;		// Amount of blocks to be sent or received
	uint32_t				blockSize;		// Size in bytes of each block transfered
	uint32_t*				writeBuffer;	// Buffer with write data, used only when writing, else should be NULL
	uint32_t*				readBuffer;		// Buffer for the read data, used only when reading, else should be NULL
	sdhc_transfer_mode_t	transferMode;	// Data transfer mode
} sdhc_data_t;

typedef enum {
	SDHC_COMMAND_TYPE_NORMAL,
	SDHC_COMMAND_TYPE_SUSPEND,
	SDHC_COMMAND_TYPE_RESUME,
	SDHC_COMMAND_TYPE_ABORT
} sdhc_command_type_t;

typedef enum {
	SDHC_RESPONSE_TYPE_NONE,
	SDHC_RESPONSE_TYPE_R1,
	SDHC_RESPONSE_TYPE_R1b,
	SDHC_RESPONSE_TYPE_R2,
	SDHC_RESPONSE_TYPE_R3,
	SDHC_RESPONSE_TYPE_R4,
	SDHC_RESPONSE_TYPE_R5,
	SDHC_RESPONSE_TYPE_R5b,
	SDHC_RESPONSE_TYPE_R6,
	SDHC_RESPONSE_TYPE_R7
} sdhc_response_type_t;

typedef struct {
	uint8_t					index;			// Index of the command
	uint32_t				argument;		// Argument of the command
	sdhc_command_type_t		commandType;	// Type of command
	sdhc_response_type_t	responseType;	// Type of response expected
	uint32_t				response[4];	// Response placeholder
} sdhc_command_t;

typedef enum {
	// Specific errors, driver raises an individual flag for each one
	SDHC_ERROR_OK			= 0x00000000,
	SDHC_ERROR_DMA 			= 0x00000001,
	SDHC_ERROR_AUTO_CMD12 	= 0x00000002,
	SDHC_ERROR_DATA_END		= 0x00000004,
	SDHC_ERROR_DATA_CRC		= 0x00000008,
	SDHC_ERROR_DATA_TIMEOUT	= 0x00000010,
	SDHC_ERROR_CMD_INDEX	= 0x00000020,
	SDHC_ERROR_CMD_END		= 0x00000040,
	SDHC_ERROR_CMD_CRC		= 0x00000080,
	SDHC_ERROR_CMD_TIMEOUT	= 0x00000100,
	SDHC_ERROR_CMD_BUSY		= 0x00000200,

	// Some errors can be grouped
	SDHC_ERROR_DATA			= (SDHC_ERROR_DATA_END | SDHC_ERROR_DATA_CRC | SDHC_ERROR_DATA_TIMEOUT),
	SDHC_ERROR_CMD			= (SDHC_ERROR_CMD_INDEX | SDHC_ERROR_CMD_END | SDHC_ERROR_CMD_CRC | SDHC_ERROR_CMD_TIMEOUT | SDHC_ERROR_CMD_BUSY)
} sdhc_error_t;

bool sdhcStartTransfer(sdhc_command_t* command, sdhc_data_t* data);
sdhc_error_t sdhcTransfer(sdhc_command_t* command, sdhc_data_t* data);


/*******************************************************************************
 * GLOBAL DATA
 ******************************************************************************/
static uint8_t write_block[SD_BLOCK_SIZE];
static uint8_t read_block[SD_BLOCK_SIZE];

/*******************************************************************************
 * App_Init - inicialización mínima y test de write/read en tarjeta SD
 ******************************************************************************/
void App_Init(void)
{
    /* 1) Clocks y pines para SDHC */
//    BOARD_InitSDHCPins();
    sdhc_enable_clocks_and_pins();

    /* 2) Soft reset del controlador y configurar reloj de identificación (~400 kHz) */
    sdhc_soft_reset_all();

    /* 3) Secuencia SD init (muy básica): CMD0, CMD8, loop CMD55+ACMD41 */
    uint32_t resp[4];
    int rc;

    sdhcInitializationClocks();

    /* CMD0: GO_IDLE_STATE */
//    rc = sdhc_send_command_polling(0, 0x00000000u, NULL, 0);
//    if (rc != 0) { for (;;) { __asm__("bkpt #0"); } }
    sdhc_command_t command;
    sdhc_data_t data;
	bool success;
	success = false;
	command.index = 0;
	command.argument = 0;
	command.commandType = SDHC_COMMAND_TYPE_NORMAL;
	command.responseType = SDHC_RESPONSE_TYPE_NONE;
	if (sdhcTransfer(&command, NULL) == 0x00)
	{
		success = true;
	}


    /* CMD8: SEND_IF_COND (verifica voltajes, arg = 0x1AA) */
    memset(resp, 0, sizeof(resp));
    rc = sdhc_send_command_polling(8, 0x000001AAu, resp, 1);
    /* Si falla, la tarjeta puede ser antigua; para PoC seguimos de todas formas. */

    /* CMD55/ACMD41 loop */
    const uint32_t acmd41_arg = (1UL << 30); /* HCS = 1 */
    int timeout = 1000;
    for (;;) {
        rc = sdhc_send_command_polling(55, 0x0, resp, 1);
        if (rc != 0) { for (;;) { __asm__("bkpt #0"); } }

        rc = sdhc_send_command_polling(41, acmd41_arg, resp, 1); /* R3 (OCR) */
        if (rc != 0) { for (;;) { __asm__("bkpt #0"); } }

        if (resp[0] & (1U << 31)) break; /* Card Power Up Status bit */

        /* small delay */
        for (volatile int d = 0; d < 100000; ++d) __asm__("nop");
        if (--timeout <= 0) { for (;;) { __asm__("bkpt #0"); } }
    }

    /* CMD16: set block length to 512 (para SDSC); será ignorado en SDHC/SDXC */
    (void) sdhc_send_command_polling(16, SD_BLOCK_SIZE, resp, 1);

    /* 4) Preparar bloque de prueba */
    for (int i = 0; i < SD_BLOCK_SIZE; ++i) write_block[i] = (uint8_t)(i & 0xFF);
    const uint32_t test_block = 8; /* no uses block 0 si tienes MBR importante */

    /* 5) Escribir bloque */
    rc = sdhc_write_single_block_polling(test_block, write_block);
    if (rc != 0) { for (;;) { __asm__("bkpt #0"); } }

    /* 6) Leer bloque */
    memset(read_block, 0, sizeof(read_block));
    rc = sdhc_read_single_block_polling(test_block, read_block);
    if (rc != 0) { for (;;) { __asm__("bkpt #0"); } }

    /* 7) Comparar */
    if (memcmp(write_block, read_block, SD_BLOCK_SIZE) == 0) {
        /* éxito */
        for (;;) { __asm__("bkpt #0"); }
    } else {
        /* fallo */
        for (;;) { __asm__("bkpt #0"); }
    }
}

/*******************************************************************************
 * App_Run - loop principal (vacío para este PoC)
 ******************************************************************************/
void App_Run(void)
{
    /* vacio */
}

/*******************************************************************************
 * LOCAL FUNCTION DEFINITIONS
 ******************************************************************************/

/* Habilita clocks a SDHC y configura PCR de PTE0..PTE5 para ALT4 (SDHC).
   Uso de bits literales para pull/drive para evitar dependencias de nombres concretos. */
void sdhc_enable_clocks_and_pins(void)
{

	// Initialization of GPIO peripheral to detect switch
	gpioMode(SDHC_SWITCH_PIN, INPUT_PULLDOWN);
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

    /* small settle delay */
    for (volatile int d = 0; d < 1000; ++d) __asm__("nop");

    // Disable the automatically gating off of the peripheral's clock, hardware and other
	SDHC->SYSCTL = (SDHC->SYSCTL & ~SDHC_SYSCTL_PEREN_MASK) | SDHC_SYSCTL_PEREN(1);
	SDHC->SYSCTL = (SDHC->SYSCTL & ~SDHC_SYSCTL_HCKEN_MASK) | SDHC_SYSCTL_HCKEN(1);
	SDHC->SYSCTL = (SDHC->SYSCTL & ~SDHC_SYSCTL_IPGEN_MASK) | SDHC_SYSCTL_IPGEN(1);

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
	NVIC_EnableIRQ(SDHC_IRQn);
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



/* Envío de comando por polling (espera CC o timeout). resp puede ser NULL si resp_words == 0 */
/* Mejor implementación según RM: arma XFERTYP con checks y maneja errores de comando */
int sdhc_send_command_polling(uint8_t cmd_index, uint32_t arg,
                              uint32_t *resp, size_t resp_words)
{
    /* 1) Esperar que el controlador no esté ocupado con otro comando (CIHB == 0) */
    int wait = 1000000;
    while ((SDHC->PRSSTAT & SDHC_PRSSTAT_CIHB_MASK) && --wait) { /* spin */ }
    if (wait == 0) return -99; /* timeout esperando CIHB clear */

    /* 2) Escribir argumento */
    SDHC->CMDARG = arg;

    /* 3) Construir XFERTYP = wCmd según manual (bits: CMDINX, RSPTYP, DPSEL, CICEN, CCCEN, etc.) */
    uint32_t wCmd = 0;

    /* CMD index: bits CMDINX_SHIFT..CMDINX (use SDK shift) */
    wCmd |= ((uint32_t)cmd_index & 0x3Fu) << SDHC_XFERTYP_CMDINX_SHIFT;

    /* Response type (RSPTYP): 0=no resp, 1=48-bit, 2=48-bit busy, 3=136-bit (R2).
       Mapeo: si resp_words==0 -> 0; ==1 -> R1/48; ==4 -> R2/136 */
    if (resp_words == 0) {
        wCmd |= (0u << SDHC_XFERTYP_RSPTYP_SHIFT);
    } else if (resp_words == 1) {
        wCmd |= (1u << SDHC_XFERTYP_RSPTYP_SHIFT);
    } else if (resp_words == 4) {
        wCmd |= (3u << SDHC_XFERTYP_RSPTYP_SHIFT);
    } else {
        /* Si te surge otro caso, asigna 48-bit por defecto */
        wCmd |= (1u << SDHC_XFERTYP_RSPTYP_SHIFT);
    }

    /* Typical command sanity checks:
       - Habilitar Command Index Check (CICEN) y Command CRC Check (CCCEN) para respuestas
         que llevan CRC (R1/R2/R5...). No aplicar para response types que no usan CRC (R3 OCR).
       - Activar CICEN/CCCEN según el tipo de respuesta. Esto sigue la recomendación del RM. */
    /* R3 (OCR) has no CRC; R1 and R2 do. */
    if (resp_words == 1 || resp_words == 4) {
        /* Enable command index and CRC checks */
#ifdef SDHC_XFERTYP_CICEN_MASK
        wCmd |= SDHC_XFERTYP_CICEN_MASK;
#endif
#ifdef SDHC_XFERTYP_CCCEN_MASK
        wCmd |= SDHC_XFERTYP_CCCEN_MASK;
#endif
    }

    /* DPSEL (Data Present Select): normalmente lo marca la rutina de transferencia (read/write)
       - Aquí no lo forzamos para comandos sin data. Si necesitás mandar commands con data
         desde aquí, marcá DPSEL (SDHC_XFERTYP_DPSEL_MASK). */

    /* Si usaras internal DMA: setear el bit ADMA en XFERTYP (no usado aquí) */
    /* Multi-block flags (MSBSEL, BCEN, AC12EN) se configuran por la función que inicia la transferencia. */

    /* 4) Limpiar flags de comando en IRQSTAT (CC + command errors) escribiendo 1's */
    uint32_t cmd_error_mask = 0;
#ifdef SDHC_IRQSTAT_CTOE_MASK
    cmd_error_mask |= SDHC_IRQSTAT_CTOE_MASK;
#endif
#ifdef SDHC_IRQSTAT_CCE_MASK
    cmd_error_mask |= SDHC_IRQSTAT_CCE_MASK;
#endif
#ifdef SDHC_IRQSTAT_CEB_MASK
    cmd_error_mask |= SDHC_IRQSTAT_CEB_MASK;
#endif
#ifdef SDHC_IRQSTAT_CCRC_MASK
    cmd_error_mask |= SDHC_IRQSTAT_CCRC_MASK;
#endif
    /* Clear CC and any command error bits before issuing */
    SDHC->IRQSTAT = SDHC_IRQSTAT_CC_MASK | cmd_error_mask;

    /* 5) Issue command: write XFERTYP (wCmd) */
    SDHC->XFERTYP = wCmd;

    /* 6) Wait for Command Complete (CC) or command errors (with timeout) */
    int loops = 1000000;
    for (; loops > 0; --loops) {
        uint32_t st = SDHC->IRQSTAT;
        if (st & SDHC_IRQSTAT_CC_MASK) {
            /* Command Complete received */
            break;
        }
        /* If a command error occurred, handle it early */
#ifdef SDHC_IRQSTAT_CTOE_MASK
        if (st & SDHC_IRQSTAT_CTOE_MASK) {
            SDHC->IRQSTAT = SDHC_IRQSTAT_CTOE_MASK; /* clear */
            return -2; /* Command timeout error */
        }
#endif
#ifdef SDHC_IRQSTAT_CCE_MASK
        if (st & SDHC_IRQSTAT_CCE_MASK) {
            SDHC->IRQSTAT = SDHC_IRQSTAT_CCE_MASK;
            return -3; /* Command CRC error */
        }
#endif
#ifdef SDHC_IRQSTAT_CEB_MASK
        if (st & SDHC_IRQSTAT_CEB_MASK) {
            SDHC->IRQSTAT = SDHC_IRQSTAT_CEB_MASK;
            return -4; /* Command End Bit error */
        }
#endif
    }
    if (loops == 0) return -5; /* timeout waiting CC */

    /* 7) On CC: read IRQSTAT and check errors one last time */
    uint32_t final_stat = SDHC->IRQSTAT;
    /* If any command error bits are set, clear and return error */
    if (final_stat & cmd_error_mask) {
        SDHC->IRQSTAT = cmd_error_mask; /* clear those errors */
        return -6;
    }

    /* Clear CC bit (write 1) */
    SDHC->IRQSTAT = SDHC_IRQSTAT_CC_MASK;

    /* 8) Read response fields if requested */
    if (resp != NULL) {
        if (resp_words == 1) {
            resp[0] = SDHC->CMDRSP[0];
        } else if (resp_words == 4) {
            resp[0] = SDHC->CMDRSP[0];
            resp[1] = SDHC->CMDRSP[1];
            resp[2] = SDHC->CMDRSP[2];
            resp[3] = SDHC->CMDRSP[3];
        }
    }

    return 0; /* success */
}


/* Escribir un bloque de 512 bytes por polling (CMD24) */
int sdhc_write_single_block_polling(uint32_t block_addr, const uint8_t *data)
{
    /* setear BLKATTR: BLKSIZE + BLKCNT */
    SDHC->BLKATTR = ((SD_BLOCK_SIZE & 0xFFFFu) << SDHC_BLKATTR_BLKSIZE_SHIFT)
                    | ((1u & 0xFFFFu) << SDHC_BLKATTR_BLKCNT_SHIFT);

    /* preparar XFERTYP para CMD24 con data present (DPSEL) y R1 response */
    uint32_t xfer = 0;
    xfer |= (24u << SDHC_XFERTYP_CMDINX_SHIFT); /* CMD24 */
    xfer |= SDHC_XFERTYP_DPSEL_MASK;            /* data present */
    xfer |= (1u << SDHC_XFERTYP_RSPTYP_SHIFT);  /* R1 */
    SDHC->CMDARG = block_addr;
    SDHC->IRQSTAT = 0xFFFFFFFFu;
    SDHC->XFERTYP = xfer;

    /* escribir por DATPORT (32-bit words) cuando BWR esté activo */
    uint32_t words = SD_BLOCK_SIZE / 4;
    const uint32_t *wptr = (const uint32_t *)data;
    uint32_t written = 0;
    for (int loop = 0; loop < 2000000 && written < words; ++loop) {
        uint32_t st = SDHC->IRQSTAT;
        if (st & SDHC_IRQSTAT_BWR_MASK) {
            SDHC->IRQSTAT = SDHC_IRQSTAT_BWR_MASK; /* clear */
            SDHC->DATPORT = wptr[written++];
        }
        if (st & SDHC_IRQSTAT_CTOE_MASK) {
            SDHC->IRQSTAT = SDHC_IRQSTAT_CTOE_MASK;
            return -2;
        }
    }
    if (written < words) return -4; /* timeout */

    /* esperar Transfer Complete */
    for (int loop = 0; loop < 2000000; ++loop) {
        if (SDHC->IRQSTAT & SDHC_IRQSTAT_TC_MASK) {
            SDHC->IRQSTAT = SDHC_IRQSTAT_TC_MASK;
            return 0;
        }
        if (SDHC->IRQSTAT & SDHC_IRQSTAT_CTOE_MASK) {
            SDHC->IRQSTAT = SDHC_IRQSTAT_CTOE_MASK;
            return -3;
        }
    }
    return -5;
}

/* Leer un bloque de 512 bytes por polling (CMD17) */
int sdhc_read_single_block_polling(uint32_t block_addr, uint8_t *data)
{
    SDHC->BLKATTR = ((SD_BLOCK_SIZE & 0xFFFFu) << SDHC_BLKATTR_BLKSIZE_SHIFT)
                    | ((1u & 0xFFFFu) << SDHC_BLKATTR_BLKCNT_SHIFT);

    uint32_t xfer = 0;
    xfer |= (17u << SDHC_XFERTYP_CMDINX_SHIFT); /* CMD17 */
    xfer |= SDHC_XFERTYP_DPSEL_MASK;            /* data present */
    xfer |= (1u << SDHC_XFERTYP_RSPTYP_SHIFT);  /* R1 */
    SDHC->CMDARG = block_addr;
    SDHC->IRQSTAT = 0xFFFFFFFFu;
    SDHC->XFERTYP = xfer;

    uint32_t words = SD_BLOCK_SIZE / 4;
    uint32_t *wptr = (uint32_t *)data;
    uint32_t read = 0;
    for (int loop = 0; loop < 2000000 && read < words; ++loop) {
        uint32_t st = SDHC->IRQSTAT;
        if (st & SDHC_IRQSTAT_BRR_MASK) {
            SDHC->IRQSTAT = SDHC_IRQSTAT_BRR_MASK;
            wptr[read++] = SDHC->DATPORT;
        }
        if (st & SDHC_IRQSTAT_CTOE_MASK) {
            SDHC->IRQSTAT = SDHC_IRQSTAT_CTOE_MASK;
            return -2;
        }
    }
    if (read < words) return -4;

    for (int loop = 0; loop < 2000000; ++loop) {
        if (SDHC->IRQSTAT & SDHC_IRQSTAT_TC_MASK) {
            SDHC->IRQSTAT = SDHC_IRQSTAT_TC_MASK;
            return 0;
        }
        if (SDHC->IRQSTAT & SDHC_IRQSTAT_CTOE_MASK) {
            SDHC->IRQSTAT = SDHC_IRQSTAT_CTOE_MASK;
            return -3;
        }
    }
    return -5;
}

////////////////////////////////////////////////////////////////////////
// LOCAL FUNCTIONS
////////////////////////////////////////////////////////////////////////
void sdhcSetClock(uint32_t frequency)
{
	uint8_t sdcklfs, dvs;
	getSettingsByFrequency(frequency, &sdcklfs, &dvs);
	SDHC->SYSCTL = (SDHC->SYSCTL & ~SDHC_SYSCTL_SDCLKEN_MASK) | SDHC_SYSCTL_SDCLKEN(0);
	SDHC->SYSCTL = (SDHC->SYSCTL & ~SDHC_SYSCTL_SDCLKFS_MASK) | SDHC_SYSCTL_SDCLKFS(sdcklfs);
	SDHC->SYSCTL = (SDHC->SYSCTL & ~SDHC_SYSCTL_DVS_MASK)     | SDHC_SYSCTL_DVS(dvs);
	SDHC->SYSCTL = (SDHC->SYSCTL & ~SDHC_SYSCTL_SDCLKEN_MASK) | SDHC_SYSCTL_SDCLKEN(1);
}

static void SDHC_CardDetectedHandler(void)
{
	if (gpioRead(SDHC_SWITCH_PIN) == HIGH)
	{
//		if (!context.cardStatus)
//		{
//			context.cardStatus = true;
//			if (context.onCardInserted)
//			{
//				context.onCardInserted();
//			}
//		}
		gpioWrite(PIN_LED_GREEN, true);
		gpioWrite(PIN_LED_BLUE, false);
		gpioWrite(PIN_LED_RED, false);
	}
	else
	{
//		if (context.cardStatus)
//		{
//			context.cardStatus = false;
//			if (context.onCardRemoved)
//			{
//				context.onCardRemoved();
//			}
//		}
		gpioWrite(PIN_LED_GREEN, false);
		gpioWrite(PIN_LED_BLUE, false);
		gpioWrite(PIN_LED_RED, true);
	}
}

void sdhcInitializationClocks(void)
{
	// Send initialization clocks to the SD card
	uint32_t timeout = SDHC_RESET_TIMEOUT;
	bool forceExit = false;
	SDHC->SYSCTL |= SDHC_SYSCTL_INITA_MASK;
	while (!forceExit && (SDHC->SYSCTL & SDHC_SYSCTL_INITA_MASK))
	{
		if (timeout)
		{
			timeout--;
		}
		else
		{
			forceExit = true;
		}
	}
}






//////////////// COSAS NO MIAS

// SDHC possible values for the register fields
#define SDHC_RESPONSE_LENGTH_NONE	SDHC_XFERTYP_RSPTYP(0b00)
#define SDHC_RESPONSE_LENGTH_48		SDHC_XFERTYP_RSPTYP(0b10)
#define SDHC_RESPONSE_LENGTH_136	SDHC_XFERTYP_RSPTYP(0b01)
#define SDHC_RESPONSE_LENGTH_48BUSY	SDHC_XFERTYP_RSPTYP(0b11)

#define SDHC_COMMAND_CHECK_CCR		SDHC_XFERTYP_CCCEN(0b1)
#define SDHC_COMMAND_CHECK_INDEX	SDHC_XFERTYP_CICEN(0b1)


bool sdhcStartTransfer(sdhc_command_t* command, sdhc_data_t* data)
{
	bool 		successful = false;
	uint32_t	flags = 0;

//	if (sdhcIsAvailable())
//	{
		if (!(SDHC->PRSSTAT & SDHC_PRSSTAT_CDIHB_MASK) && !(SDHC->PRSSTAT & SDHC_PRSSTAT_CIHB_MASK))
		{
//			context.currentCommand = command;
//			context.currentData = data;
//			context.transferedWords = 0;
//			context.available = false;
//			context.transferCompleted = false;
//			context.currentError = SDHC_ERROR_OK;

			// Command related configurations of the peripheral
			if (command)
			{
				// Set the response length expected, and whether index and CCC check is required
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

			// Data related configurations of the peripheral
			if (data)
			{
				// Alignment of words, the memory buffer passed by the user must be 4 byte aligned
				if (data->blockSize % sizeof(uint32_t) != 0U)
				{
					data->blockSize += sizeof(uint32_t) - (data->blockSize % sizeof(uint32_t));
				}

				// Set the block size and block count
				SDHC->BLKATTR = (SDHC->BLKATTR & ~SDHC_BLKATTR_BLKCNT_MASK) | SDHC_BLKATTR_BLKCNT(data->blockCount);
				SDHC->BLKATTR = (SDHC->BLKATTR & ~SDHC_BLKATTR_BLKSIZE_MASK) | SDHC_BLKATTR_BLKSIZE(data->blockSize);

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

//			// For data transfer, when not using the CPU, additional configuration of the DMA is required
//			if (data)
//			{
//				switch (data->transferMode)
//				{
//					case SDHC_TRANSFER_MODE_ADMA1:
//						if (sdhcSetAdma1Table(data))
//						{
//							successful = true;
//						}
//						break;
//					case SDHC_TRANSFER_MODE_ADMA2:
//						if (sdhcSetAdma2Table(data))
//						{
//							successful = true;
//						}
//						break;
//					case SDHC_TRANSFER_MODE_CPU:
//					default:
//						successful = true;
//						break;
//				}
//			}
//			else
//			{
//				successful = true;
//			}
			successful = true;
		}
//	}

	if (successful)
	{
		// Starts the transfer process
		SDHC->XFERTYP = flags;
	}

	return successful;
}

sdhc_error_t sdhcTransfer(sdhc_command_t* command, sdhc_data_t* data)
{
	sdhc_error_t error = 0x00;
	bool forceExit = false;

	if (sdhcStartTransfer(command, data))
	{
		while (!forceExit)
		{
			if (SDHC->IRQSTAT & (SDHC_IRQSTAT_CCE_MASK | SDHC_IRQSTAT_CTOE_MASK) == 0)
			{
				forceExit = 1;
			}
		}
	}
	else
	{
		error = 0x01;
	}

	return error;
}
