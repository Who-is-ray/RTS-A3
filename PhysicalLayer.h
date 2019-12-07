/*
 * PhysicalLayer.h
 *
 *  Created on: Dec 5, 2019
 *      Author: Ray
 */

#ifndef PHYSICALLAYER_H_
#define PHYSICALLAYER_H_

#define FRAME_MAXSIZE 13

#define STX 0x02
#define ETX 0x03
#define DLE 0x10

// structure of frame
typedef struct
{
	int length; // length of frame
	char frm[FRAME_MAXSIZE]; // frame data
}frame;

void EncodePacketToFrame(void* pkt, frame* frm);

#endif /* PHYSICALLAYER_H_ */
