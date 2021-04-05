#include <iostream>
#include <thread>
#include <mutex>
#include <chrono>

#include <stdio.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netdb.h>

#include "frame.h"
#include "utils.h"

//#define TIMEOUT 10

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
                 for (int i = 0; i < windowSize -1; i++) {
                    // sequence number
                    cout << lar + i + 1 << ",";
                    j = i;
                 }
                 cout << lar + j + 2 << "]" << endl;
}

int main(int argc, char *argv[]) {
    double totalBandwidth;
    unsigned long long nbytes;
    int TIMEOUT;
    char destinationIP[99];
    //&desntinationIP = "10.34.40.33";
    //char destinationIP[99];
    int destinationPort;
    int bufferSizeMaximum;
    struct hostent *destinationHost;
    char fileName[99];
    int sockfd;
    char IP_ADDRESS[20];
    int PORT;
    int f;
    char MD5cmd[1000];
    FILE *fp2;
    struct sockaddr_in server_addr;
    FILE *fp;
    long packets_sent = 0;
    long retransmitted = 0;
    char s_err[5] = "";
    int drops[100];
    char error[5] = "";
    char protocol[4] = "";
    char time[5] = "";
    int timeout = 0;
    int window_size = 0;
    int seq_num = 0;
    int packet_no = 0;
    int tot_time = 0;
    int tot_through = 0;
    int eff_through = 0;

    printf("Type of Protocol: SNW, GBN, or SR:\n");
    scanf("%s", protocol);

    string Protocol (protocol);

    printf("Size of packet:\n");
    scanf("%d", &bufferSizeMaximum);
    bufferSizeMaximum = bufferSizeMaximum * MAX_DATA_SIZE;

    printf("Timeout interval length\n");
    scanf("%d", &TIMEOUT);

    if (Protocol.compare("SNW") == 0){
        windowSize = 1;
    } else{
           printf("Size of sliding window:\n");
           scanf("%d", &windowSize);
    }

    printf("Situational Errors (none(n) or user specified(u)):\n");
    scanf("%s", s_err);
    if (strcmp(s_err, "u") == 0) {
        int num = 0;
        int place = 0;
        printf("drop packets(d) or lose acks(l)\n");
        scanf("%s", error);
        printf("packet or ack numbers(-1 to exit):");
        while (scanf("%d", &num)) {
            if (num == -1) { break; }
            drops[place] = num;
            place = place + 1;
        }
    }

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
                                if (sequenceNumber == 0){
                                    sendto(socketBucket, frame, frameLength, 0, (const struct sockaddr *) &serverAddress,
                                   sizeof(serverAddress));
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
            if(Protocol.compare("SR") == 0 || Protocol.compare("SNW") == 0) {
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

                                  if (!windowMaskACK[0] && (elapsedTime(currentTime(), windowTransferTime[0]) > TIMEOUT)){
                                     cout << "Packet: " << sequenceNumber << "*****Timed Out *****" << endl;

                                       for (int i = 0; i < windowSize; i++) {
                                            sequenceNumber = lar + i + 1;
                                            int shiftingBuffer = sequenceNumber * MAX_DATA_SIZE;
                                            dataLength = (bufferSize - shiftingBuffer < MAX_DATA_SIZE) ? (bufferSize - shiftingBuffer)
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
                                        }
                                    }
                              windowMutex.unlock();
                            }
                    }
                }

                //  cout << "Current window = [";
                //  for (int i = 0; i < windowSize; i++) {
                //     sequenceNumber = lar + i + 1;
                //     cout << sequenceNumber << ",";
                //  }
                //       cout << "]" << endl;

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

        auto duration =  chrono::duration_cast<chrono::milliseconds>(stop - start).count();

        totalBandwidth = (nbytes / duration) * .00781248;

        cout << "Session successfully terminated \n" << endl;
        cout << "Number of original packets sent: " << packets_sent << endl;
        cout << "Number of retransmitted packets: " << retransmitted << endl;
         cout << "Total elapsed time: " << (duration/1000) << " seconds" << "\n" << endl;
        cout << "Total throughput (Mbps): " << totalBandwidth << endl;
        cout << "Effective throughput: " << (packets_sent - retransmitted) << endl;

    fclose(file);
    delete[] windowMaskACK;
    delete[] windowTransferTime;
    ackListener.detach();

    cout << "\nDone" << endl;
    return 0;
}

