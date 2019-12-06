/*
 * ApplicationLayer.c
 *
 *  Created on: Dec 5, 2019
 *      Author: Ray
 */

#include "ApplicationLayer.h"

int GenerateMessages(program* prog, int route, Message_QueueItem* head)
{
	/* Follows instructions in supplied program (the route) */
	unsigned int pc;
	unsigned int curr_spd = 0, curr_dir = CW;

	pc = 0;
	while (pc < prog->length && pc < MAXSIZE && prog->action[pc] != END)
	{
		printf("%d: ", pc);
		switch (prog->action[pc])
		{
		case GO: /* Go to HS# to dir and spd */
			printf("GO: ");
			pc++;
			curr_dir = prog->action[pc++];
			curr_spd = prog->action[pc++];
			printf("Direction: %s Speed: %d HS: %d\n",
				curr_dir == CW ? "CW" : "CCW", curr_spd, prog->action[pc]);
			break;
		case SWITCH: /* Throw specific switch */
			printf("SWITCH: ");
			pc++;
			printf("Switch: %d %s\n", prog->action[pc++],
				prog->action[pc] ? "STRAIGHT" : "DIVERGED");
			break;
		case HALT: /* Halt the train */
			printf("HALT\n");
			curr_spd = 0;
			printf("Speed to zero\n");
			break;
		case PAUSE: /* Pause the train for # second */
			printf("PAUSE\n");
			pc++;
			printf("Speed to zero\n");
			printf("Wait for %d seconds\n", prog->action[pc]);
			printf("Set speed to: %d\n", curr_spd);
			break;
		default:
			printf("Unknown inst at pc: %d", pc);
		}
		pc++;
	}
	printf("End of program\n");

	return TRUE;
}
