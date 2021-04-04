#include "frame.h"
#include "utils.h"
#include <iostream>

using namespace std;

int createPacketFrame(int sequenceNumber, char *frame, char *data, int dataLength, bool end)
{
    if(end)
        frame[0] = 0x0;
    else
        frame[0] = 0x1;

    uint32_t netSequenceNumber = htonl(sequenceNumber); // converts unsigned integer to network byte order.
    uint32_t netDataLength= htonl(dataLength);
    memcpy(frame + 1, &netSequenceNumber, 4);
    memcpy(frame + 5, &netDataLength, 4);
    memcpy(frame + 9, data, dataLength);
    frame[dataLength + 9] = checksum(frame, dataLength + (int) 9);

    return dataLength + (int)10;
}

bool readPacketFrame(int *sequenceNumber, char *data, int *dataLength, bool *end, char *frame)
{
    if(frame[0] == 0x0)
        *end = true;
    else
        *end = false;

    uint32_t netSequenceNumber;
    memcpy(&netSequenceNumber, frame + 1, 4);
    *sequenceNumber = ntohl(netSequenceNumber);

    uint32_t netDataLength;
    memcpy(&netDataLength, frame + 5, 4);
    *dataLength = ntohl(netDataLength);

    memcpy(data, frame + 9, *dataLength);

    return frame[*dataLength + 9] != checksum(frame, *dataLength + (int) 9);
}

void createPacketACK(int sequenceNumber, char *ack, bool errorAck)
{
	if(errorAck)
		ack[0]=0x0;	
	else
		ack[0]=0x1;

	uint32_t netSequenceNumber = htonl(sequenceNumber);
	memcpy(ack + 1, &netSequenceNumber, 4);
	ack[5] = checksum(ack, ACK_SIZE - (int) 1);
}

bool readPacketACK(int *sequenceNumber, bool *errorAck, char *ack)
{
    *errorAck = ack[0] == 0x0 ? true : false;
    uint32_t netSequenceNumber;
    memcpy(&netSequenceNumber, ack + 1, 4);
    *sequenceNumber = ntohl(netSequenceNumber);

    return ack[5] != checksum(ack, ACK_SIZE - (int) 1);
}

char checksum(char *frame, int count) {
    u_long sum = 0;
    while (count--) {
        sum += *frame++;
        if (sum & 0xFFFF0000) {
            sum &= 0xFFFF;
            sum++; 
        }
    }
    return (sum & 0xFFFF);
}