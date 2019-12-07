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
	unsigned char checksum = 0;
	frm->length = Pkt->size + FRAME_SIZE_OFFSET;

	int i = 0;
	// load STX
	frm->frm[i++] = STX;

	// load packet
	int DLE_count = 0;
	for (; i < Pkt->size + 1; i++)
	{
	    if(Pkt->pkt[i - 1] == STX || Pkt->pkt[i - 1] == DLE ||Pkt->pkt[i - 1] == ETX)
	    {
	        DLE_count++;
	        frm->frm[i++] = DLE;
	    }

		frm->frm[i-DLE_count] = Pkt->pkt[i - 1];
		checksum += Pkt->pkt[i - 1]; // update check sum
	}

	// load checksum
	frm->frm[i++] = checksum;

	// load ETX
	frm->frm[i] = ETX;
}
