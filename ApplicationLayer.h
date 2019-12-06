/*
 * ApplicationLayer.h
 *
 *  Created on: Dec 5, 2019
 *      Author: Ray
 */

#ifndef APPLICATIONLAYER_H_
#define APPLICATIONLAYER_H_

#define MAXSIZE			32	/* Maximum program length */
#define LOCOMOTIVE_1	1	/* Locomotive # 1*/
#define TRUE    1
#define FALSE   0

 /* Actions */
#define GO   	0	/* Data: Direction (CW or CCW), Speed (1..7), HS# */
#define SWITCH	1	/* Data: SW# and STRAIGHT or DIVERGED */
#define END		2	/* No data, end of program */
#define HALT	3	/* Stop moving, continue with next action */
#define PAUSE	4	/* Data: # seconds to pause */

/* Direction data */
#define CW 		0
#define CCW		1

/* Switch data */
#define STRAIGHT	0
#define DIVERGED	1

typedef struct _Message_QueueItem Message_QueueItem;

// Structure of message
typedef struct
{
	unsigned char code; /* Message code (described below) */
	unsigned char arg1; /* First argument (optional) */
	unsigned char arg2; /* Second argument (optional) */
} Message;

// Structure of speend magnitude and direction
typedef struct 
{
	unsigned char magnitude : 4; /* 0 ¨C stop through 15 ¨C maximum */ 
	unsigned char ignored : 3;	/* Zero */ 
	unsigned char direction : 1; /* 1 for CCW and 0 for CW */ 
}mag_dir;

// Structure of locomotive's program
typedef struct 
{
	int length; /* # of actions in program */
	char action[MAXSIZE];
}program;

// Queue item of message in the message queue
struct _Message_QueueItem
{
	Message_QueueItem* Next;
	Message_QueueItem* Privious;
	Message* msg;
};

int GenerateMessages(program* prog, int route, Message_QueueItem* head);

#endif /* APPLICATIONLAYER_H_ */
