/*
 * DataLinkLayer.h
 *
 *  Created on: Dec 5, 2019
 *      Author: Ray
 */

#ifndef DATALINKLAYER_H_
#define DATALINKLAYER_H_

#define PACKET_MAXSIZE		5	// Max size of packet
#define CONTROL	0	//position of control
#define LENGTH	1	//position of length
#define MESSAGE	2	//position of message
#define TYPE_SHIFT	6	//shift right 6 bit to get type
#define NS_SHIFT	3	//shift right 3 bit to get Ns
#define NR_AND		0b00000111 //and this to  get Nr

typedef enum { DATA, ACK, NACK }PktType;

typedef struct 
{
	unsigned char nr : 3;
	unsigned char ns : 3;
	PktType type : 2;
}control;

// stucture of packet
typedef struct
{
	int size; // size of packet
	char pkt[PACKET_MAXSIZE]; // packet data
}packet;

void EncodeMsgToPacket(char* msg, int length, packet* pkt);

void GetAckFrame(void* fm, PktType type);

#endif /* DATALINKLAYER_H_ */
