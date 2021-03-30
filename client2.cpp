//Client code by Sam Strecker
//11/23/2020
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <arpa/inet.h>
#include <string>
#include <iostream>

using namespace std;
char cipherKey[] = "";


void send_file(FILE *fp, int sockfd, int packetSize) {
    using namespace std;
    int j = 1;

    char data[packetSize];

    while (fgets(data, packetSize, fp) != NULL) {

        //std output to screen
        cout << "Sent encrypted packet# " << j << " -encrypted as ";

        for (int i = 0; i < packetSize; i++) {
            data[i] = data[i] ^ *cipherKey;

            //Only prints out the first 4 and last 4 hex values
            if (i < 4 || i > (packetSize - 5)) {
                //Converts to hex
                cout << std::hex << (0xFF & data[i]);
            }
        }
        //change stdout back to dec
        cout << std::dec << "\n";

        if (send(sockfd, data, sizeof(data), 0) == -1) {
            perror("Error in sending file.");
            exit(1);
        }
        bzero(data, packetSize);
        //Zeros out the data for the buffer to be filled again
        j++;
        //increments counter for loop
    }
    fclose(fp);
    return;
}


int main() {
    //declaration of variables
    int sockfd;
    char IP_ADDRESS[20];
    char fileName[20];
    int PORT;
    int f;
    int PacketSIZE;
    char MD5cmd[1000];
    FILE *fp2;
    struct sockaddr_in server_addr;
    FILE *fp;
    int packets_sent = 0;
    int retransmitted = 0;
    char s_err[5] = "";
    int drops[100];
    char error[5] = "";
    char protocol[4] = "";
    int packetSIZE = 0;
    char time[5] = "";
    int timeout = 0;
    int window_size = 0;
    int seq_num = 0;
    int packet_no = 0;
    int tot_time = 0;
    int tot_through = 0;
    int eff_through = 0;

    printf("Type of Protocol: SNW, GBN, or SR\n");
    scanf("%s", protocol);
    printf("Size of packet:\n");
    scanf("%d", &packetSIZE);
    printf("Timeout interval(user-specified(u) or ping calculated(p))\n");
    scanf("%s", time);
    if(strcmp(time, "u") == 0){
        printf("Enter timeout time:\n");
        scanf("%d", &timeout);
    }else{

    }
    printf("Size of sliding window:\n");
    scanf("%d", &window_size);
    printf("Range of sequence numbers\n");
    scanf("%d", &seq_num);
    printf("Situational Errors (none(n), randomly generated(r), user specified(u))");
    scanf("%s", s_err);
    if(strcmp(s_err, "u") == 0){
        int num = 0;
        int place = 0;
        printf("drop packets(d) or lose acks(l)\n");
        scanf("%s", error);
        printf("packet or ack numbers(-1 to exit):");
        while(scanf("%d", &num)){
            if(num == -1){ break;}
            drops[place] = num;
            place = place +1;
        }
    }else if(strcmp(s_err, "r") == 0){
        //randomly gen numbers
    }
    while(packet_no <5) { //packets left to send
        //send packet
        printf("Packet %d sent\n", packet_no);
        packets_sent = packets_sent + 1;
        int window[window_size];
        if(1==1){ //server response

            printf("Ack %d recieved\n", packet_no);

            printf("Current window = [");
            for(int i = 0; i< window_size -1; i++){
                printf("%d, ", window[i]);
            }
            printf("%d]\n", window[window_size]);
            packet_no = packet_no +1;

        }
    }
    printf("Session successfully terminated\n\n");
    printf("Number of original packets sent: %d\n", packets_sent);
    printf("Number of retransmitted packets: %d\n", retransmitted);
    printf("Total elapsed time: %d\n", tot_time);
    printf("Total throughput (Mbps): %d\n", tot_through);
    printf("Effective throughput: %d\n", eff_through);











    printf("Connect to IP address: ");
    fgets(IP_ADDRESS, sizeof(IP_ADDRESS), stdin);
    //Use fgets to make sure IPs aren't padded with zeros giving us an unwanted IP

    printf("Port#: ");
    scanf("%d", &PORT);

    //creates socket connection
    sockfd = socket(AF_INET, SOCK_STREAM, 0);

    if (sockfd < 0) {
        perror("Error in socket");
        exit(1);
    }

    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(PORT);
    server_addr.sin_addr.s_addr = inet_addr(IP_ADDRESS);

    printf("Server socket created successfully.\n");


    f = connect(sockfd, (struct sockaddr *) &server_addr, sizeof(server_addr));
    if (f == -1) {
        perror("Error in socket");
        exit(1);
    }

    //get user input
    printf("File to be sent: ");
    scanf("%s", fileName);
    printf("Pkt size#: ");
    scanf("%d", &PacketSIZE);
    printf("Enter encryption key: ");
    scanf("%s", cipherKey);

    fp = fopen(fileName, "r");

    if (fp == NULL) {
        perror("Error in reading file.");
        exit(1);
    }

    //calls helper function to recv the packets and writes them to the file locally
    send_file(fp, sockfd, PacketSIZE);
    close(sockfd);

    printf("Send Success!\n");

    //concats two strings together for the system call of md5sum
    strcpy(MD5cmd, "md5sum ");
    strcat(MD5cmd, fileName);

    char MD5Output[1035];

    fp2 = popen(MD5cmd, "r");

    if (fgets(MD5Output, sizeof(MD5Output), fp2) != NULL) {
        printf("MD5: ");
        printf("%s", MD5Output);
    }

    return 0;
}