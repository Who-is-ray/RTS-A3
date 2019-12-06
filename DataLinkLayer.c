/*
 * DataLinkLayer.c
 *
 *  Created on: Dec 5, 2019
 *      Author: Ray
 */

#include "DataLinkLayer.h"
#include "ApplicationLayer.h"

extern char Ns;
extern char Nr;

#define CONTROL	0	//position of control
#define LENGTH	1	//position of length
#define MESSAGE	2	//position of message
#define PACKET_SIZE_OFFSET	2	// data packet has 2 more byte than message

void EncodeMsgToPacket(char* msg, int length, packet* pkt)
{
	pkt->size = length + PACKET_SIZE_OFFSET;

	// set control
	control ctl = { Nr,Ns,DATA };
	memcpy(&pkt->pkt[CONTROL], &ctl, sizeof(control));

	// set lenght
	pkt->pkt[LENGTH] = length;

	// set message
	int i;
	for ( i = 0; i < length; i++)
		pkt->pkt[i + MESSAGE] = msg[i];
}
