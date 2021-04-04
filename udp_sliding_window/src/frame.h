#ifndef FRAME_H
#define FRAME_H

#include <stdlib.h> 
#include <string.h>
#include <sys/types.h>
#include <netinet/in.h>

#define MAX_DATA_SIZE 1024
#define MAX_FRAME_SIZE 1034
#define ACK_SIZE 6

typedef unsigned char byte;

int createPacketFrame(int sequenceNumber, char *frame, char *data, int dataLength, bool end);
bool readPacketFrame(int *sequenceNumber, char *data, int *dataLength, bool *end, char *frame);

void createPacketACK(int sequenceNumber, char *ack, bool errorAck);
bool readPacketACK(int *sequenceNumber, bool *errorAck, char *ack);

char checksum(char *frame, int count);

#endif
