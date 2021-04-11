#include <iostream>
#include <thread>
#include <mutex>
#include <chrono>
#include <stdio.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netdb.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <chrono>
#include <stdlib.h>

#define currentTime chrono::high_resolution_clock::now
#define timeStamp chrono::high_resolution_clock::time_point
#define elapsedTime(end, start) chrono::duration_cast<chrono::milliseconds>(end - start).count()

#define MAX_DATA_SIZE 1024
#define MAX_FRAME_SIZE 1034
#define ACK_SIZE 6

typedef unsigned char byte;

int createPacketFrame(int sequenceNumber, char *frame, char *data, int dataLength, bool end);

bool readPacketFrame(int *sequenceNumber, char *data, int *dataLength, bool *end, char *frame);

void createPacketACK(int sequenceNumber, char *ack, bool errorAck);

bool readPacketACK(int *sequenceNumber, bool *errorAck, char *ack);

char checksum(char *frame, int count);

using namespace std;

int socketBucket;
struct sockaddr_in serverAddress, clientAddress;

int windowSize;
bool *windowMaskACK;
timeStamp *windowTransferTime;
int lar, lfs;

timeStamp TMIN = currentTime();
mutex windowMutex;

void listen_ack() {
    char ack[ACK_SIZE];
    int lengthACK;
    int sequenceNumberACK;
    bool errorACK;
    bool nak;

    while (true) {
        socklen_t serverAddressLength;
        lengthACK = recvfrom(socketBucket, (char *) ack, ACK_SIZE, MSG_WAITALL,
                             (struct sockaddr *) &serverAddress, &serverAddressLength);

        errorACK = readPacketACK(&sequenceNumberACK, &nak, ack);
        windowMutex.lock();

        if(errorACK){
            cout << "Packet failed to send" << endl;
        }

        if (!errorACK && sequenceNumberACK > lar && sequenceNumberACK <= lfs) {
            cout << "Ack: " << sequenceNumberACK << " received" << endl;
            if (!nak)
                windowMaskACK[sequenceNumberACK - (lar + 1)] = true;
            else
                windowTransferTime[sequenceNumberACK - (lar + 1)] = TMIN;
        }

        windowMutex.unlock();
    }
}

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

int main(int argc, char *argv[]) {
    double totalBandwidth;
    unsigned long long nbytes;
    int TIMEOUT;
    char destinationIP[99];
    int destinationPort;
    int bufferSizeMaximum;
    struct hostent *destinationHost;
    char fileName[99];
    int sockfd;
    FILE *fp2;
    struct sockaddr_in server_addr;
    FILE *fp;
    long packets_sent = 0;
    long retransmitted = 0;
    int s_err = 0;
    int drops[100];
    char error[5] = "";
    char protocol[4] = "";

    printf("Type of Protocol: SNW, GBN, or SR:\n");
    scanf("%s", protocol);

    string Protocol(protocol);

    printf("Size of packet:\n");
    scanf("%d", &bufferSizeMaximum);
    bufferSizeMaximum = bufferSizeMaximum * MAX_DATA_SIZE;

    printf("Timeout interval length\n");
    scanf("%d", &TIMEOUT);

    if (Protocol.compare("SNW") == 0) {
        windowSize = 1;
    } else {
        printf("Size of sliding window:\n");
        scanf("%d", &windowSize);
    }

    printf("Situational Errors (no(0) or yes(1)):\n");
    scanf("%d", &s_err);

    printf("Destination IP Address:\n");
    scanf("%s", destinationIP);

    printf("File to Send:\n");
    scanf("%s", fileName);

    printf("Destination Port:\n");
    scanf("%d", &destinationPort);


    /* Get hostnet from server hostname or IP address */
    destinationHost = gethostbyname(destinationIP);

    memset(&serverAddress, 0, sizeof(serverAddress));
    memset(&clientAddress, 0, sizeof(clientAddress));

    /* Fill server address data structure */
    serverAddress.sin_family = AF_INET;
    bcopy(destinationHost->h_addr, (char *) &serverAddress.sin_addr, destinationHost->h_length);
    serverAddress.sin_port = htons(destinationPort);

    /* Fill client address data structure */
    clientAddress.sin_family = AF_INET;
    clientAddress.sin_addr.s_addr = INADDR_ANY;
    clientAddress.sin_port = htons(0);

    /* Create socket file descriptor */
    socketBucket = socket(AF_INET, SOCK_DGRAM, 0);
    if (socketBucket < 0)
        return 1;

    /* Bind socket to client address */
    if (::bind(socketBucket, (const struct sockaddr *) &clientAddress, sizeof(clientAddress)) < 0)
        return 1;

    if (access(fileName, F_OK) == -1)
        return 1;

    /* Open file to send */
    FILE *file = fopen(fileName, "rb");
    char buffer[bufferSizeMaximum];
    int bufferSize;

    /* Start thread to listen for ack */
    thread ackListener(listen_ack);

    char frame[MAX_FRAME_SIZE];
    char data[MAX_DATA_SIZE];
    int frameLength;
    int dataLength;

    /* Send file */
    bool readStatus = false;
    int bufferNumber = 0;

    auto start = chrono::high_resolution_clock::now();

    while (!readStatus) {
        /* Read part of file to buffer */
        bufferSize = fread(buffer, 1, bufferSizeMaximum, file);
        if (bufferSize == bufferSizeMaximum) {
            char temp[1];
            int bufferNumberbufferSizeNext = fread(temp, 1, 1, file);
            if (bufferNumberbufferSizeNext == 0)
                readStatus = true;

            int error = fseek(file, -1, SEEK_CUR);
        } else if (bufferSize < bufferSizeMaximum) {
            readStatus = true;
        }

        windowMutex.lock();

        /* Initialize sliding window variables */
        long sequenceCount = bufferSize / MAX_DATA_SIZE + ((bufferSize % MAX_DATA_SIZE == 0) ? 0 : 1);
        int sequenceNumber;

        windowTransferTime = new timeStamp[windowSize];
        windowMaskACK = new bool[windowSize];
        bool windowMaskSending[windowSize];

        for (int i = 0; i < windowSize; i++) {
            windowMaskACK[i] = false;
            windowMaskSending[i] = false;
        }
        lar = -1;
        lfs = lar + windowSize;

        windowMutex.unlock();

        /* Send current buffer with sliding window */
        bool sendStatus = false;
        while (!sendStatus) {
            windowMutex.lock();

            /* Check window ack mask, shift window if possible */
            if (windowMaskACK[0]) {
                int shift = 1;
                for (int i = 1; i < windowSize; i++) {
                    if (!windowMaskACK[i])
                        break;

                    shift += 1;
                }

                for (int i = 0; i < windowSize - shift; i++) {
                    windowMaskSending[i] = windowMaskSending[i + shift];
                    windowMaskACK[i] = windowMaskACK[i + shift];
                    windowTransferTime[i] = windowTransferTime[i + shift];
                }

                for (int i = windowSize - shift; i < windowSize; i++) {
                    windowMaskSending[i] = false;
                    windowMaskACK[i] = false;
                }

                //largest accepted frame
                //last frame sent
                lar += shift;
                lfs = lar + windowSize;
            }

            windowMutex.unlock();

            for (int i = 0; i < windowSize; i++) {
                sequenceNumber = lar + i + 1;

                if (sequenceNumber < sequenceCount) {
                    windowMutex.lock();

                    if (!windowMaskSending[i]) {
                        cout << "Packet " << sequenceNumber << " sent" << endl << flush;
                        cout << "seqCount " << sequenceCount << endl;

                        packets_sent++;
                        print_window(windowSize, lar);

                        int shiftingBuffer = sequenceNumber * MAX_DATA_SIZE;
                        dataLength = (bufferSize - shiftingBuffer < MAX_DATA_SIZE) ? (bufferSize - shiftingBuffer)
                                                                                   : MAX_DATA_SIZE;
                        memcpy(data, buffer + shiftingBuffer, dataLength);

                        bool eot = (sequenceNumber == sequenceCount - 1) && (readStatus);
                        frameLength = createPacketFrame(sequenceNumber, frame, data, dataLength, eot);

                        //nudge receiver to wake them up
                        if (sequenceNumber == 0) {
                            sendto(socketBucket, frame, frameLength, 0, (const struct sockaddr *) &serverAddress,
                                   sizeof(serverAddress));
                        }

                        //XoR bit on first bit of CRC in packet to create user defined error
                        if (s_err == 1) {
                            frame[dataLength + 9] = (frame[dataLength +9] ^ 1);
                            frameLength = createPacketFrame(sequenceNumber, frame, data, dataLength, eot);

                            if ((sequenceNumber % 50) == 0) {
                                sendto(socketBucket, frame, frameLength, 0, (const struct sockaddr *) &serverAddress,
                                       sizeof(serverAddress));
                                retransmitted++;
                            }
                        }

                        sendto(socketBucket, frame, frameLength, 0, (const struct sockaddr *) &serverAddress,
                               sizeof(serverAddress));

                        windowMaskSending[i] = true;
                        windowTransferTime[i] = currentTime();
                    }
                    windowMutex.unlock();
                }
            }

            //###selective repeat
            /* Send frames that has not been sent or has timed out */
            if (Protocol.compare("SR") == 0 || Protocol.compare("SNW") == 0) {

                for (int i = 0; i < windowSize; i++) {
                    sequenceNumber = lar + i + 1;

                    if (sequenceNumber < sequenceCount) {
                        windowMutex.lock();

                        // if you have sent it & timed out
                        if (!windowMaskACK[i] && (elapsedTime(currentTime(), windowTransferTime[i]) > TIMEOUT)) {

                            retransmitted++;
                            cout << "Packet: " << sequenceNumber << " *****Timed Out *****" << endl << flush;
                            cout << "Packet: " << sequenceNumber << " Re-transmitted." << endl << flush;
                            print_window(windowSize, lar);

                            int shiftingBuffer = sequenceNumber * MAX_DATA_SIZE;
                            dataLength = (bufferSize - shiftingBuffer < MAX_DATA_SIZE) ? (bufferSize - shiftingBuffer)
                                                                                       : MAX_DATA_SIZE;
                            memcpy(data, buffer + shiftingBuffer, dataLength);

                            bool eot = (sequenceNumber == sequenceCount - 1) && (readStatus);
                            frameLength = createPacketFrame(sequenceNumber, frame, data, dataLength, eot);

                            sendto(socketBucket, frame, frameLength, 0, (const struct sockaddr *) &serverAddress,
                                   sizeof(serverAddress));
                            windowMaskSending[i] = true;
                            windowTransferTime[i] = currentTime();

                            //determines if the packet will be "lost" or ack "lost"
                            if (s_err == 1) {
                                if ((sequenceNumber % 2) == 0) {
                                    sendto(socketBucket, frame, frameLength, 1,
                                           (const struct sockaddr *) &serverAddress,
                                           sizeof(serverAddress));
                                }
                            }
                        }

                        windowMutex.unlock();
                    }
                }
            }
            //###selective repeat

            if (Protocol.compare("GBN") == 0) {
                sequenceNumber = lar + 1;

                for (int i = 0; i < windowSize; i++) {
                    sequenceNumber = lar + i + 1;

                    if (sequenceNumber < sequenceCount) {
                        windowMutex.lock();

                        if (!windowMaskACK[0] && (elapsedTime(currentTime(), windowTransferTime[0]) > TIMEOUT)) {
                            cout << "Packet: " << sequenceNumber << "*****Timed Out *****" << endl;

                            for (int i = 0; i < windowSize; i++) {
                                sequenceNumber = lar + i + 1;
                                int shiftingBuffer = sequenceNumber * MAX_DATA_SIZE;
                                dataLength = (bufferSize - shiftingBuffer < MAX_DATA_SIZE) ? (bufferSize -
                                                                                              shiftingBuffer)
                                                                                           : MAX_DATA_SIZE;
                                memcpy(data, buffer + shiftingBuffer, dataLength);

                                bool eot = (sequenceNumber == sequenceCount - 1) && (readStatus);
                                frameLength = createPacketFrame(sequenceNumber, frame, data, dataLength, eot);

                                retransmitted++;
                                cout << "Packet: " << sequenceNumber << " Re-transmitted." << endl;
                                cout << "seqNum: " << sequenceNumber << endl;
                                print_window(windowSize, lar);

                                sendto(socketBucket, frame, frameLength, 0, (const struct sockaddr *) &serverAddress,
                                       sizeof(serverAddress));
                                windowMaskSending[i] = true;
                                windowTransferTime[i] = currentTime();

                                //determines if the packet will be "lost" or ack "lost"
                                if (s_err == 1) {
                                    if (sequenceNumber % 2 == 0) {
                                        sendto(socketBucket, frame, frameLength, 1,
                                               (const struct sockaddr *) &serverAddress,
                                               sizeof(serverAddress));
                                    }
                                }
                            }
                        }
                        windowMutex.unlock();
                    }
                }
            }

            /* Move to next buffer if all frames in current buffer has been acked */
            if (lar >= sequenceCount - 1) sendStatus = true;

        }
        nbytes = (unsigned long long) bufferNumber * (unsigned long long) bufferSizeMaximum
                 + (unsigned long long) bufferSize;

        bufferNumber += 1;

        if (readStatus) {
            break;
        }
    }

    auto stop = chrono::high_resolution_clock::now();

    auto duration = chrono::duration_cast<chrono::milliseconds>(stop - start).count();

    totalBandwidth = (nbytes / duration) * .00781248;

    cout << "Session successfully terminated \n" << endl;
    cout << "Number of original packets sent: " << packets_sent << endl;
    cout << "Number of retransmitted packets: " << retransmitted << endl;
    cout << "Total elapsed time: " << (duration / 1000) << " seconds" << "\n" << endl;
    cout << "Total throughput (Mbps): " << totalBandwidth << endl;
    cout << "Effective throughput: " << (packets_sent - retransmitted) << endl;

    fclose(file);
    delete[] windowMaskACK;
    delete[] windowTransferTime;
    ackListener.detach();

    cout << "\nDone" << endl;
    return 0;
}

