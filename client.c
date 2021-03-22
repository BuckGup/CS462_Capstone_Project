#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <arpa/inet.h>
#include <string>
#include <iostream>

#define SIZE 1024
using namespace std;
char cipherKey[] = "";


void send_file(FILE *fp, int sockfd, int packetSize) {
    using namespace std;
    int n;
    int j = 1;

    char data[packetSize];

    while (fgets(data, packetSize, fp) != NULL) {

        cout << "Sent encrypted packet# " << j << " -encrypted as ";

        for (int i = 0; i < packetSize; i++) {
            data[i] = data[i] ^ *cipherKey;

            if (i < 4 || i > (packetSize - 5)) {
                cout << std::hex << (0xFF & data[i]);
            }
        }
        cout << std::dec << "\n";

        if (send(sockfd, data, sizeof(data), 0) == -1) {
            perror("[-]Error in sending file.");
            exit(1);
        }
        bzero(data, packetSize);
        j++;
    }
    fclose(fp);
    return;
}


int main() {
    int sockfd;
    char IP_ADDRESS[20];
    char fileName[20];
    int PORT;
    int e;
    int PacketSIZE;
    char MD5cmd[1000];
    FILE *fp2;

    struct sockaddr_in server_addr;
    FILE *fp;
    printf("Connect to IP address: ");
    fgets(IP_ADDRESS, sizeof(IP_ADDRESS), stdin);

    printf("Port#: ");
    scanf("%d", &PORT);


    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd < 0) {
        perror("[-]Error in socket");
        exit(1);
    }
    printf("[+]Server socket created successfully.\n");

    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(PORT);
    server_addr.sin_addr.s_addr = inet_addr(IP_ADDRESS);

    e = connect(sockfd, (struct sockaddr *) &server_addr, sizeof(server_addr));
    if (e == -1) {
        perror("[-]Error in socket");
        exit(1);
    }

    printf("File to be sent: ");
    scanf("%s", fileName);
    printf("Pkt size#: ");
    scanf("%d", &PacketSIZE);
    printf("Enter encryption key: ");
    scanf("%s", cipherKey);

    fp = fopen(fileName, "r");

    if (fp == NULL) {
        perror("[-]Error in reading file.");
        exit(1);
    }

    send_file(fp, sockfd, PacketSIZE);
    close(sockfd);
    printf("Send Success!\n");


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