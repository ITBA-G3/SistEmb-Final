/*******************************************************************************
 * INCLUDE HEADER FILES
 ******************************************************************************/

#include "cqueue.h"
#include "MK64F12.h" // o lo que uses para __disable_irq/__enable_irq

static inline uint32_t irq_save(void){ uint32_t prim = __get_PRIMASK(); __disable_irq(); return prim; }
static inline void irq_restore(uint32_t prim){ if(!prim) __enable_irq(); }

/*******************************************************************************
 * CONSTANT AND MACRO DEFINITIONS USING #DEFINE
 ******************************************************************************/

#define QSIZE  0xFE	// Queue size (must be less than 0xFF)	

/*******************************************************************************
 * FUNCTION PROTOTYPES WITH LOCAL SCOPE
 ******************************************************************************/

static unsigned char i2c_userPullQueue (void);

/*******************************************************************************
 * VARIABLES WITH LOCAL SCOPE
 ******************************************************************************/

static dataByte_t buffer[QSIZE];  // storage for circuar queue
static unsigned char news;	    // How many "news" are remaining in Queue
static dataByte_t *pin,*pout;     // input and output pointers 	

static dataByte_t bufferUser[QSIZE];  // storage for circuar queue
static unsigned char newsUser;	    // How many "news" are remaining in Queue
static dataByte_t *pinUser,*poutUser;     // input and output pointers 	

/*
 Initialize queue 
*/
void i2c_QueueInit(void)
{
	pin=buffer;	//set up pin,pout and "news" counter 	
	pout=buffer;
	news=0;

	pinUser=bufferUser;	//set up pin,pout and "news" counter 	
	poutUser=bufferUser;
	newsUser=0;
}

/*
  Push data on queue 
*/
unsigned char i2c_PushQueue(dataByte_t data)
{	
    uint32_t prim = irq_save();

	if (news > QSIZE-1)		//test for Queue overflow
	{
		news=QOVERFLOW;		// inform queue has overflowed
		return news;		
	}	

	*pin++=data;			// pull data
	(news)++;				// update "news" counter

	if (pin == buffer+QSIZE)	// if queue size is exceded reset pointer
		pin=buffer;

    irq_restore(prim);
	return(news);			// inform Queue state
}

/*
  Retrieve data from queue 
*/
unsigned char i2c_PullQueue(void)
{
    uint32_t prim = irq_save();

	dataByte_t *data;
	data=pout;			// Aux data pointer
	pout++;
	news--;				// update "news" counter

	if (pout == buffer+QSIZE)	// Check for Queue boundaries
		pout=buffer;		// if queue size is exceded reset pointer

    irq_restore(prim);
	return (data->byte);			// rerturn retrieved data
}

unsigned char i2c_userPushQueue(unsigned char data)
{
	if (newsUser > QSIZE-1)		//test for Queue overflow
	{
		newsUser = QOVERFLOW;		// inform queue has overflowed
		return newsUser;		
	}	

	pinUser->byte = data;			// pull data
	pinUser->dataLength = pout->dataLength;			// pull data
	pinUser++;
	newsUser++;				// update "news" counter

	if (pinUser == bufferUser+QSIZE)	// if queue size is exceded reset pointer
		pinUser = bufferUser;

	return newsUser;
}

void i2c_getUserData (unsigned char *data)
{
	unsigned char* p = data;
	int n = poutUser->dataLength;
	for(int i = 0; i <= n; i++)
	{
		*p = i2c_userPullQueue();
		p++;
	}
}

static unsigned char i2c_userPullQueue (void)
{
	unsigned char data;
	data = poutUser->byte;			// pull data
	poutUser++;
	newsUser--;				// update "news" counter

	if (poutUser == bufferUser+QSIZE)	// Check for Queue boundaries
		poutUser=bufferUser;		// if queue size is exceded reset pointer

	return data;			// rerturn retrieved data
}

/*
  Get queue Status
*/
unsigned char i2c_QueueStatus(void)
{
	return news;			// Retrieve "news" counter		
}

unsigned char i2c_userQueueStatus(void)
{
	return newsUser;
}

unsigned char i2c_readQueueLength(void)
{
	return pout->dataLength;
}

unsigned char i2c_getQueueSelection(void)
{
	return pout->selection;
}



