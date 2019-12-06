/*
 * DataLinkLayer.c
 *
 *  Created on: Dec 5, 2019
 *      Author: Ray
 */

#include "DataLinkLayer.h"
#include "ApplicationLayer.h"

volatile char Ns = 0;
volatile char Nr = 0;

void EncodeMsgToPacket(Message* msg, int length)
{

}