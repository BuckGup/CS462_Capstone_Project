#include <iostream>
#include <thread>
#include <stdio.h>
#include <sys/socket.h>
#include <netdb.h>
#include <chrono>
#include <stdlib.h>

#define currentTime chrono::high_resolution_clock::now
#define timeStamp chrono::high_resolution_clock::time_point
#define elapsedTime(end, start) chrono::duration_cast<chrono::milliseconds>(end - start).count()

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


/* ACK standby time */
#define ACK_STBY 3000

using namespace std;

void print_window(int windowSize, int lar) {
    int j = 0;
    cout << "Current window = [";
    for (int i = 0; i < windowSize - 1; i++) {
        // sequence number
        cout << lar + i + 1 << ",";
        j = i;
    }
    cout << lar + j + 2 << "]" << endl;
}

int createPacketFrame(int sequenceNumber, char *frame, char *data, int dataLength, bool end) {
    if (end)
        frame[0] = 0x0;
    else
        frame[0] = 0x1;

    uint32_t netSequenceNumber = htonl(sequenceNumber); // converts unsigned integer to network byte order.
    uint32_t netDataLength = htonl(dataLength);
    memcpy(frame + 1, &netSequenceNumber, 4);
    memcpy(frame + 5, &netDataLength, 4);
    memcpy(frame + 9, data, dataLength);
    frame[dataLength + 9] = checksum(frame, dataLength + (int) 9);

    return dataLength + (int) 10;
}

bool readPacketFrame(int *sequenceNumber, char *data, int *dataLength, bool *end, char *frame) {
    if (frame[0] == 0x0)
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

void createPacketACK(int sequenceNumber, char *ack, bool errorAck) {
    if (errorAck)
        ack[0] = 0x0;
    else
        ack[0] = 0x1;

    uint32_t netSequenceNumber = htonl(sequenceNumber);
    memcpy(ack + 1, &netSequenceNumber, 4);
    ack[5] = checksum(ack, ACK_SIZE - (int) 1);
}

bool readPacketACK(int *sequenceNumber, bool *errorAck, char *ack) {
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

int socketBucket;
struct sockaddr_in serverAddress, clientAddress;

void send_ack() {
    char frame[MAX_FRAME_SIZE];
    char data[MAX_DATA_SIZE];
    char ack[ACK_SIZE];
    int frameLength, dataLength;
    socklen_t clientAddressLength;

    int sequenceNumber;
    bool frameError;
    bool end;

    /* listen for frames and send ack */
    while (true) {
        frameLength = recvfrom(socketBucket, (char *) frame, MAX_FRAME_SIZE, MSG_WAITALL,
                               (struct sockaddr *) &clientAddress, &clientAddressLength);

        frameError = readPacketFrame(&sequenceNumber, data, &dataLength, &end, frame);

        createPacketACK(sequenceNumber, ack, frameError);

        cout << "Ack: " << sequenceNumber << " sent" << endl;
        sendto(socketBucket, ack, ACK_SIZE, 0, (const struct sockaddr *) &clientAddress, clientAddressLength);
    }
}

int main(int argc, char *argv[]) {
    char *fileName;
    unsigned int windowSize;
    // unsigned int bufferSize;
    int port;
    int bufferSizeMaximum;
    // int windowSize;
    int retransmitted;
    int packets_received = 0;


    //accepts falgs from user
    if (argc == 5) {
        fileName = argv[1];
        windowSize = (int) atoi(argv[2]);
        bufferSizeMaximum = MAX_DATA_SIZE * (int) atoi(argv[3]);
        port = atoi(argv[4]);
    } else {
        return 1;
    }

    memset(&serverAddress, 0, sizeof(serverAddress));
    memset(&clientAddress, 0, sizeof(clientAddress));

    /* define server address */
    serverAddress.sin_family = AF_INET;
    serverAddress.sin_addr.s_addr = INADDR_ANY;
    serverAddress.sin_port = htons(port);

    /* socket */
    socketBucket = socket(AF_INET, SOCK_DGRAM, 0);
    if (socketBucket < 0)
        return 1;

    /* bind socket to server address */
    if (::bind(socketBucket, (const struct sockaddr *) &serverAddress, sizeof(serverAddress)) < 0)
        return 1;

    FILE *file = fopen(fileName, "wb");
    char buffer[bufferSizeMaximum];
    int bufferSize;

    /* sliding window */
    char frame[MAX_FRAME_SIZE];
    char data[MAX_DATA_SIZE];
    char ack[ACK_SIZE];
    int frameLength;
    int dataLength;
    int lfr, laf;
    int sequenceNumber;
    bool end;
    bool frameError;

    /* receive frames until end */
    bool receivedStatus = false;
    int numberBuffer = 0;
    while (!receivedStatus) {
        bufferSize = bufferSizeMaximum;
        memset(buffer, 0, bufferSize);

        int numberBuffer = (int) bufferSizeMaximum / MAX_DATA_SIZE;
        bool windowMaskReceive[windowSize];
        for (int i = 0; i < windowSize; i++)
            windowMaskReceive[i] = false;

        lfr = -1;
        laf = lfr + windowSize;
        //largest accepted frame
        //last frame received

        /* receive current buffer with sliding window */
        while (true) {
            socklen_t clientAddressLength;
            frameLength = recvfrom(socketBucket, (char *) frame, MAX_FRAME_SIZE, MSG_WAITALL,
                                   (struct sockaddr *) &clientAddress, &clientAddressLength);
            frameError = readPacketFrame(&sequenceNumber, data, &dataLength, &end, frame);

            createPacketACK(sequenceNumber, ack, frameError);
            sendto(socketBucket, ack, ACK_SIZE, 0, (const struct sockaddr *) &clientAddress, clientAddressLength);

            if (sequenceNumber <= laf) {
                if (!frameError) {
                    cout << "Packet " << sequenceNumber << " received" << endl;
                    packets_received++;

                    cout << "Checksum OK" << endl;
                    cout << "Ack " << sequenceNumber << " sent" << endl;

                    int shiftingBufferPosition = sequenceNumber * MAX_DATA_SIZE;

                    if (sequenceNumber == lfr + 1) {
                        memcpy(buffer + shiftingBufferPosition, data, dataLength);

                        int shift = 1;
                        for (int i = 1; i < windowSize; i++) {
                            if (!windowMaskReceive[i])
                                break;

                            shift += 1;
                        }

                        for (int i = 0; i < windowSize - shift; i++) {
                            windowMaskReceive[i] = windowMaskReceive[i + shift];
                        }

                        for (int i = windowSize - shift; i < windowSize; i++) {
                            windowMaskReceive[i] = false;
                        }

                        lfr += shift;
                        laf = lfr + windowSize;
                    } else if (sequenceNumber > lfr + 1) {
                        if (!windowMaskReceive[sequenceNumber - (lfr + 1)]) {
                            memcpy(buffer + shiftingBufferPosition, data, dataLength);
                            windowMaskReceive[sequenceNumber - (lfr + 1)] = true;
                        }
                    }

                    if (end) {
                        bufferSize = shiftingBufferPosition + dataLength;
                        numberBuffer = sequenceNumber + 1;
                        receivedStatus = true;
                    }
                } else {
                    cout << "Checksum failed" << endl;
                    retransmitted++;
                }
            }

            /* if current buffer done, move to next buffer */
            if (lfr >= numberBuffer - 1)
                break;
        }

        cout << "Last packet seq# received: " << sequenceNumber << endl;
        cout << "Number of original packets received: " << packets_received << endl;
        cout << "Number of packets retransmitted: " << retransmitted << endl;

        fwrite(buffer, 1, bufferSize, file);
        numberBuffer += 1;
    }

    fclose(file);

    /* keep sending requested ack to sender for ACK_STBY secs in new thread */
    thread stdby_thread(send_ack);
    timeStamp start_time = currentTime();
    while (elapsedTime(currentTime(), start_time) < ACK_STBY) {
        cout << "\r" << flush;
        this_thread::sleep_for(chrono::milliseconds(100));
        cout << "\r" << flush;
        this_thread::sleep_for(chrono::milliseconds(100));
        cout << "\r" << flush;
        this_thread::sleep_for(chrono::milliseconds(100));
        cout << "\r" << flush;
        this_thread::sleep_for(chrono::milliseconds(100));
    }
    stdby_thread.detach();

    cout << "\nDone" << endl;
    return 0;
}

