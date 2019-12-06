/*
 *File name: Process.c
 *School: Dalhousie University
 *Department: Electrical and computer engineering
 *Course: Real time system
 *Professor: Larry Hughes
 *Purpose: includes registration of process that can put process in the running queue. And processes itself.
 *Author:   Ray     Su  B00634512
            Victor  Gao B00677182
 *Last day modified: Nov.20
 */
#include <stdio.h>
#include <stdlib.h>
#include "Queue.h"
#include "process.h"
#include "KernelCall.h"
#include "PKCall.h"
#include "Uart.h"
#include "QueueFuncs.h"
#include "ApplicationLayer.h"

#define PSR_INITIAL_VAL		0x01000000 // PSR initial value
#define INITIAL_STACK_TOP_OFFSET    960 //stack top offset of stack pointer
#define UART_OUTPUT_MBX	20 //Uart output mailbox number
#define PID_IDLE		0 //process id Idle
#define PID_UART		4 //process id Uart
#define PRIORITY_3		3  //priority 3
#define PRIORITY_4		4  //priority 4
#define PRIORITY_UART	5  //priority Idle
#define PRIORITY_IDLE	0  //priority Uart


// create and initialize priority list
PCB* PRIORITY_LIST[PRIORITY_LIST_SIZE] = {NULL, NULL, NULL, NULL, NULL, NULL};

// RUNNING Pcb
volatile PCB* RUNNING = NULL;

// function of idle process
void process_IDLE()
{
	while (1);
}

// Uart output process
void process_UART0_OUTPUT()
{
	int msg = 0;
	int size = sizeof(msg);
	int sender;// null_sender, null_msg, null_size;
	Bind(UART_OUTPUT_MBX); // bind mailbox

	while (TRUE) // keep checking mailbox
	{
		Receive(UART_OUTPUT_MBX, &sender, &msg, &size); // get message
		OutputData((char*)&msg, sizeof(msg)); // output message
	}

}

// registrate process
int reg_process(void (*func_name)(), int pid, int priority)
{
	PCB* pcb = (PCB*)malloc(sizeof(PCB)); // Allocate memory for pcb
	pcb->PID = pid; // Assign ID
	pcb->Priority = priority; // Assign priority
	pcb->StackTop = malloc(STACKSIZE);
	pcb->Msg_Wait = NULL; // clear message wait
	pcb->Mailbox_Head = NULL; // clear mailbox head

	// Stack should grow from bottom
	pcb->PSP = (Stack*)((unsigned long)pcb->StackTop + INITIAL_STACK_TOP_OFFSET);

	pcb->PSP->PSR = PSR_INITIAL_VAL; // Assign PSR initial value
	pcb->PSP->PC = (unsigned long)func_name; // Assign process's function to PC
	pcb->PSP->LR = (unsigned long)Terminate; // Assign terminate function to LR

	// Assign all other registers to null
	pcb->PSP->R0 = NULL;
	pcb->PSP->R1 = NULL;
	pcb->PSP->R2 = NULL;
	pcb->PSP->R3 = NULL;
	pcb->PSP->R4 = NULL;
	pcb->PSP->R5 = NULL;
	pcb->PSP->R6 = NULL;
	pcb->PSP->R7 = NULL;
	pcb->PSP->R8 = NULL;
	pcb->PSP->R9 = NULL;
	pcb->PSP->R10 = NULL;
	pcb->PSP->R11 = NULL;
	pcb->PSP->R12 = NULL;

	Enqueue(pcb, (QueueItem**)&(PRIORITY_LIST[pcb->Priority])); // Add to queue

	if ((RUNNING == NULL) || (priority > RUNNING->Priority)) 
		// if is the first process or has higher priority, assign it to RUNNING
		RUNNING = pcb;

	return TRUE;
}

// return pcb pointer of the first process below input priority
PCB* CheckLowerPriorityProcess(int priority)
{
	int i;
	for (i = priority - 1; i >= 0; i--) // check lower priority
	{
		if (PRIORITY_LIST[i] != NULL) // if find one
			return PRIORITY_LIST[i]; // return found process
	}
	return NULL; // return null if no process left, includes idle process
}

unsigned long get_PSP(void)
{
	/* Returns contents of PSP (current process stack */
	__asm(" mrs     r0, psp");
	__asm(" bx  lr");
	return 0;   /***** Not executed -- shuts compiler up */
			/***** If used, will clobber 'r0' */
}

unsigned long get_MSP(void)
{
	/* Returns contents of MSP (main stack) */
	__asm(" mrs     r0, msp");
	__asm(" bx  lr");
	return 0;
}

void set_PSP(volatile unsigned long ProcessStack)
{
	/* set PSP to ProcessStack */
	__asm(" msr psp, r0");
}

void set_MSP(volatile unsigned long MainStack)
{
	/* Set MSP to MainStack */
	__asm(" msr msp, r0");
}

void volatile save_registers()
{
	/* Save r4..r11 on process stack */
	__asm(" mrs     r0,psp");
	/* Store multiple, decrement before; '!' - update R0 after each store */
	__asm(" stmdb   r0!,{r4-r11}");
	__asm(" msr psp,r0");
}

void volatile restore_registers()
{
	/* Restore r4..r11 from stack to CPU */
	__asm(" mrs r0,psp");
	/* Load multiple, increment after; '!' - update R0 */
	__asm(" ldmia   r0!,{r4-r11}");
	__asm(" msr psp,r0");
}

unsigned long get_SP()
{
	/**** Leading space required -- for label ****/
	__asm("     mov     r0,SP");
	__asm(" bx  lr");
	return 0;
}

void Train_1_Application_Process()
{
	int mbx = Bind(LOCOMOTIVE_1); // bind mailbox
	Message_QueueItem* first_msg = NULL; // create message queue head

	// create route
	program route1 = { 13,
	GO, CW, 4, 3, /* Go CW @ speed 4 to HS#3 */
	HALT,
	SWITCH, 0, DIVERGED, /* Switch '0' to diverged */
	GO, CW, 2, 21,
	END };

	// convert program to message queue
	if (GenerateMessages(&route1, LOCOMOTIVE_1, &first_msg)) // if success
	{

	}
	else // if failed
	{

	}
}


// Initialize all processes and force switch to thread mode
void Initialize_Process()
{
	reg_process(process_IDLE, PID_IDLE, PRIORITY_IDLE); // register idle process
	reg_process(Train_1_Application_Process, LOCOMOTIVE_1, PRIORITY_3); // register process 1
	reg_process(process_UART0_OUTPUT, PID_UART, PRIORITY_UART); // fegister uart output process
}
