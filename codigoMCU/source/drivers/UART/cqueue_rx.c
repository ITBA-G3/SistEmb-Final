/***************************************************************************/ /**
   @file     cqueue.c
   @brief    Circular Queue
   @author   JACOBY, D.
  ******************************************************************************/

#include "cqueue_rx.h"
#include <stdint.h>
#define RXQSIZE 511 // Queue size

static unsigned char buffer[RXQSIZE]; // storage for circuar queue
static uint16_t news;          // How many "news" are remaining in Queue
static unsigned char *pin, *pout;   // input and output pointers

/*
 Initialize queue
*/
void RXQueueInit(void)

{

    pin = buffer; // set up pin,pout and "news" counter
    pout = buffer;
    news = 0;
}

/*
  Push data on queue
*/
unsigned char RXPushQueue(unsigned char data)
{

    if (news > RXQSIZE - 1) // test for Queue overflow
    {
        news = RXQOVERFLOW; // inform queue has overflowed
        return (news);
    }

    *pin++ = data; // pull data
    news++;        // update "news" counter

    if (pin == buffer + RXQSIZE) // if queue size is exceded reset pointer
        pin = buffer;

    return (news); // inform Queue state
}

/*
  Retrieve data from queue
*/
unsigned char RXPullQueue(void)
{
	if(news>0){
    unsigned char data;

    data = *pout++; // pull data
    news--;         // update "news" counter

    if (pout == buffer + RXQSIZE) // Check for Queue boundaries
        pout = buffer;          // if queue size is exceded reset pointer
    return (data);              // rerturn retrieved data
	}
}

/*
  Get queue Status
*/
unsigned char RXQueueStatus(void)

{
    return (news); // Retrieve "news" counter
}
