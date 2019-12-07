/*
 * DataLinkLayer.c
 *
 *  Created on: Dec 5, 2019
 *      Author: Ray
 */

#include "DataLinkLayer.h"
#include "ApplicationLayer.h"
#include "PhysicalLayer.h"

extern char Ns;
extern char Nr;

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

void GetAckFrame(void* fm, PktType type)
{
	frame* frm = (frame*)fm;
	// set control
	control ctl = { Nr,IGNORED,type };
	packet pkt;
	memcpy(&(pkt.pkt[CONTROL]), &ctl, sizeof(control));
	pkt.size = 1;

	EncodePacketToFrame(&pkt, frm);
}
