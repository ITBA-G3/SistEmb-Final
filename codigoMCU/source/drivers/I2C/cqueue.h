#ifndef CQUEUE_H
#define CQUEUE_H

/*******************************************************************************
 * CONSTANT AND MACRO DEFINITIONS USING #DEFINE
 ******************************************************************************/

#define QOVERFLOW 0xFF

/*******************************************************************************
 * ENUMERATIONS AND STRUCTURES AND TYPEDEFS
 ******************************************************************************/

enum buffer {RX, TX};

typedef struct{
	unsigned char byte;			// In case of RX, copy data to RX buffer before pulling struct from buffer
	unsigned char dataLength;		// :)
	unsigned char selection;		// TX o RX
} dataByte_t;

/*******************************************************************************
 * FUNCTION PROTOTYPES WITH GLOBAL SCOPE
 ******************************************************************************/

void i2c_QueueInit(void);
// unsigned char PushQueue(unsigned char data, unsigned char selection);
unsigned char i2c_PushQueue(dataByte_t data);
unsigned char i2c_PullQueue(void);
unsigned char i2c_userPushQueue(unsigned char data);
unsigned char i2c_QueueStatus();
unsigned char i2c_userQueueStatus(void);
unsigned char i2c_readQueueLength(void);
unsigned char i2c_getQueueSelection(void);
void i2c_getUserData (unsigned char *data);


#endif
