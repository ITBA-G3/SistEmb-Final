/***************************************************************************/ /**
   @file     cqueue.h
   @brief    Circular Queue
   @author   JACOBY, D.
  ******************************************************************************/

#ifndef CQUEUE_RX_H
#define CQUEUE_RX_H

void RXQueueInit(void);
unsigned char RXPushQueue(unsigned char data);
unsigned char RXPullQueue(void);
unsigned char RXQueueStatus(void);
#define RXQOVERFLOW 512

#endif // CQUEUE_RX_H
