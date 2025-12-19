/***************************************************************************/ /**
   @file     uart.h
   @brief    UART driver
   @author   Grupo 3
  ******************************************************************************/

#ifndef _UART_DRV_H_
#define _UART_DRV_H_

/*******************************************************************************
 * INCLUDE HEADER FILES
 ******************************************************************************/

#include <stdint.h>
#include <stdbool.h>

/*******************************************************************************
 * CONSTANT AND MACRO DEFINITIONS USING #DEFINE
 ******************************************************************************/

// Size of internal buffer for reception.
#define INTERNAL_RX_BUFFER_LENGTH 100U
#define NUM_UART_CHANNELS 2

/*******************************************************************************
 * ENUMERATIONS AND STRUCTURES AND TYPEDEFS
 ******************************************************************************/

typedef enum
{
    UART_0,
    UART_3
} UARTx_t;

typedef enum
{
    UARTx_NOT_INIT = -1,
    UARTx_NO_DATA,
    UARTx_NEW_CHAR,
} UARTxStatus_t;

// UART mode definitions
typedef enum
{
    UART_MODE_NORMAL,
    UART_MODE_HW_FLOW_CONTROL
} UARTxMode_t;

typedef enum
{
    UART_PARITY_NONE,
    UART_PARITY_EVEN,
    UART_PARITY_ODD
} UARTxParity_t;

typedef enum
{
    UART_STOP_BITS_1,
    UART_STOP_BITS_2,
} UARTxStopBits_t;

typedef enum
{
    UART_DATA_BITS_8,
    UART_DATA_BITS_9,
} UARTxDataBits_t;

// UART configuration structure
typedef struct
{
    UARTxMode_t mode;
    uint32_t baudrate;
    uint8_t data_bits;
    UARTxParity_t parity;
    UARTxStopBits_t stop_bits;
    // uint8_t bit_order;
    // uint8_t invert_data;
    // uint8_t hw_flow_control;
    // uint8_t use_ext_clk;
    // struct
    // {
    //     uint8_t rx_fifo_level;
    //     uint8_t tx_fifo_level;
    // } fifo_config;
} UARTxConfig_t;

/*******************************************************************************
 * VARIABLE PROTOTYPES WITH GLOBAL SCOPE
 ******************************************************************************/

// +ej: extern unsigned int anio_actual;+

/*******************************************************************************
 * FUNCTION PROTOTYPES WITH GLOBAL SCOPE
 ******************************************************************************/

/**
 * @brief Initializes UART
 *
 */
void UARTinit(UARTx_t id, UARTxConfig_t *config);


uint8_t UARTisTxMsgComplete(UARTx_t id);

/**
 * @brief Setups desired UART with specified config
 *
 * @param uart UARTx_t (UART_0, UART_3)
 * @param config UARTxConfig_t. Pass 0 to get default settings.
 */
void UARTconfig(UARTx_t id, UARTxConfig_t *config);

/**
 * @brief Changes baudrate of desired UART. 9600 is the default value.
 *
 * @param uart
 * @param baudrate
 */
void UARTsetBaudRate(UARTx_t id, uint32_t baudrate);

/**
 * @brief Sets data format for the desired UART. 8bits is the default value
 *
 * @param uart UARTx_t (UART_0, UART_3)
 * @param dataBits UARTxDataBits_t (UART_DATA_BITS_8, UART_DATA_BITS_9)
 */
void UARTsetDataBits(UARTx_t uart, UARTxDataBits_t dataBits);

/**
 * @brief Sets parity for the desired UART. NONE is the default value
 *
 * @param uart UARTx_t (UART_0, UART_3)
 * @param parity  UARTxParity_t (UART_PARITY_NONE, UART_PARITY_EVEN, UART_PARITY_ODD)
 */
void UARTsetParity(UARTx_t uart, UARTxParity_t parity);

/**
 * @brief Returns status of given UART. If there is data to read, returns number of internal-buffered bytes (>= 1).
 *
 * @param uart UARTx_t (UART_0, UART_3)
 * @return int16_t UARTxStatus_t (UARTx_NOT_INIT = -1, UARTx_NO_DATA, UARTx_NEW_CHAR.)
 */
int16_t UARTgetStatus(UARTx_t uart);

/**
 * @brief Gets the first internal-buffered received byte of the given UART. The byte is stored in the given pointer
 * and is removed from the internal buffer. The function returns the number of pendings bytes in the
 * internal buffer.
 *
 * @param uart UARTx_t (UART_0, UART_3)
 * @param data Pointer to a char to store de byte
 * @return int16_t -1 if error, pending bytes to read otherwise
 */
int16_t UARTgetChar(UARTx_t uart, char *data);

/**
 * @brief Gets the whole internal buffer of the given UART, which is stored in the given pointer. The internal buffer
 * is cleared. The function returns the number of bytes received.
 *
 * @param uart UARTx_t (UART_0, UART_3)
 * @param str Pointer to store the string
 * @return int16_t -1 if error, number of bytes received otherwise.
 */
int16_t UARTgetString(UARTx_t uart, char *str);

/**
 * @brief Sends a byte through given UART
 *
 * @param uart UARTx_t (UART_0, UART_3)
 * @param data byte to send
 * @return N/A
 */
void UARTsendChar(UARTx_t uart, char data);

/**
 * @brief Sends a string through given UART. Size of string must be specified.
 *
 * @param uart UARTx_t (UART_0, UART_3)
 * @param data Pointer to string to send
 * @param length Length of the string
 * @return int16_t -1 if error, number of transmitted bytes otherwise
 */
void UARTsendString(UARTx_t uart, char *data, uint16_t length);

/**
 * @brief Port of standard printf function. Supports %d, %s, %f (4 fixed decimals), %c
 *
 * @param uart UARTx_t (UART_0, UART_3)
 * @param format String with printf format
 * @param ... Params to print
 * @return Success
 */

uint8_t num2str(uint16_t value, char *buffer);
/**
 * @brief Converts uint16_t to string
 *
 * @param value Number to convert
 * @param buffer Pointer to store the string
 * @return uint8_t Length of the string
 */

uint16_t UARTPushChar(UARTx_t uart, uint8_t);
uint8_t UARTPullChar(UARTx_t uart, char *data);
uint16_t UARTQueueStatusRX(UARTx_t uart);
/*******************************************************************************
 ******************************************************************************/

#endif // _UART_DRV_H_
