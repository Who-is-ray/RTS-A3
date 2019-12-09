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
#include "Systick.h"
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
#define TO_SECOND		2	//Half second to full second

// create and initialize priority list
PCB* PRIORITY_LIST[PRIORITY_LIST_SIZE] = {NULL, NULL, NULL, NULL, NULL, NULL};

// RUNNING Pcb
volatile PCB* RUNNING = NULL;

volatile int RESEND_COUNTING = FALSE; // flag for systick to count for resend
volatile int RESEND_MBX; // MAILBOX to receive resend notice from systick

volatile char Ns = 0; // receive seq #
volatile char Nr = 0; // send seq #
frame privious_frame;
frame current_frame;

typedef struct
{
	PktType type;
	Message* msg;
}received_msg;

// function of idle process
void process_IDLE()
{
	while (1);
}

// Uart0 output process
void process_UART0_OUTPUT()
{
	int msg = 0;
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

void SendFrame(frame* to_send, int locomotive)
{
	int sz = sizeof(&to_send);
	Send(UART1_OUTPUT_MBX, locomotive, &to_send, &sz); // output message
	Ns++; // update Ns
	Ns &= NR_AND; // limit Ns in 3 bit, value from 0 to 7

	// Notice Systick
	RESEND_COUNTING = TRUE;
	RESEND_MBX = locomotive;

	// wait ack
	int sender = ERROR;
	received_msg* rec_msg;
	int sz_msg = sizeof(rec_msg);
	int rec_ack = FALSE;
	while (!rec_ack)
	{
		Receive(locomotive, &sender, &rec_msg, &sz_msg); // wait to receive ack or resend, then blocked here, go idle
		if (sender == RECEIVED_PORCESSOR_MBX) // if received message from trainset
		{
			if (rec_msg->type == ACK) // if received ack
				rec_ack = TRUE;
		}

		else if (sender == SYSTICK_MBX) // if time to resend
		{
			//resend message
			RESEND_COUNTING = TRUE;
			Send(UART1_OUTPUT_MBX, locomotive, &to_send, &sz); // output message
		}

		free(rec_msg); // release memory
	}
}

// Function to encode message to packet, encode packet to frame, send frame and wait for ack
void SentMessage(int msg_len, Message* msg, int locomotive)
{
	// encode to packet
	packet pkt;
	EncodeMsgToPacket((char*)msg, msg_len, &pkt);

	// encode to current frame
	privious_frame = current_frame; // save last frame
	EncodePacketToFrame(&pkt, &current_frame);// store data to current frame

	// send frame
	frame* to_send = &current_frame;//create an address, same as sendframe(&current_frame,...)
	SendFrame(to_send, LOCOMOTIVE_1);
}

// function to run the program
int Run_machine(program* prog, int locomotive)
{
	/* Follows instructions in supplied program (the route) */
	unsigned int pc;// a counter
	unsigned int curr_spd = NULL, curr_dir = CW, destination = NULL;//current speed and current direction

	pc = 0;//pc will start from beginning
	while (pc < prog->length && pc < PROGRAM_MAXSIZE && prog->action[pc] != END)
	{
		switch (prog->action[pc])
		{
			case GO: /* Go dir @ speed to HS */
			{
				pc++;
				curr_dir = prog->action[pc++];
				curr_spd = prog->action[pc++];
				destination = prog->action[pc];//destination hall sensor

				//create message
				mag_dir speed = { curr_spd, IGNORED, curr_dir };
				Message msg = { LOCOMOTIVE_CONTROLER, locomotive};
				//Message is the same name with in Message.h but in process.c we didn't include Message.h
				memcpy(&msg.arg2,&speed, sizeof(msg.arg2));

				SentMessage(TWO_ARGS, &msg, locomotive);

				int arrive_destination = FALSE;
				while (!arrive_destination)
				{
					received_msg* hs_msg = NULL;
					int sz = sizeof(hs_msg);
					int sender;
					Receive(locomotive, &sender, &hs_msg, &sz); // receive message

					if (sender == RECEIVED_PORCESSOR_MBX) // if reached a hole sensor
					{
						if (hs_msg->msg->code == HOLESENSOR_TRAINSET) // if is hole sensor message
						{
							if (hs_msg->msg->arg1 == destination) // if arrive destination
							{
								arrive_destination = TRUE;
							}
						}
					}
					free(hs_msg); // release memary
				}

				break;
			}
			case SWITCH: /* Throw specific switch */
			{
				pc++;
				unsigned char switch_id = prog->action[pc++]; // get switch id
				unsigned char switch_dir = prog->action[pc]; // get switch dir

				Message msg = { SWITHC_CONTROLER, switch_id, switch_dir }; // create message
				SentMessage(TWO_ARGS, &msg, locomotive); // send message

				break;
			}
			case HALT: /* Halt the train */
			{
				//create message
				mag_dir speed = { 0, IGNORED, IGNORED };
				curr_spd = 0;
				Message msg = { LOCOMOTIVE_CONTROLER, locomotive };
				memcpy(&msg.arg2, &speed, sizeof(msg.arg2));
				SentMessage(TWO_ARGS, &msg, locomotive); // send message

				break;
			}
			case PAUSE: /* Pause the train for # second */
			{
				int sec = prog->action[pc];

				//create message
				mag_dir speed = { 0, IGNORED, IGNORED };
				Message msg = { LOCOMOTIVE_CONTROLER, locomotive };
				memcpy(&msg.arg2, &speed, sizeof(msg.arg2));

				int sender = ERROR;
				received_msg* rec_msg;
				int sz_msg = sizeof(rec_msg);
				int time_out = FALSE;
				int time_count = 0;

				// wait to second
				while (!time_out)
				{
					Receive(locomotive, &sender, &rec_msg, &sz_msg);
					if (sender == SYSTICK_MBX) // if received message from trainset
						time_count++;

					if (time_count*TO_SECOND == sec)
						time_out = TRUE;

					free(rec_msg); // release memory
				}

				mag_dir speed_resume = { curr_spd, IGNORED, curr_dir };
				memcpy(&msg.arg2, &speed_resume, sizeof(msg.arg2));
				SentMessage(TWO_ARGS, &msg, locomotive);

				break;
			}
		}
		pc++;
	}
	//printf("End of program\n");

	return TRUE;
}

void Train_1_Application_Process()
{
	int mbx = Bind(LOCOMOTIVE_1); // bind mailbox

	// create route
	program route = { 13,
	    GO, CW, 7, 21,
	    SWITCH, 5, DIVERGED, /* Switch '0' to diverged */
	    HALT,
	    GO, CCW, 5, 3, /* Go CW @ speed 5 to HS#3 */

	    END };

    //char msg[9] = {0x02,0x00,0x10,0x03,0xc0,0xff,0x85,0xb8,0x03}; // working!!!
    //OutputData(msg, 9, 1);

	Run_machine(&route, ALL_LOCOMOTIVE);
}

/* The process to manage the received message from trainset*/
void Received_Message_Processor()
{
	int mbx = Bind(RECEIVED_PORCESSOR_MBX); // bind mailbox
	frame* received_frame = NULL;
	int size = sizeof(received_frame);
	int sender = NULL;
	packet pkt;
	while (TRUE)
	{
		Receive(RECEIVED_PORCESSOR_MBX, &sender, &received_frame, &size); // check if message arrived

		// Varify checksum
		if (DecodeFrameToPacket(received_frame, &pkt)) // if received packet is valid (checksum correct)
		{
			int type = ((int)pkt.pkt[CONTROL]) >> TYPE_SHIFT;
			int nr = ((int)pkt.pkt[CONTROL]) & NR_AND;

			if (type == DATA) // if received data
			{
				int ns = ((int)pkt.pkt[CONTROL]) >> NS_SHIFT;
				if (ns == Nr) // if received correct message
				{
					Nr++; // update Nr
					Nr &= NR_AND; // limit Nr in 3 bit, value from 0 to 7

					// send acknowledgement
					frame* ack_frame = NULL;
					int sz_ack = sizeof(ack_frame);
					GetAckFrame(ack_frame, ACK);
					Send(UART1_OUTPUT_MBX, RECEIVED_PORCESSOR_MBX, &ack_frame, &sz_ack); // send ack

					// pass message to train process
					received_msg* msg = (received_msg*)malloc(sizeof(received_msg)); // get message
					msg->type = DATA; // type is data
					msg->msg = (Message*)&(pkt.pkt[MESSAGE]); // take message address
					int sz = sizeof(msg);
					Send(LOCOMOTIVE_1, RECEIVED_PORCESSOR_MBX, &msg, &sz);

				}
				else if (ns == Nr+1) // if miss last message
				{
					// return NACK
					frame* nack_frame = NULL;
					int sz_nack = sizeof(nack_frame);
					GetAckFrame(nack_frame, NACK);
					Send(UART1_OUTPUT_MBX, RECEIVED_PORCESSOR_MBX, &nack_frame, &sz_nack); // send nack
				}
			}
			else if(type == ACK) // if received ack
			{
				if (nr == Ns) // if received correct ack
				{
					received_msg* msg = (received_msg*)malloc(sizeof(received_msg));
					msg->type = ACK;
					msg->msg = NULL;
					int sz = sizeof(msg);

					// notice train process
					Send(LOCOMOTIVE_1, RECEIVED_PORCESSOR_MBX, &msg, &sz);
				}
			}
			else if(type == NACK) // if received nack
			{
				// resend last frame and current frame
				SendFrame(&privious_frame, LOCOMOTIVE_1);
				SendFrame(&current_frame, LOCOMOTIVE_1);
			}
		}

		free(received_frame); // release meemory
	}
}

// Initialize all processes and force switch to thread mode
void Initialize_Process()
{
	reg_process(process_IDLE, PID_IDLE, PRIORITY_IDLE); // register idle process
	reg_process(Train_1_Application_Process, LOCOMOTIVE_1, PRIORITY_3); // register process 1
    reg_process(Received_Message_Processor, 2, PRIORITY_3); // register Received_Message_Processor process
    reg_process(process_UART0_OUTPUT, PID_UART0, PRIORITY_UART); // fegister uart output process
    reg_process(process_UART1_OUTPUT, PID_UART1, PRIORITY_UART); // fegister uart output process
}
