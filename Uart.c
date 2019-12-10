/**
 *File name: Uart.c
 *School: Dalhousie University
 *Department: Electrical and computer engineering
 *Course: Real time system
 *Professor: Larry Hughes
 *Purpose: this file includes all the settings of UART, to enable the interrupt and build the handler.
 *          this is kind of upversion of assignment 2.
 *Author:   Ray     Su  B00634512
            Victor  Gao B00677182
 *Last day modified: Nov.04
 *  From Assignment 1
 */

#include <stdlib.h>
#include "Uart.h"
#include "Queue.h"
#include "PKCall.h"
#include "PhysicalLayer.h"

#define STX 0x02
#define ETX 0x03
#define DLE 0x10

volatile int UART0_STATUS = IDLE;
volatile int UART1_STATUS = IDLE;
int DATA_COUNT = 0;
int STX_RECEIVED = FALSE;
int DLE_RECEIVED = FALSE;
frame* RECEIVED_FRAME = NULL;

extern void SendCall(SendMsgArgs* args);

void UART0_Init(void)
{
	volatile int wait;

	/* Initialize UART0 */
	SYSCTL_RCGCGPIO_R |= SYSCTL_RCGCUART_GPIOA;   // Enable Clock Gating for UART0
	SYSCTL_RCGCUART_R |= SYSCTL_RCGCGPIO_UART0;   // Enable Clock Gating for PORTA
	wait = 0; // give time for the clocks to activate

	UART0_CTL_R &= ~UART_CTL_UARTEN;        // Disable the UART
	wait = 0;   // wait required before accessing the UART config regs

	// Setup the BAUD rate
	UART0_IBRD_R = 8;   // IBRD = int(16,000,000 / (16 * 115,200)) = 8.680555555555556
	UART0_FBRD_R = 44;  // FBRD = int(.680555555555556 * 64 + 0.5) = 44.05555555555556

	UART0_LCRH_R = (UART_LCRH_WLEN_8);  // WLEN: 8, no parity, one stop bit, without FIFOs)

	GPIO_PORTA_AFSEL_R = 0x3;        // Enable Receive and Transmit on PA1-0
	GPIO_PORTA_PCTL_R = (0x01) | ((0x01) << 4);         // Enable UART RX/TX pins on PA1-0
	GPIO_PORTA_DEN_R = EN_DIG_PA0 | EN_DIG_PA1;        // Enable Digital I/O on PA1-0

	UART0_CTL_R = UART_CTL_UARTEN;        // Enable the UART
	wait = 0; // wait; give UART time to enable itself.
}

void UART1_Init(void)
{
	volatile int wait;

	/* Initialize UART1 */
	SYSCTL_RCGCGPIO_R |= SYSCTL_RCGCUART_GPIOB;   // Enable Clock Gating for UART0
	SYSCTL_RCGCUART_R |= SYSCTL_RCGCGPIO_UART1;   // Enable Clock Gating for PORTA
	wait = 0; // give time for the clocks to activate

	UART1_CTL_R &= ~UART_CTL_UARTEN;        // Disable the UART
	wait = 0;   // wait required before accessing the UART config regs

	// Setup the BAUD rate
	UART1_IBRD_R = 8;   // IBRD = int(16,000,000 / (16 * 115,200)) = 8.680555555555556
	UART1_FBRD_R = 44;  // FBRD = int(.680555555555556 * 64 + 0.5) = 44.05555555555556

	UART1_LCRH_R = (UART_LCRH_WLEN_8);  // WLEN: 8, no parity, one stop bit, without FIFOs)

	GPIO_PORTB_AFSEL_R = 0x3;        // Enable Receive and Transmit on PA1-0
	GPIO_PORTB_PCTL_R = (0x01) | ((0x01) << 4);         // Enable UART RX/TX pins on PA1-0
	GPIO_PORTB_DEN_R = EN_DIG_PB0 | EN_DIG_PB1;        // Enable Digital I/O on PA1-0

	UART1_CTL_R = UART_CTL_UARTEN;        // Enable the UART
	wait = 0; // wait; give UART time to enable itself.
}

void InterruptEnable(unsigned long InterruptIndex)
{
	/* Indicate to CPU which device is to interrupt */
	if (InterruptIndex < 32)
		NVIC_EN0_R = 1 << InterruptIndex;       // Enable the interrupt in the EN0 Register
	else
		NVIC_EN1_R = 1 << (InterruptIndex - 32);    // Enable the interrupt in the EN1 Register
}

void UART0_IntEnable(unsigned long flags)
{
	/* Set specified bits for interrupt */
	UART0_IM_R |= flags;
}

void UART0_IntDisable(unsigned long flags)
{
	/* Clear specified bits for interrupt */
	UART0_IM_R &= ~flags;
}

void UART0_IntHandler(void)
{
	/*
	 * Simplified UART ISR - handles receive and xmit interrupts
	 * Application signalled when data received
	 */
	if (UART0_MIS_R & UART_INT_RX)
	{
		/* RECV done - clear interrupt and make char available to application */
		UART0_ICR_R |= UART_INT_RX;
		// no receive function for uart 0
	}

	if (UART0_MIS_R & UART_INT_TX)
	{
		/* XMIT done - clear interrupt */
		UART0_ICR_R |= UART_INT_TX;

		char data;
		if (DeQueueIO(UART0, UART, &data)) // if output queue is not empty
			UART0_DR_R = data;  // transmit next data
		else // if output queue is empty
			UART0_STATUS = IDLE; // idle
	}
}

void UART1_IntEnable(unsigned long flags)
{
	/* Set specified bits for interrupt */
	UART1_IM_R |= flags;
}

void UART1_IntDisable(unsigned long flags)
{
	/* Clear specified bits for interrupt */
	UART1_IM_R &= ~flags;
}

void UART1_IntHandler(void)
{
	/*
	 * Simplified UART ISR - handles receive and xmit interrupts
	 * Application signalled when data received
	 */
	if (UART1_MIS_R & UART_INT_RX)
	{
		/* RECV done - clear interrupt and make char available to application */
		UART1_ICR_R |= UART_INT_RX;

		// check received message
		char data = UART1_DR_R;

		if (data == STX && !STX_RECEIVED) // if received STX and not in recording
		{
			STX_RECEIVED = TRUE;
			RECEIVED_FRAME = (frame*)malloc(sizeof(frame));
			RECEIVED_FRAME->frm[DATA_COUNT++] = data;
			DATA_COUNT = 0; // reset data_count
		}
		else if (STX_RECEIVED) // if in recording
		{
			if (DATA_COUNT>=FRAME_MAXSIZE) // if over size, clear and return
			{
				STX_RECEIVED = FALSE;
				DLE_RECEIVED = FALSE;
				DATA_COUNT = 0;
				free(RECEIVED_FRAME); // free memory
				return;
			}

			if (DLE_RECEIVED) // if received DLE last byte
			{
				RECEIVED_FRAME->frm[DATA_COUNT++] = data; // record data anyway
				DLE_RECEIVED = FALSE;
			}
			else if (data == DLE) // if received DLE
				DLE_RECEIVED = TRUE;
			else if (data == ETX) // if received ETX
			{
				STX_RECEIVED = FALSE;
				RECEIVED_FRAME->frm[DATA_COUNT++] = data; // record data anyway
				RECEIVED_FRAME->length = DATA_COUNT; // record length

				// send received frame to received processor
				int sz = sizeof(&RECEIVED_FRAME);
				SendMsgArgs arg = { RECEIVED_PORCESSOR_MBX, UART1_MBX, &RECEIVED_FRAME, &sz };//the receive function who depend on this mailbox will be unblock
				SendCall(&arg);
			}
			else // record data
			    RECEIVED_FRAME->frm[DATA_COUNT++] = data;
		}
	}

	if (UART1_MIS_R & UART_INT_TX)
	{
		/* XMIT done - clear interrupt */
		UART1_ICR_R |= UART_INT_TX;

		char data;
		if (DeQueueIO(UART1, UART, &data)) // if output queue is not empty
			UART1_DR_R = data;  // transmit next data
		else // if output queue is empty
			UART1_STATUS = IDLE; // idle
	}
}

void InterruptMasterEnable(void)
{
	/* enable CPU interrupts */
	__asm(" cpsie   i");
}

void InterruptMasterDisable(void)
{
	/* disable CPU interrupts */
	__asm(" cpsid   i");
}

