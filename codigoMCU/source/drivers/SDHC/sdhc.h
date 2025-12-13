#ifndef SDHC_H
#define SDHC_H

#include <stdint.h>
#include <stdbool.h>


// SDHC internal parameters
#define SDHC_MAXIMUM_BLOCK_SIZE		4096
#define SDHC_RESET_TIMEOUT			100000
#define SDHC_CLOCK_FREQUENCY		(96000000U)

typedef enum {
	SDHC_TRANSFER_MODE_CPU,		// Data transfer will be executed by the CPU host
	SDHC_TRANSFER_MODE_ADMA1,	// Data transfer will be executed by the advanced DMA controller v1
	SDHC_TRANSFER_MODE_ADMA2	// Data transfer will be executed by the advanced DMA controller v2
} sdhc_transfer_mode_t;

typedef enum {
	SDHC_RESET_DATA,
	SDHC_RESET_CMD,
	SDHC_RESET_ALL
} sdhc_reset_t;

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

typedef struct {
	uint8_t					index;			// Index of the command
	uint32_t				argument;		// Argument of the command
	sdhc_command_type_t		commandType;	// Type of command
	sdhc_response_type_t	responseType;	// Type of response expected
	uint32_t				response[4];	// Response placeholder
} sdhc_command_t;

typedef struct {
	uint32_t				blockCount;		// Amount of blocks to be sent or received
	uint32_t				blockSize;		// Size in bytes of each block transfered
	uint32_t*				writeBuffer;	// Buffer with write data, used only when writing, else should be NULL
	uint32_t*				readBuffer;		// Buffer for the read data, used only when reading, else should be NULL
	sdhc_transfer_mode_t	transferMode;	// Data transfer mode
} sdhc_data_t;

void sdhc_enable_clocks_and_pins(void);
void sdhc_reset(sdhc_reset_t reset_type);
void sdhc_soft_reset_all(void);
bool sdhc_start_transfer(sdhc_command_t* command, sdhc_data_t* data);
void sdhc_initialization_clocks(void);
sdhc_error_t sdhc_transfer(sdhc_command_t* command, sdhc_data_t* data);
void sdhc_set_clock(uint32_t hz);


#endif //SDHC_H
