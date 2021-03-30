//Server code by Sam Strecker
//11/23/2020
#include <stdio.h>
#include <arpa/inet.h>
#include <iostream>
#include <stdlib.h>
#include <string.h>

char cipherKey[] = "";
char userFileName[] = "";

void write_file(int sockfd, int packetSIZE) {
    using namespace std;
    int n;
    int i = 1;
    FILE *fp;
    const char *filename = userFileName;
    char buffer[packetSIZE];

    fp = fopen(filename, "w");
    while (1) {

        n = recv(sockfd, buffer, packetSIZE, 0);
        if (n <= 0) {
            break;
            return;
        }
        //std output of the packets received
        cout << "Rec packet# " << i << " -encrypted as ";

        for (int i = 0; i < packetSIZE; i++) {
            //prints the first four and last four hex values of the packet
            if (i < 4 || i > (packetSIZE - 5)) {
                cout << std::hex << (0xFF & buffer[i]);
            }
            buffer[i] = buffer[i] ^ *cipherKey;
        }

        //change stdout back to dec
        cout << std::dec << "\n";

        fprintf(fp, "%s", buffer);
        //writes it to the file in the local directory
        bzero(buffer, packetSIZE);
        //Zeros out the data for the buffer to be filled again
        i++;
    }
    fclose(fp);
    return;
}

int main() {

    char MD5cmd[1000];
    char IP_ADDRESS[20];
    int PORT;
    int e;
    int PacketSIZE;
    FILE *fp2;

    int sockfd, new_sock;
    struct sockaddr_in server_addr, new_addr;
    socklen_t addr_size;

    //creation or socket
    sockfd = socket(AF_INET, SOCK_STREAM, 0);

    if (sockfd < 0) {
        perror("Error in socket");
        exit(1);
    }

    //get users input
    printf("Connect to IP address: ");
    scanf("%s", IP_ADDRESS);

    printf("Port#: ");
    scanf("%d", &PORT);

    printf("Pkt size#: ");
    scanf("%d", &PacketSIZE);
    printf("key: ");
    scanf("%s", cipherKey);

    printf("Filename: ");
    scanf("%s", userFileName);

    printf("Server socket created successfully.\n");

    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(PORT);
    server_addr.sin_addr.s_addr = inet_addr(IP_ADDRESS);

    e = bind(sockfd, (struct sockaddr *) &server_addr, sizeof(server_addr));
    using namespace std;
    if (e < 0) {
        perror("Error in bind");
        exit(1);
    }

    printf("Binding successfull.\n");

    //waiting for the client to send packets now
    if (listen(sockfd, 10) == 0) {
        printf("Listening....\n");
    } else {
        perror("Error in listening");
        exit(1);
    }

    addr_size = sizeof(new_addr);
    //new socket with the accept header
    new_sock = accept(sockfd, (struct sockaddr *) &new_addr, &addr_size);
    //calls helper function to recv the packets and writes them to the file locally

    write_file(new_sock, PacketSIZE);
    printf("Receive success!\n");

    //concats strings
    strcpy(MD5cmd, "md5sum ");
    strcat(MD5cmd, userFileName);
    char MD5Output[1035];

    fp2 = popen(MD5cmd, "r");

    //print out the md5sum of the file that was sent
    if (fgets(MD5Output, sizeof(MD5Output), fp2) != NULL) {
        printf("MD5: ");
        printf("%s", MD5Output);
    }
    return 0;
}