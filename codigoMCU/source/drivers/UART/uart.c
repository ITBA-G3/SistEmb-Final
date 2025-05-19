/***************************************************************************/ /**
  @file     uart.c
  @brief    UART driver
  @author   Grupo 3
 ******************************************************************************/

/*******************************************************************************
 * INCLUDE HEADER FILES
 ******************************************************************************/

#include "uart.h"
#include "hardware.h"
#include "MK64F12.h"
#include "gpio.h"
#include <stdarg.h>
#include "cqueue.h"
#include "cqueue_rx.h"

/*******************************************************************************
 * CONSTANT AND MACRO DEFINITIONS USING #DEFINE
 ******************************************************************************/

#define UART0_TX_PIN 17 // PB17
#define UART0_RX_PIN 16 // PB16

#define UART3_TX_PIN 17 // PC17
#define UART3_RX_PIN 16 // PC16

#define UART_HAL_DEFAULT_BAUDRATE 9600

#define INT_BUFFER_SIZE 10U
#define FLOAT_BUFFER_SIZE 20U

#define DEFAULT_FLOAT_DECIMAL_PLACES 4

/*******************************************************************************
 * ENUMERATIONS AND STRUCTURES AND TYPEDEFS
 ******************************************************************************/

/*******************************************************************************
 * VARIABLES WITH GLOBAL SCOPE
 ******************************************************************************/

/*******************************************************************************
 * FUNCTION PROTOTYPES FOR PRIVATE FUNCTIONS WITH FILE LEVEL SCOPE
 ******************************************************************************/

/*******************************************************************************
 * ROM CONST VARIABLES WITH FILE LEVEL SCOPE
 ******************************************************************************/

static UART_Type *const UART_PTRS[] = UART_BASE_PTRS;

/*******************************************************************************
 * STATIC VARIABLES AND CONST VARIABLES WITH FILE LEVEL SCOPE
 ******************************************************************************/
static UARTxStatus_t uartStatus[NUM_UART_CHANNELS] = {UARTx_NOT_INIT, UARTx_NOT_INIT};
static uint32_t ReceivedData[NUM_UART_CHANNELS];
static uint8_t ReceiveBuffer[NUM_UART_CHANNELS][INTERNAL_RX_BUFFER_LENGTH];

static uint32_t intCont = 0;

/*******************************************************************************
*******************************************************************************
                       GLOBAL FUNCTION DEFINITIONS
*******************************************************************************
******************************************************************************/

void UARTinit(UARTx_t id, UARTxConfig_t *config)
{
    UARTconfig(id, config);
    uartStatus[UART_0] = UARTx_NO_DATA;
    QueueInit();
    RXQueueInit();
}

void UARTconfig(UARTx_t id, UARTxConfig_t *config)
{

    switch (id)
    {
    	case UART_0:
			SIM->SCGC4 |= SIM_SCGC4_UART0_MASK; // clock gating

			PORTB->PCR[UART0_TX_PIN] = 0x0;               // clear all bits
			PORTB->PCR[UART0_TX_PIN] |= PORT_PCR_MUX(3);  // set MUX to UART0
			PORTB->PCR[UART0_TX_PIN] |= PORT_PCR_IRQC(0); // disable gpio interrupts

			PORTB->PCR[UART0_RX_PIN] = 0x0;                  // clear all bits
			PORTB->PCR[UART0_RX_PIN] |= PORT_PCR_MUX(3);     // set MUX to UART0
			PORTB->PCR[UART0_RX_PIN] |= PORT_PCR_IRQC(0x00); // disable gpio interrupts

			UARTsetBaudRate(id, config->baudrate);
			UARTsetParity(id, config->parity);
			UARTsetDataBits(id, config->data_bits);

			UART0->PFIFO = 0x80; // enable 8bit TXFIFO
			UART0->CFIFO = 0xC0; // flush TXFIFO & RXFIFO
			UART0->CFIFO = 0x00;
			UART0->TWFIFO = 0x01; // watermark == 1byte left

			// enable tx & rx & rxints
			UART0->C2 = UART_C2_TE_MASK | UART_C2_RE_MASK | UART_C2_RIE_MASK;

			NVIC_EnableIRQ(UART0_RX_TX_IRQn); // enable uart interrupts

			uartStatus[id] = UARTx_NO_DATA;
			break;

		case UART_3:
			SIM->SCGC4 |= SIM_SCGC4_UART3_MASK; // clock gating

			PORTC->PCR[UART3_TX_PIN] = 0x0;               // clear all bits
			PORTC->PCR[UART3_TX_PIN] |= PORT_PCR_MUX(3);  // set MUX to UART0
			PORTC->PCR[UART3_TX_PIN] |= PORT_PCR_IRQC(0); // disable gpio interrupts

			PORTC->PCR[UART3_RX_PIN] = 0x0;               // clear all bits
			PORTC->PCR[UART3_RX_PIN] |= PORT_PCR_MUX(3);  // set MUX to UART0
			PORTC->PCR[UART3_RX_PIN] |= PORT_PCR_IRQC(0); // disable gpio interrupts

			UARTsetBaudRate(id, config->baudrate);

			// enable tx & rx & rxints
			UART3->C2 = UART_C2_TE_MASK | UART_C2_RE_MASK | UART_C2_RIE_MASK;

			NVIC_EnableIRQ(UART3_RX_TX_IRQn); // enable uart interrupts

			uartStatus[id] = UARTx_NO_DATA;
			break;
    }
}

uint8_t UARTisTxMsgComplete(UARTx_t id){
	return QueueStatus();
}

void UARTsetBaudRate(UARTx_t id, uint32_t baudrate)
{
    UART_Type *uartPt = UART_PTRS[id];

    uint16_t sbr, brfa;
    uint32_t clock;

    clock = ((uartPt == UART0) || (uartPt == UART1)) ? (__CORE_CLOCK__) : (__CORE_CLOCK__ >> 1);

    baudrate = ((baudrate == 0) ? (UART_HAL_DEFAULT_BAUDRATE) : ((baudrate > 921600) ? (UART_HAL_DEFAULT_BAUDRATE) : (baudrate)));

    sbr = clock / (baudrate << 4);               // sbr = clock/(Baudrate x 16)
    brfa = (clock << 1) / baudrate - (sbr << 5); // brfa = 2*Clock/baudrate - 32*sbr

    uartPt->BDH = UART_BDH_SBR(sbr >> 8);
    uartPt->BDL = UART_BDL_SBR(sbr);
    uartPt->C4 = (uartPt->C4 & ~UART_C4_BRFA_MASK) | UART_C4_BRFA(brfa);
}

void UARTsetDataBits(UARTx_t id, UARTxDataBits_t numDataBits)
{
    if (numDataBits == UART_DATA_BITS_8)
        UART_PTRS[id]->C1 |= UART_C1_M_MASK;

    else if (numDataBits == UART_DATA_BITS_9)
        UART_PTRS[id]->C1 &= ~UART_C1_M_MASK;
}

void UARTsetParity(UARTx_t id, UARTxParity_t parity)
{
    if (parity != UART_PARITY_NONE)
    {
        UART_PTRS[id]->C1 |= (UART_PTRS[id]->C1 & ~UART_C1_PE_MASK) | UART_C1_PE(1);                         // enable parity
        UART_PTRS[id]->C1 |= (UART_PTRS[id]->C1 & ~UART_C1_PT_MASK) | UART_C1_PT(parity == UART_PARITY_ODD); // 1==odd, 0==even
    }
}

int16_t UARTgetStatus(UARTx_t uart)
{
    return (uartStatus[uart] < 0) ? UARTx_NOT_INIT : ReceivedData[uart];
}

int16_t UARTgetChar(UARTx_t uart, char *data)
{
    int16_t response = -1; // -1 means 'there's not char to get'

    if (ReceivedData[uart] && data)
    {
        *data = ReceiveBuffer[uart][0]; // get 1st char

        uint16_t i;
        for (i = 1; i < ReceivedData[uart]; i++)
        {
            ReceiveBuffer[uart][i - 1] = ReceiveBuffer[uart][i]; // shift one char to bottom
        }

        ReceiveBuffer[uart][(ReceivedData[uart]--) - 1] = 0; // clear top char

        response = ReceivedData[uart]; // pending chars to get
    }

    return response;
}

int16_t UARTgetString(UARTx_t id, char *str)
{
    int16_t response = -1; // -1 means 'there's not char to get'

    if (ReceivedData[id] && str)
    {
        uint16_t i;
        for (i = 0; i < ReceivedData[id]; i++)
        {
            str[i] = ReceiveBuffer[id][i]; // copy data to output array
            ReceiveBuffer[id][i] = 0;      // clear buffer
        }

        response = ReceivedData[id]; // return chars read
        ReceivedData[id] = 0;
    }

    return response;
}

void UARTsendChar(UARTx_t id, char data)
{
    if (!QueueStatus())
    {
        if (!(UART_PTRS[id]->S1 & UART_S1_TC_MASK)) // if transmitter active
        {
            PushQueue(data);
            UART0->C2 |= UART_C2_TCIE(1); // enables TC interrupts
        }
        else // transmitter idle
        {
            UART_PTRS[id]->D = data;
        }
    }
    else if (QueueStatus() > 0 && QueueStatus() < QOVERFLOW - 1)
    {
        PushQueue(data);
    }
}

void UARTsendString(UARTx_t id, char *data, uint16_t length)
{
    if (data && length)
    {
        uint16_t i;
        for (i = 0; i < length; i++)
        {
            UARTsendChar(id, data[i]);
        }
    }
}

uint8_t num2str(uint16_t value, char *buffer)
{
    uint8_t length = 0;

    if (buffer)
    {
        if (value != 0)
        {
            int i = 0;
            int n;
            bool sign = value < 0;

            if (sign) // add '-' only for base 10
                value *= -1;

            // Convert the number to a string
            while (value != 0)
            {
                n = value % 10;
                buffer[i++] = n + '0';
                value /= 10;
            }

            if (sign) // add '-' only for base 10
                buffer[i++] = '-';

            // Reverse the string
            int j;
            for (j = 0; j < i / 2; j++)
            {
                uint8_t temp = buffer[j];
                buffer[j] = buffer[i - 1 - j];
                buffer[i - 1 - j] = temp;
            }

            // Null terminate the string
            buffer[i] = '\n';

            length = i+1;
        }
        else
        {
            buffer[0] = '0';
            buffer[1] = '\n';
            length = 2;
        }
    }
    return length;
}

/*******************************************************************************
*******************************************************************************
                       LOCAL FUNCTION DEFINITIONS
*******************************************************************************
******************************************************************************/

void UART0_RX_TX_IRQHandler(void)
{
    uint8_t status = UART0->S1;

    if (status & UART_S1_TC_MASK) // transmit complete
    {
        if (QueueStatus())
        {
            UART0->D = PullQueue();

        }
        else
        {
            UART0->C2 = (UART0->C2 & ~UART_C2_TCIE_MASK) | UART_C2_TCIE(0); // disables TC interrupts
        }
    }

    if (status & UART_S1_RDRF_MASK)
    {
        ReceiveBuffer[UART_0][ReceivedData[UART_0]++] = UART0->D;
        intCont++;
    }
}

uint16_t UARTPushChar(UARTx_t uart, uint8_t data)
{
    RXPushQueue(data);
    return RXQueueStatus();
}

uint8_t UARTPullChar(UARTx_t uart, char *data)
{
    *data = RXPullQueue();
    return *data;
}

uint16_t UARTQueueStatusRX(UARTx_t uart)
{
    return RXQueueStatus();
}
