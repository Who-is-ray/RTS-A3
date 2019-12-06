/*
 *File name: Queue.c
 *School: Dalhousie University
 *Department: Electrical and computer engineering
 *Course: Real time system
 *Professor: Larry Hughes
 *Purpose: a queue that support the uart to store the message that wait to assigned to uart data register.
 *Author:   Ray     Su  B00634512
            Victor  Gao B00677182
 *Last day modified: Nov.19
 */

#include <string.h>
#include "Queue.h"
#include "Uart.h"

extern int UART_STATUS;

Queue InQ_UART0;
Queue OutQ_UART0;

void Queue_Init()
{//initialize both input and output queue
    InQ_UART0.Head=0;//input queue's head
    InQ_UART0.Tail=0;//input queue's tail
    OutQ_UART0.Head=0;//output queue's head
    OutQ_UART0.Tail=0;//output queue's tail
}

int EnQueueIO(QueueType t, Source s, char v)
{
    int head;
    switch (t)
    {
        case INPUT:
        {
            head = InQ_UART0.Head;
            if(((head+1) & QSM1) != InQ_UART0.Tail) // if not full
            {
                (InQ_UART0.queue[head]).value = v;
                InQ_UART0.queue[head].source = s;
                InQ_UART0.Head=(head+1)&QSM1;
                return TRUE;
            }
            break;
        }
        case OUTPUT:
        {
            head = OutQ_UART0.Head;
            if(((head+1) & QSM1) != OutQ_UART0.Tail)  // if not full
            {
                UART0_IntDisable(UART_INT_TX); // disable UART transmit interrupt
                if(UART_STATUS == BUSY) // if uart is busy
                {
                    // add to queue
                    OutQ_UART0.queue[OutQ_UART0.Head].value = v;
                    OutQ_UART0.Head=(head+1)&QSM1;
                }
                else // uart not busy
                {
                    // directly output, set to busy
                    UART_STATUS = BUSY;
                    UART0_DR_R = v;
                }
                UART0_IntEnable(UART_INT_TX); // enable UART transmit interrupt
                return TRUE;
            }
            break;
        }
    }
    return FALSE;
}

int DeQueueIO(QueueType t, Source* s, char* v)
{
    int tail;
    switch (t)
    {
        case INPUT:
        {
            tail = InQ_UART0.Tail;
            if(InQ_UART0.Head!=tail)  // if not empty, dequeue
            {
                *s=InQ_UART0.queue[tail].source;
                *v=InQ_UART0.queue[tail].value;
                InQ_UART0.Tail = (tail+1)&QSM1;    // Update tail
            }
            else    // if empty
                return FALSE;

            break;
        }

        case OUTPUT:
        {
            tail = OutQ_UART0.Tail;
            if(OutQ_UART0.Head!=tail)  // if not empty, dequeue
            {
                *v=OutQ_UART0.queue[tail].value;
                OutQ_UART0.Tail = (tail+1)&QSM1;    // Update tail
            }
            else    // if empty
                return FALSE;

            break;
        }
    }
    return TRUE;
}

/* Transmit a character*/
void TransChar(char c, int uart_id)
{
	switch (uart_id)
	{
	case UART0:
	}
	while (EnQueueIO(OUTPUT, UART, c) == FALSE); // wait until it is enqueued
}

/* Output a string*/
void OutputData(char* s, int size, int uart_id)
{
	int i;
	for (i = 0; i < size; i++) // output each character in string
	{
		TransChar(*s, uart_id);
		s++;
	}
}
