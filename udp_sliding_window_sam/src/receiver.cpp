#include <iostream>
#include <thread>

#include <stdio.h>
#include <sys/socket.h>
#include <netdb.h>

#include "frame.h"
#include "utils.h"

/* ACK standby time */
#define ACK_STBY 3000

using namespace std;

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
            cout << "Packet " << sequenceNumber << " received" << endl;

            createPacketACK(sequenceNumber, ack, frameError);
            //TODO: put in some sort of control to fake errors. By not calling the sendto the sender will not 
            // get the ACK. Emulationing noise.
            sendto(socketBucket, ack, ACK_SIZE, 0, (const struct sockaddr *) &clientAddress, clientAddressLength);

            if (sequenceNumber <= laf) {
                if (!frameError) {
                    cout << "Checksum OK" << endl;
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
                }
            }

            else{
                cout << "Checksum failed" << endl;
            }

            /* if current buffer done, move to next buffer */
            if (lfr >= numberBuffer - 1)
                break;
        }

        unsigned long long nbytes = (unsigned long long) numberBuffer * (unsigned long long) bufferSizeMaximum
                                    + (unsigned long long) bufferSize;
        cout << "\r" << "[RECEIVED " << nbytes << " BYTES]" << flush;
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

