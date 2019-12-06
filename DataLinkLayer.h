/*
 * DataLinkLayer.h
 *
 *  Created on: Dec 5, 2019
 *      Author: Ray
 */

#ifndef DATALINKLAYER_H_
#define DATALINKLAYER_H_

#define PACKET_MAXSIZE		5	// Max size of packet

enum PktType { DATA, ACK, NACK };

typedef struct 
{
	unsigned char nr : 3;
	unsigned char ns : 3;
	enum PktType type : 2;
}control;

// stucture of packet
typedef struct
{
	int size; // size of packet
	char pkt[PACKET_MAXSIZE]; // packet data
}packet;

void EncodeMsgToPacket(char* msg, int length, packet* pkt);

#endif /* DATALINKLAYER_H_ */
