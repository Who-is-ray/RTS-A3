/*
 * DataLinkLayer.h
 *
 *  Created on: Dec 5, 2019
 *      Author: Ray
 */

#ifndef DATALINKLAYER_H_
#define DATALINKLAYER_H_

enum PktType { DATA, ACK, NACK };

struct control
{
	unsigned nr : 3;
	unsigned ns : 3;
	enum PktType type : 2;
};

void EncodeMsgToPacket(Message* msg, int length);

#endif /* DATALINKLAYER_H_ */
