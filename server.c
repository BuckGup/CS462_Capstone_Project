#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <arpa/inet.h>
#include <iostream>

#define SIZE 32
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

        cout << "Rec packet# " << i << " -encrypted as ";

        for (int i = 0; i < packetSIZE; i++) {

            if (i < 4 || i > (packetSIZE - 5)) {
                //printf("%x", buffer[i]);
                cout << std::hex << (0xFF & buffer[i]);
            }
            buffer[i] = buffer[i] ^ *cipherKey;
        }

        cout << std::dec << "\n";


        fprintf(fp, "%s", buffer);
        bzero(buffer, packetSIZE);
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
    //char buffer[SIZE];

    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd < 0) {
        perror("[-]Error in socket");
        exit(1);
    }
    printf("Connect to IP address: ");
    //fgets(IP_ADDRESS,sizeof(IP_ADDRESS),stdin);
    scanf("%s", IP_ADDRESS);

    printf("Port#: ");
    scanf("%d", &PORT);

    printf("Pkt size#: ");
    scanf("%d", &PacketSIZE);

    printf("key: ");
    scanf("%s", cipherKey);


    printf("Filename: ");
    scanf("%s", userFileName);


    printf("[+]Server socket created successfully.\n");

    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(PORT);
    server_addr.sin_addr.s_addr = inet_addr(IP_ADDRESS);

    e = bind(sockfd, (struct sockaddr *) &server_addr, sizeof(server_addr));
    using namespace std;
    if (e < 0) {
        perror("[-]Error in bind");
        exit(1);
    }
    printf("[+]Binding successfull.\n");

    if (listen(sockfd, 10) == 0) {
        printf("[+]Listening....\n");
    } else {
        perror("[-]Error in listening");
        exit(1);
    }

    addr_size = sizeof(new_addr);
    new_sock = accept(sockfd, (struct sockaddr *) &new_addr, &addr_size);
    write_file(new_sock, PacketSIZE);
    printf("Receive success!\n");


    strcpy(MD5cmd, "md5sum ");
    strcat(MD5cmd, userFileName);
    char MD5Output[1035];

    fp2 = popen(MD5cmd, "r");

    if (fgets(MD5Output, sizeof(MD5Output), fp2) != NULL) {
        printf("MD5: ");
        printf("%s", MD5Output);
    }



    return 0;
}