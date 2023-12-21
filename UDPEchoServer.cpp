/*UDPEchoServer.cpp
 *Normal Build
 * $ g++ -std=c++14 UDPEchoServer.cpp -o UDPEchoServer
 * Usage :
 * $ UDPEchoServer <port>
*/


#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <chrono>
#include <fstream>

#define PORT 12000
#define MAXLINE 1024

// Function prototypes
int main(int argc, char* argv[]);
void INTHandler(int sig);
void handleFileTransfer(int sockid, const sockaddr_in& clientaddr, const char* filename);

int main(int argc, char* argv[]) {
    int sockid;
    struct sockaddr_in serveraddr, clientaddr;

    int len;
    int n;
    int msg_count = 0;

    char buff_in[MAXLINE];
    char buff_out[MAXLINE + 40];

    int port = PORT;
    if (argc == 2) {
        port = atoi(argv[1]);
    }

    //create socket descriptor
    //- family : AF_INET
    // -type: SOCK_DGRAM
    // -protocol
    if ((sockid = socket(AF_INET, SOCK_DGRAM, 0)) < 0) {
        perror("socket creation failure");
        exit(EXIT_FAILURE);
    }

    //clear sockaddr_in structures
    memset(&serveraddr, 0, sizeof(serveraddr));
    memset(&clientaddr, 0, sizeof(clientaddr));

    // fill server information
    serveraddr.sin_family = AF_INET;
    serveraddr.sin_addr.s_addr = INADDR_ANY;
    serveraddr.sin_port = htons(port);

    // Bind the socket with the server address
    if (bind(sockid, (const sockaddr*)&serveraddr, sizeof(serveraddr)) < 0) {
        perror("bind failed");
        exit(EXIT_FAILURE);
    }

    signal(SIGINT, INTHandler);
    printf("UDP Echo server listening on port %d\nPress ctrl+c to stop...\n", port);

    len = sizeof(clientaddr);
    while (true) {
        // UDP receive
        // - socket descriptor
        // - buffer for message
        // - buffer size
        // - type of message(flags)
        // - null or ptr to where save the client address
        // - ptr to address length

        n = recvfrom(sockid, buff_in, MAXLINE, MSG_WAITALL, (sockaddr*)&clientaddr, (socklen_t*)&len);

        if (n > 0) {
            //There was a message
            buff_in[n] = '\0';
            msg_count++;

            printf("Client request (%d) received from %s using port %u: %s\n",
                   msg_count, inet_ntoa(clientaddr.sin_addr), clientaddr.sin_port, buff_in);

            if (strncmp(buff_in, "FILE:", 5) == 0) {
                //If message starts with FILE: handle the file transfer
                char* filename = buff_in + 5;
                handleFileTransfer(sockid, clientaddr, filename);
            } else {
                //Respond with a regular message
                sprintf(buff_out, "[%d]: %s", msg_count, buff_in);
                sendto(sockid, buff_out, strlen(buff_out), 0, (const sockaddr*)&clientaddr, len);
            }
        } else {
            perror("recvfrom returned error");
            exit(EXIT_FAILURE);
        }
    }
    return 0;
}

//Function for the file transfer
void handleFileTransfer(int sockid, const sockaddr_in& clientaddr, const char* filename) {
    int len;
    char buff_in[MAXLINE];
    char buff_out[MAXLINE + 40];

    //Open file
    std::ofstream file(filename, std::ios::out | std::ios::binary);
    if (!file.is_open()) {
        perror("Error opening file for writing");
        return;
    }

    while (true) {
        int n = recvfrom(sockid, buff_in, MAXLINE, MSG_WAITALL, (sockaddr*)&clientaddr, (socklen_t*)&len);
        if (n > 0) {
            //check for the end of file transfer
            if (strncmp(buff_in, "FILE_END", 8) == 0) {
                break;
            }

            file.write(buff_in, n);
        } else {
            perror("recvfrom returned error");
            exit(EXIT_FAILURE);
        }
    }
    file.close();
    sendto(sockid, buff_out, strlen(buff_out), 0, (const sockaddr*)&clientaddr, len);
}

void INTHandler(int sig) {
    char c;

    signal(sig, SIG_IGN);
    printf("\nDo you really want to quit? [y/n] ");
    c = getchar();
    if (c == 'y' || c == 'Y') {
        exit(0);
    } else {
        signal(SIGINT, INTHandler);
    }
}
