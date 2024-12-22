/***************************************************************************//**
  @file     port.h
  @brief    Simple GPIO Pin services, similar to Arduino
  @author   Grupo 2
 ******************************************************************************/

#ifndef _PORT_H_
#define _PORT_H_

/*******************************************************************************
 * INCLUDE HEADER FILES
 ******************************************************************************/

#include <stdint.h>
#include <stdbool.h>
#include "hardware.h"


/*******************************************************************************
 * CONSTANT AND MACRO DEFINITIONS USING #DEFINE
 ******************************************************************************/
typedef uint8_t pin_t;
typedef void (*pinIrqFun_t)(void);

#define CANT_PORTS 5
#define BITS_PORTS 32


// Convert port and number into pin ID
// Ex: PTB5  -> PORTNUM2PIN(PB,5)  -> 0x25
//     PTC22 -> PORTNUM2PIN(PC,22) -> 0x56
#define PORTNUM2PIN(p,n)    (((p)<<5) + (n))
#define PIN2PORT(p)         (((p)>>5) & 0x07)
#define PIN2NUM(p)          ((p) & 0x1F)


static PORT_Type * const port_base[CANT_PORTS] = PORT_BASE_PTRS;
static uint32_t const ports_irqs[CANT_PORTS] = PORT_IRQS;


#define PORT_ADRESS(pin) 		(port_base[PIN2PORT(pin)])
#define PCR_ADRESS(pin) 		(&(PORT_ADRESS(pin)->PCR[PIN2NUM(pin)]))
#define PCR_TYPE_ADRESS(pin) 	((PCR_Type*)(PCR_ADRESS(pin)))

#define CLEAR_ISF_PORT(p) (p->ISFR=0xFFFFFFFF)

/*******************************************************************************
 * ENUMERATIONS AND STRUCTURES AND TYPEDEFS
 ******************************************************************************/

// Ports
enum { PA1, PB1, PC1, PD1, PE1 };


enum PORTMux
{
	PORT_mAnalog,
	PORT_mGPIO,
	PORT_mAlt2,
	PORT_mAlt3,
	PORT_mAlt4,
	PORT_mAlt5,
	PORT_mAlt6,
	PORT_mAlt7,
};

typedef struct {
    uint32_t PS : 1;
    uint32_t PE : 1;
    uint32_t SRE : 1;
    uint32_t RESERVED_0 : 1;
    uint32_t PFE : 1;
    uint32_t ODE : 1;
    uint32_t DSE : 1;
    uint32_t RESERVED_1 : 1;
    uint32_t MUX : 3;
    uint32_t RESERVED_2 : 4;
    uint32_t LK : 1;
    uint32_t IRQC : 4;
    uint32_t RESERVED_3 : 4;
    uint32_t ISF : 1;
    uint32_t RESERVED_4 : 7;
} PCR_Type;


/*******************************************************************************
 * VARIABLE PROTOTYPES WITH GLOBAL SCOPE
 ******************************************************************************/



/*******************************************************************************
 * FUNCTION PROTOTYPES WITH GLOBAL SCOPE
 ******************************************************************************/

bool portAddIRQ (pin_t pin, uint8_t irqMode, pinIrqFun_t irqFun);
void portFILT(pin_t pin, uint8_t ciclos);


/*******************************************************************************
 ******************************************************************************/

#endif // _PORT_H_
