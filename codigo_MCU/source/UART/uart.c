/*******************************************************************************
  @file     +uart.c+
  @brief    +Driver para UART+
  @author   +GRUPO 3+
 ******************************************************************************/

/*******************************************************************************
 * INCLUDE HEADER FILES
 ******************************************************************************/
#include "uart.h"
#include "port.h"
#include "cola.h"

#include "../board.h"


/*******************************************************************************
 * CONSTANT AND MACRO DEFINITIONS USING #DEFINE
 ******************************************************************************/
#define UART_HAL_DEFAULT_BAUDRATE 9600

/*******************************************************************************
 * ENUMERATIONS AND STRUCTURES AND TYPEDEFS
 ******************************************************************************/


/*******************************************************************************
 * FUNCTION PROTOTYPES FOR PRIVATE FUNCTIONS WITH FILE LEVEL SCOPE
 ******************************************************************************/
static void UART_SetBaudRate (UART_Type *uart, uint32_t baudrate);
static void UART_IQR_handler(uint8_t id);


/*******************************************************************************
 * ROM CONST VARIABLES WITH FILE LEVEL SCOPE
 ******************************************************************************/

/*******************************************************************************
 * STATIC VARIABLES AND CONST VARIABLES WITH FILE LEVEL SCOPE
 ******************************************************************************/
static UART_Type * const uart_base[UART_CANT_IDS] = UART_BASE_PTRS;
static cola_t txBuffer[UART_CANT_IDS];
static cola_t rxBuffer[UART_CANT_IDS];


static OS_SEM* semaphores[UART_CANT_IDS];
static OS_ERR err;

/*******************************************************************************
 *******************************************************************************
                        GLOBAL FUNCTION DEFINITIONS
 *******************************************************************************
 ******************************************************************************/
void uartInit (uint8_t id, uart_cfg_t config, OS_SEM* semaphore_ptr)
{
	PCR_Type* ptr_pcr_rx;
	PCR_Type* ptr_pcr_tx;
	PCR_Type* ptr_pcr_cts;
	PCR_Type* ptr_pcr_rts;
	UART_Type* ptr_uart;



	switch(id)
	{
	case UART_0:
		SIM->SCGC4 |= SIM_SCGC4_UART0_MASK;
		NVIC_EnableIRQ(UART0_RX_TX_IRQn);
		ptr_pcr_rx = PCR_ADRESS(PORTNUM2PIN(PB,17));
		ptr_pcr_tx = PCR_ADRESS(PORTNUM2PIN(PB,16));
		ptr_pcr_cts = PCR_ADRESS(PORTNUM2PIN(PB,3));
		ptr_pcr_rts = PCR_ADRESS(PORTNUM2PIN(PB,2));
		ptr_uart = uart_base[UART_0];
		ptr_pcr_rx -> MUX = PORT_mAlt3;
		ptr_pcr_tx -> MUX = PORT_mAlt3;
		ptr_pcr_cts -> MUX = PORT_mAlt3;
		ptr_pcr_rts -> MUX = PORT_mAlt3;
		break;
	case UART_1:
		SIM->SCGC4 |= SIM_SCGC4_UART1_MASK;
		NVIC_EnableIRQ(UART1_RX_TX_IRQn);
		ptr_pcr_rx = PCR_ADRESS(PORTNUM2PIN(PC,3));
		ptr_pcr_tx = PCR_ADRESS(PORTNUM2PIN(PC, 4));
		ptr_pcr_cts = PCR_ADRESS(PORTNUM2PIN(PC,2));
		ptr_pcr_rts = PCR_ADRESS(PORTNUM2PIN(PC,1));
		ptr_uart = uart_base[UART_1];
		ptr_pcr_rx -> MUX = PORT_mAlt3;
		ptr_pcr_tx -> MUX = PORT_mAlt3;
		ptr_pcr_cts -> MUX = PORT_mAlt3;
		ptr_pcr_rts -> MUX = PORT_mAlt3;
		break;
	case UART_2:
		SIM->SCGC4 |= SIM_SCGC4_UART2_MASK;
		NVIC_EnableIRQ(UART2_RX_TX_IRQn);
		ptr_pcr_rx = PCR_ADRESS(PORTNUM2PIN(PD,2));
		ptr_pcr_tx = PCR_ADRESS(PORTNUM2PIN(PD, 3));
		ptr_pcr_cts = PCR_ADRESS(PORTNUM2PIN(PD,1));
		ptr_pcr_rts = PCR_ADRESS(PORTNUM2PIN(PD,0));
		ptr_uart = uart_base[UART_2];
		ptr_pcr_rx -> MUX = PORT_mAlt2;
		ptr_pcr_tx -> MUX = PORT_mAlt2;
		ptr_pcr_cts -> MUX = PORT_mAlt2;
		ptr_pcr_rts -> MUX = PORT_mAlt2;
		break;
	case UART_3:
		SIM->SCGC4 |= SIM_SCGC4_UART3_MASK;
		NVIC_EnableIRQ(UART3_RX_TX_IRQn);
		ptr_pcr_rx = PCR_ADRESS(PORTNUM2PIN(PC,16));
		ptr_pcr_tx = PCR_ADRESS(PORTNUM2PIN(PC, 17));
		ptr_pcr_cts = PCR_ADRESS(PORTNUM2PIN(PB,9));
		ptr_uart = uart_base[UART_3];
		ptr_pcr_rx -> MUX = PORT_mAlt2;
		ptr_pcr_tx -> MUX = PORT_mAlt2;
		ptr_pcr_cts -> MUX = PORT_mAlt2;
		//ptr_pcr_rts -> MUX = PORT_mAlt2;
		break;
	case UART_4:
		SIM->SCGC1 |= SIM_SCGC1_UART4_MASK;
		NVIC_EnableIRQ(UART4_RX_TX_IRQn);
		ptr_pcr_rx = PCR_ADRESS(PORTNUM2PIN(PE,25));
		ptr_pcr_tx = PCR_ADRESS(PORTNUM2PIN(PE, 24));
		ptr_pcr_cts = PCR_ADRESS(PORTNUM2PIN(PB,26));
		ptr_uart = uart_base[UART_4];
		ptr_pcr_rx -> MUX = PORT_mAlt2;
		ptr_pcr_tx -> MUX = PORT_mAlt2;
		ptr_pcr_cts -> MUX = PORT_mAlt2;
		//ptr_pcr_rts -> MUX = PORT_mAlt2;
		break;
	case UART_5:
			//	SIM->SCGC1 |= SIM_SCGC1_UART5_MASK;
			break;

	}
	if(id < 5)
	{
		//ptr_uart->C1 |= UART_C1_PT(config.parity) | UART_C1_PE_MASK;
		UART_SetBaudRate(ptr_uart, config.baudrate);
		ptr_uart->C2 = UART_C2_TE_MASK | UART_C2_RE_MASK | UART_C2_RIE_MASK ;//UART_C2_TIE_MASK ;
		ptr_uart->PFIFO |=  UART_PFIFO_RXFIFOSIZE(3)| UART_PFIFO_TXFE(1) | UART_PFIFO_RXFE(1);
		//ptr_uart->RWFIFO = 1;
		colaInit(&txBuffer[id]);
		colaInit(&rxBuffer[id]);
		semaphores[id] = semaphore_ptr;
	}
}




uint8_t uartIsRxMsg(uint8_t id)
{


}

uint8_t uartGetRxMsgLength(uint8_t id)
{
	return rxBuffer[id].count;
}


uint8_t uartReadMsg(uint8_t id, char* msg, uint8_t cant)
{
	return colaPull(&rxBuffer[id]);
}

void uartWriteMsg(uint8_t id, unsigned const char* msg, uint8_t cant) {
	uint8_t i = 0;
	if(cant < ((Q_SIZE) - txBuffer[id].count) ) //MaxElementos - cantElementos = elementosLibres
		for(i = 0 ; i < cant ; i++)
			colaPush(&txBuffer[id], *msg++);
}

bool uartWriteMsg2Check(void) {
	return (UART0->S1)& UART_S1_TDRE_MASK;
}

void uartWriteMsg2(unsigned const char msg) {
	UART0->D = msg;
}

void uartWriteMsgHardware(uint8_t id, const char* msg, uint8_t cant) {
	uart_base[id]->D = (uint8_t)*msg;
}

void uartWriteMsgBloq(uint8_t id, const char* msg, uint8_t cant) {
	uint8_t i = 0;
	while(i++<cant){
		while(((uart_base[id]->S1)& UART_S1_TDRE_MASK) == 0);
		uart_base[id]->D = (uint8_t)*msg++;
	}
}

uint8_t uartIsTxMsgComplete(uint8_t id)
{
	/*
	if(uart_base[id]->TCFIFO == 0)
	{
		return true;
	}
	else
		return false;
	*/
	return !(uart_base[id]->TCFIFO);

}




/*******************************************************************************
 *******************************************************************************
                        LOCAL FUNCTION DEFINITIONS
 *******************************************************************************
 ******************************************************************************/
static void UART_SetBaudRate (UART_Type *uart, uint32_t baudrate)
{
	uint16_t sbr, brfa;
	uint32_t clock;

	clock = ((uart == UART0) || (uart == UART1))?(__CORE_CLOCK__):(__CORE_CLOCK__ >> 1);

	baudrate = ((baudrate == 0)?(UART_HAL_DEFAULT_BAUDRATE):
			((baudrate > 0x1FFF)?(UART_HAL_DEFAULT_BAUDRATE):(baudrate)));

	sbr = clock / (baudrate * 16);               // sbr = clock/(Baudrate x 16)
	brfa = (clock * 2) / baudrate - (sbr * 32); // brfa = 2*Clock/baudrate - 32*sbr

	uart->BDH = UART_BDH_SBR(sbr >> 8);
	uart->BDL = UART_BDL_SBR(sbr);
	uart->C4 =  UART_C4_BRFA(brfa);
}

static void UART_IQR_handler(uint8_t id) {
	//Interrupción de transmision
	/*
	if((uart_base[id]->S1)& UART_S1_TC_MASK) {
		if(txBuffer[id].count)
			uart_base[id]->D = colaPull(&txBuffer[id]);
		else
			uart_base[id]->D = 0;
	}
	*/

	//Interrupcion de recepcion
	if((uart_base[id]->S1)& UART_S1_RDRF_MASK) {
		if(rxBuffer[id].count < Q_SIZE-1)
		{
			colaPush(&rxBuffer[id], (unsigned const char)uart_base[id]->D);
			OSSemPost(semaphores[id], OS_OPT_POST_ALL, &err);
		}
		//else //ocurre error, se va a pisar data en el buffer circuilar
			//uart_base[id]->D = 0;
	}


}

void UART0_RX_TX_IRQHandler(void) {
	UART_IQR_handler(UART_0);
}

void UART1_RX_TX_IRQHandler(void) {
	UART_IQR_handler(UART_1);
}

void UART2_RX_TX_IRQHandler(void) {
	UART_IQR_handler(UART_2);
}

void UART3_RX_TX_IRQHandler(void) {
	UART_IQR_handler(UART_3);
}
void UART4_RX_TX_IRQHandler(void) {
	UART_IQR_handler(UART_4);
}

void UART5_RX_TX_IRQHandler(void) {
	UART_IQR_handler(UART_5);
}
























