/***************************************************************************//**
  @file     I2C.c
  @brief    I2C communication functions prototypes
  @author   Grupo 3 - Jose Iván Hertter, Ezequiel Diaz Guzmán
 ******************************************************************************/

#include "I2C.h"
#include "cqueue.h"

static I2C_Type* I2C_ptr = I2C0;
void I2C_subroutine (void);
static uint8_t isMaster (void);
static uint8_t isTx (void);

static bool start_I2C = false;
static bool conclude = false;
static bool firstRX = true;

void init_I2C (void)
{
	i2c_QueueInit();	//	circular buffers for dataLengths
    // Activación del clock
    SIM->SCGC5 |= SIM_SCGC5_PORTE(1);  // Para los puertos
	// Para el I2C1
    SIM->SCGC4 |= SIM_SCGC4_I2C0(1);
    
	//Configuración de puertos
	PORTE->PCR[24] = PORT_PCR_MUX(5) | PORT_PCR_ODE(1);
	PORTE->PCR[25] = PORT_PCR_MUX(5) | PORT_PCR_ODE(1);

    //Escritura del Control Register para setearlo en modo master y transmisor.
    I2C_ptr->C1 = (I2C_C1_DMAEN(0) | I2C_C1_WUEN(0) | I2C_C1_RSTA(0)
                  | I2C_C1_TXAK(0) | I2C_C1_TX(1) | I2C_C1_IICIE(1)
                  | I2C_C1_IICEN(1));
	//Escritura del I2CF (frequency divider) usando la tabla del reference manual

    I2C_ptr->F = 0b10110101;// Seteo de frecuencia I2C

	// Initialize subroutine
	pisrRegister(I2C_subroutine, 100);
}

void kill_I2C (void)
{
    I2C_ptr->C1 &= I2C_C1_IICEN(0);
}

bool write_I2C (uint8_t writeAddress, uint8_t* writeData, uint8_t writeDataLength)
{
	dataByte_t auxData;

	auxData.byte = writeAddress << 1 | 0b0;// Set R/W bit 0
	auxData.dataLength = writeDataLength;
	auxData.selection = TX;
    i2c_PushQueue(auxData);	

    for (int i=0; i<writeDataLength; i++)
    {
        auxData.byte = writeData[i];
        auxData.dataLength = writeDataLength - i - 1;
        i2c_PushQueue(auxData);
    }

	// if(writeDataLength==3)
	// {
	// 	auxData.byte = writeData;
	// 	auxData.dataLength--;
	// 	PushQueue(auxData);
	// }
	start_I2C = true;
	return true;
}

bool read_I2C (uint8_t slaveAddress, uint8_t dataSize)
{
	dataByte_t auxData;

	auxData.byte = (slaveAddress << 1) | 0b1;
	auxData.selection = TX;
	auxData.dataLength = dataSize;
	i2c_PushQueue(auxData);

	auxData.selection = RX;
	for (int i = dataSize-1; i >= 0; i--)
	{
		auxData.dataLength = i;
		i2c_PushQueue(auxData);
	}
	return RX_READY; 
}

void I2C_subroutine (void)
{
	if((I2C_ptr->FLT) & I2C_FLT_STOPF_MASK)
	{
		I2C_ptr->FLT |= I2C_FLT_STOPF(1); 	// Clear stop flag
		I2C_ptr->S |= I2C_S_IICIF(1);		// Clear IICIF
		return;
	}
	else
	{
		if((I2C_ptr->FLT) & I2C_FLT_STARTF_MASK)
		{
			I2C_ptr->FLT |= I2C_FLT_STARTF(1);  // clear START flag
			I2C_ptr->S |= I2C_S_IICIF(1);		// clear IICIF
		}
		else
		{
			I2C_ptr->S |= I2C_S_IICIF(1);	// clear IICIF
		}
		if(isMaster())
		{
			if(i2c_QueueStatus() > 0)
			{
				if(isTx())
				{
					if(i2c_readQueueLength() > 0 && conclude == false)		// is last byte transmitted?
					{
						I2C_ptr->D = i2c_PullQueue();
					}
					else if(i2c_readQueueLength() == 0 && conclude == false)
					{
						I2C_ptr->D = i2c_PullQueue();
						conclude = true;
					}
					else if(conclude == true)
					{
						conclude = false;
						firstRX = true;
						I2C_ptr->C1 |= I2C_C1_RSTA_MASK; //generate repeated start condition
						I2C_ptr->C1 &= ~I2C_C1_TXAK_MASK;
					}
				}
				else if (firstRX)
				{
					(uint8_t)I2C_ptr->D;//dummy read
					firstRX = false;
				}
				else if(firstRX == false)
				{
					if((i2c_readQueueLength() >= 1) && (conclude == false))
					{	
						i2c_userPushQueue((uint8_t)I2C_ptr->D);
						i2c_PullQueue();
					}
					else if((i2c_readQueueLength() == 0) && (conclude == false))	// last byte to be read
					{
						i2c_userPushQueue((uint8_t)I2C_ptr->D);		// read data
						i2c_PullQueue();
						I2C_ptr->C1 |= I2C_C1_TXAK(1);
						conclude = true;
					}
					else if(conclude == true)
					{
						conclude = false;
						I2C_ptr->C1 |= I2C_C1_RSTA_MASK; //generate repeated start condition
						I2C_ptr->C1 &= ~I2C_C1_TXAK_MASK;
					}
				}
			}
			else
			{
				I2C_ptr->C1 &= ~I2C_C1_MST_MASK;		// generate stop signal
				I2C_ptr->C1 &= ~I2C_C1_TXAK_MASK;
				conclude = false;
				start_I2C = false;
			}
		}
	}
}

static uint8_t isMaster (void)
{
	if(start_I2C || i2c_QueueStatus())
	{
		I2C_ptr->C1 |= I2C_C1_MST(1);
		return 1;
	}
	return 0;
}

static uint8_t isTx (void)
{
	if(i2c_getQueueSelection() == RX)
	{
		I2C_ptr->C1 &= ~I2C_C1_TX_MASK; 	// Set receiver mode
		return 0;
	}
	else
	{
		I2C_ptr->C1 |= I2C_C1_TX_MASK;	// Set transmit mode
		return 1;
	}
}

void getData_I2C (unsigned char *arr) 	// carga en un arreglo provisto por el usuario la cantidad "bytes" de datos, si no hay esa cantidad de datos en la cola, devuelve 0.
{
	i2c_getUserData(arr);
	return;
}

unsigned char getUserQueueLength (void)
{
	return i2c_userQueueStatus();
}


/*
TODO: Detectar cuando se está mandando un byte de address, para setearle el 0 o 1 en W/R
*/
