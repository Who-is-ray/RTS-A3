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
#include "DataLinkLayer.h"
#include "PhysicalLayer.h"

#define PSR_INITIAL_VAL		0x01000000 // PSR initial value
#define INITIAL_STACK_TOP_OFFSET    960 //stack top offset of stack pointer
#define UART0_OUTPUT_MBX	20	//Uart0 output mailbox number
#define UART1_OUTPUT_MBX	21	//Uart1 output mailbox number
#define PID_IDLE		0	//process id Idle
#define PID_UART0		4	//process id Uart0
#define PID_UART1       5   //process id Uart1
#define PRIORITY_3		3	//priority 3
#define PRIORITY_4		4	//priority 4
#define PRIORITY_UART	5	//priority Idle
#define PRIORITY_IDLE	0	//priority Uart
#define TWO_ARGS		3	//two arguments message length
#define ONE_ARG			2	//one argument message length
#define NO_ARG			1	//no argument message length

// create and initialize priority list
PCB* PRIORITY_LIST[PRIORITY_LIST_SIZE] = {NULL, NULL, NULL, NULL, NULL, NULL};

// RUNNING Pcb
volatile PCB* RUNNING = NULL;

volatile char Ns = 0;
volatile char Nr = 0;
frame privious_frame;
frame current_frame;

// function of idle process
void process_IDLE()
{
	while (1);
}

// Uart0 output process
void process_UART0_OUTPUT()
{
	char msg = 0;
	int size = sizeof(msg);
	int sender;// null_sender, null_msg, null_size;
	Bind(UART0_OUTPUT_MBX); // bind mailbox

	while (TRUE) // keep checking mailbox
	{
		Receive(UART0_OUTPUT_MBX, &sender, &msg, &size); // get message
		OutputData((char*)&msg, sizeof(msg), UART0); // output message
	}

}

// Uart1 output process
void process_UART1_OUTPUT()
{
	frame* msg = NULL;
	int size = sizeof(msg);
	int sender;// null_sender, null_msg, null_size;
	Bind(UART1_OUTPUT_MBX); // bind mailbox

    //char m[8] = {0x02,0,0x03,0xe0,0x05,0x01,0xe9,0x03}; // switch 5 command text

	while (TRUE) // keep checking mailbox
	{
		Receive(UART1_OUTPUT_MBX, &sender, &msg, &size); // get message
		OutputData(msg->frm, msg->length, UART1); // output message

        //OutputData(m, 8, UART1); // output switch 5 divert

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

// Function to encode message to packet, encode packet to frame, send frame and wait for ack
void SentMessage(int msg_len, Message* msg, int locomotive)
{
	// encode to packet
	packet pkt;
	EncodeMsgToPacket((char*)msg, msg_len, &pkt);

	// encode to current frame
	privious_frame = current_frame; // save last frame
	EncodePacketToFrame(&pkt, &current_frame);

	// send frame
	frame* to_send = &current_frame;
	int sz = sizeof(&current_frame);
	Send(UART1_OUTPUT_MBX, locomotive, &to_send, &sz); // output message
	Ns++; // update Ns

	// wait ack

}

// function to run the program
int Run_machine(program* prog, int locomotive)
{
	/* Follows instructions in supplied program (the route) */
	unsigned int pc;
	unsigned int curr_spd = NULL, curr_dir = CW, destination = NULL;

	pc = 0;
	while (pc < prog->length && pc < PROGRAM_MAXSIZE && prog->action[pc] != END)
	{
		printf("%d: ", prog->action[pc]);
		switch (prog->action[pc])
		{
		case GO: /* Go to HS# to dir and spd */
		{
			printf("GO: ");
			pc++;
			curr_dir = prog->action[pc++];		
			curr_spd = prog->action[pc++];
			destination = prog->action[pc];

			printf("Direction: %s Speed: %d HS: %d\n",
				curr_dir == CW ? "CW" : "CCW", curr_spd, destination);

			//create message
			mag_dir speed = { curr_spd, IGNORED, curr_dir };
			Message msg = { LOCOMOTIVE_CONTROLER, locomotive};
			memcpy(&msg.arg2,&speed, sizeof(msg.arg2));

			SentMessage(TWO_ARGS, &msg, locomotive);

			char msg_rec = '2';
			int size_rec = sizeof(msg_rec);
			Send(UART0_OUTPUT_MBX, locomotive, &msg_rec, &size_rec);

			while (TRUE)
			{
				int hole_sensor = NULL;
				int sz = sizeof(hole_sensor);
				int sender;
				Receive(locomotive, &sender, &hole_sensor, &sz); // receive message

				if (sender == RECEIVED_PORCESSOR_MBX) // if reached a hole sensor
				{
					// send acknowledgement

					if (hole_sensor == destination) // if reached destination
						break;
				}
				//else if (sender == ) // if haven't received acknowledgement in time cycle
				//{
				//	// Resend message

				//}
			}

			break;
		}
		case SWITCH: /* Throw specific switch */
		{
			printf("SWITCH: ");
			pc++;
			printf("Switch: %d %s\n", prog->action[pc++],
				prog->action[pc] ? "STRAIGHT" : "DIVERGED");
			break;
		}
		case HALT: /* Halt the train */
		{
			printf("HALT\n");
			curr_spd = 0;
			printf("Speed to zero\n");
			break;
		}
		case PAUSE: /* Pause the train for # second */
		{
			printf("PAUSE\n");
			pc++;
			printf("Speed to zero\n");
			printf("Wait for %d seconds\n", prog->action[pc]);
			printf("Set speed to: %d\n", curr_spd);
			break;
		}
		default:
			printf("Unknown inst at pc: %d", pc);
		}
		pc++;
	}
	printf("End of program\n");

	return TRUE;
}

void Train_1_Application_Process()
{
	int mbx = Bind(LOCOMOTIVE_1); // bind mailbox

	// create route
	program route = { 13,
	GO, CW, 4, 3, /* Go CW @ speed 4 to HS#3 */
	HALT,
	SWITCH, 0, DIVERGED, /* Switch '0' to diverged */
	GO, CW, 2, 21,
	END };

	Run_machine(&route, LOCOMOTIVE_1);
}

/* The process to manage the received message from trainset*/
void Received_Message_Processor()
{
	int mbx = Bind(RECEIVED_PORCESSOR_MBX); // bind mailbox
	frame* received_frame = NULL;
	int size = sizeof(received_frame);
	int sender = NULL;
	while (TRUE)
	{
		Receive(RECEIVED_PORCESSOR_MBX, &sender, &received_frame, &size); // check if message arrived

		// Varify checksum
		
	}
}

// Initialize all processes and force switch to thread mode
void Initialize_Process()
{
	reg_process(process_IDLE, PID_IDLE, PRIORITY_IDLE); // register idle process
	reg_process(Train_1_Application_Process, LOCOMOTIVE_1, PRIORITY_3); // register process 1
    reg_process(process_UART0_OUTPUT, PID_UART0, PRIORITY_UART); // fegister uart output process
    reg_process(process_UART1_OUTPUT, PID_UART1, PRIORITY_UART); // fegister uart output process
}
