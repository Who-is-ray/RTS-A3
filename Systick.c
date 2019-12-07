/*
 *File name: Systick.c
 *School: Dalhousie University
 *Department: Electrical and computer engineering
 *Course: Real time system
 *Professor: Larry Hughes
 *Purpose: Initialization the systick, and provide the systick to support the switch process.
 *Author:   Ray     Su  B00634512
            Victor  Gao B00677182
 *Last day modified: Sep 28
 *From Assignment 1
 */

#include "Systick.h"
#include "PKCall.h"
#include "Queue.h"
#include "Uart.h"


#define NVIC_INT_CTRL_R (*((volatile unsigned long *) 0xE000ED04))
#define TRIGGER_PENDSV 0x10000000
#define RESEND_CYCLE	50 //resend every 50 timecycle

volatile int PENDSV_ON;
extern volatile int RESEND_COUNTING;
extern volatile int RESEND_MBX; // MAILBOX to receive resend notice from systick
int counting_time = 0;

extern void SendCall(SendMsgArgs* args);

void SysTickStart(void)
{
    // Set the clock source to internal and enable the counter to interrupt
    ST_CTRL_R |= ST_CTRL_CLK_SRC | ST_CTRL_ENABLE;
}

void SysTickStop(void)
{
    // Clear the enable bit to stop the counter
    ST_CTRL_R &= ~(ST_CTRL_ENABLE);
}

void SysTickPeriod(unsigned long Period)
{
    /*
     For an interrupt, must be between 2 and 16777216 (0x100.0000 or 2^24)
    */
    ST_RELOAD_R = Period - 1;  /* 1 to 0xff.ffff */
}

void SysTickIntEnable(void)
{
    // Set the interrupt bit in STCTRL
    ST_CTRL_R |= ST_CTRL_INTEN;
}

void SysTickIntDisable(void)
{
    // Clear the interrupt bit in STCTRL
    ST_CTRL_R &= ~(ST_CTRL_INTEN);
}

void SysTickHandler(void)
{
	if (RESEND_COUNTING)
		counting_time++;
	if (counting_time >= RESEND_CYCLE)
	{
		RESEND_COUNTING = FALSE;
		counting_time = 0;
		int msg = NULL;
		int sz = sizeof(msg);
		SendMsgArgs arg = { RESEND_MBX, SYSTICK_MBX, &msg, &sz };
		SendCall(&arg);
	}

    if (PENDSV_ON == FALSE)
        NVIC_INT_CTRL_R |= TRIGGER_PENDSV;// Process switch, trigger PendSV after this function
}

void SysTickInit()
{
    SysTickPeriod(MAX_WAIT);
    SysTickIntEnable();
    SysTickStart();
}
