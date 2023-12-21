/*UDPEchoClient.cpp
 *Normal Build
 * $ g++ -std=c++14 UDPEchoClient.cpp -o UDPEchoClient
 * Usage :
 * $ UDPEchoClient <serer> <port> <message>
*/


#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <netdb.h>
#include <fstream>
#include <chrono>

#define PORT 12000
#define MAXLINE 1024
#define HOST "127.0.0.1"

// Function prototypes
int main(int argc, char* argv[]);

// implementations
int main(int argc, char* argv[]) {
    int sockid;
    int port = PORT;

    int n;
    int len;

    char* host = (char*)HOST;
    char host_ip[80];
    char* message = (char*)"default message from client";
    char* filename = nullptr;
    char buff_in[MAXLINE + 40];

    struct sockaddr_in serveraddr;

    if (argc == 3 || argc == 4) {
        //2nd parameter is the host
        host = argv[1];
        //3rd parameter is the port
        port = atoi(argv[2]);

        if (argc == 4) {
            // 4th parameter is the message (optional)
            message = argv[3];
        }
    } else if (argc == 5) {
        //5th parameter checks for file
        if (strncmp(argv[3], "FILE:", 5) == 0) {
            message = argv[3];
        } else {
            fprintf(stderr, "Transfer parameter not correct\n");
            exit(EXIT_FAILURE);
        }

        filename = argv[4];
    }

    //create socket descriptor
    //- family : AF_INET
    // -type: SOCK_DGRAM
    // -protocol
    if ((sockid = socket(AF_INET, SOCK_DGRAM, 0)) < 0) {
        perror("socket creation failure");
        exit(EXIT_FAILURE);
    }

    //Initialize server address struct
    memset(&serveraddr, 0, sizeof(serveraddr));

    serveraddr.sin_family = AF_INET;
    serveraddr.sin_port = htons(port);

    struct hostent* h;
    struct in_addr** addr_list;

    if ((h = gethostbyname(host)) == NULL) {
        printf("gethostbyname returned error... using original\n");
        strcpy(host_ip, host);
    } else {
        addr_list = (struct in_addr**)h->h_addr_list;
        strcpy(host_ip, inet_ntoa(*addr_list[0]));
    }

    //convert IPv4 (or V6) addresses from text to binary
    if (inet_pton(AF_INET, host_ip, &serveraddr.sin_addr) <= 0) {
        perror("invalid address or address not supported");
        exit(EXIT_FAILURE);
    }
    
    //timeout Support
    struct timeval tv;

    tv.tv_sec = 2;
    tv.tv_usec = 500000;

    if (setsockopt(sockid, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv)) < 0) {
        perror("Error setting timeout");
    }

    auto start_time = std::chrono::high_resolution_clock::now();

    //check if message is a file
    if (strncmp(message, "FILE:", 5) == 0) {
        char* filename = message + 5;
        std::ifstream file(filename, std::ios::in | std::ios::binary);

        //Check that file is opened correctly
        if (!file.is_open()) {
            perror("Error opening file for reading");
            exit(EXIT_FAILURE);
        }

        char file_buff[MAXLINE];

        //read and send file content in chucks, tried this for grade 5
        while (file.read(file_buff, sizeof(file_buff))) {
            sendto(sockid, file_buff, file.gcount(), 0, (const sockaddr*)&serveraddr, sizeof(serveraddr));
        }

        //signal that this is end of file transfer
        sendto(sockid, "FILE_END", 8, 0, (const sockaddr*)&serveraddr, sizeof(serveraddr));

        file.close();
    } else {
        //if not file, send normal ascii message
        sendto(sockid, message, strlen(message), 0, (const sockaddr*)&serveraddr, sizeof(serveraddr));
    }

    n = recvfrom(sockid, buff_in, MAXLINE, MSG_WAITALL, (sockaddr*)&serveraddr, (socklen_t*)&len);
    //For measuring time
    auto end_time = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end_time - start_time);

    if (n > 0) {
        buff_in[n] = '\0';
        printf("Server returned: %s\n", buff_in);
        printf("Transfer time: %ld ms\n", duration.count());
    } else if (n == 0) {
        printf("Socket closed\n");
    } else {
        printf("recvfrom returned error, timeout or no data: %d\n", n);
    }
    close(sockid);
    return 0;
}
