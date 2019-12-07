/*
 * PhysicalLayer.c
 *
 *  Created on: Dec 5, 2019
 *      Author: Ray
 */

#include "PhysicalLayer.h"
#include "DataLinkLayer.h"

#define TRUE    1
#define FALSE   0
#define FRAME_SIZE_OFFSET	3	//frame has three bytes more than packet
#define CHECKSUM_POS_END	2	//checksum at length-2 postion
#define PACKET_POS	1 // position of packet in frame

void EncodePacketToFrame(void* pkt, frame* frm)
{
	packet* Pkt = (packet*)pkt;
	unsigned char checksum = 0;

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

		frm->frm[i] = Pkt->pkt[i - DLE_count - 1];
		checksum += Pkt->pkt[i - 1]; // update check sum
	}

	// load checksum
	frm->frm[i++] = checksum;

	// load ETX
	frm->frm[i] = ETX;

	// update length
	frm->length = i;
}

int DecodeFrameToPacket(frame* frm, void* pkt)
{
	packet* Pkt = (packet*)pkt;

	unsigned char checksum = 0;
	int i;
	// load packet
	for (i = 0; i < frm->length-FRAME_SIZE_OFFSET; i++)
	{
		Pkt->pkt[i] = frm->frm[i + PACKET_POS];
		checksum += frm->frm[i + PACKET_POS];
	}

	// return TRUE if checksum correct
	return checksum == frm->frm[frm->length - CHECKSUM_POS_END] ? TRUE : FALSE;
}
