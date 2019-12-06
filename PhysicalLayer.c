/*
 * PhysicalLayer.c
 *
 *  Created on: Dec 5, 2019
 *      Author: Ray
 */

#include "PhysicalLayer.h"
#include "DataLinkLayer.h"

#define FRAME_SIZE_OFFSET	3	//frame has three bytes more than packet

void EncodePacketToFrame(void* pkt, frame* frm)
{
	packet* Pkt = (packet*)pkt;
	unsigned char checksum;
	frm->length = Pkt->size + FRAME_SIZE_OFFSET;

	int i = 0;
	// load STX
	frm->frm[i++] = STX;

	// load packet
	for (i; i < Pkt->size + 1; i++)
	{
		frm->frm[i] = Pkt->pkt[i - 1];
		checksum += Pkt->pkt[i - 1]; // update check sum
	}

	// load checksum
	frm->frm[i++] = checksum;

	// load ETX
	frm->frm[i] = ETX;
}