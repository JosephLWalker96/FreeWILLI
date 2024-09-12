#include "pico/stdlib.h"
#include <FreeRTOS.h>
#include <iostream>
#include <lwip/sockets.h>
#include <unistd.h>
#include <lwip/inet.h>

#define PORT 8080
#define BUFFER_SIZE 1024
int main(){
    stdio_init_all();

    int sockfd;
    char buffer[BUFFER_SIZE];
    struct sockaddr_in serverAddr, clientAddr;

    // Create Socket
    if ((sockfd = socket(AF_INET, SOCK_DGRAM, 0))<0){
        std::cerr << "Socket creation failed" << std::endl;
    }

    memset(&serverAddr, 0, sizeof(serverAddr));
    memset(&clientAddr, 0, sizeof(clientAddr));

    // Fill server information
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_addr.s_addr = INADDR_ANY;
    serverAddr.sin_port = htons(PORT);

    // Bind the socket with the server address
    if (bind(sockfd, (const struct sockaddr *)&serverAddr, sizeof(serverAddr)) < 0) {
        std::cerr << "Bind failed" << std::endl;
        close(sockfd);
        return -1;
    }

    socklen_t len;
    int n;

    len = sizeof(clientAddr); //len is value/result

    while (true) {
        n = recvfrom(sockfd, (char *)buffer, BUFFER_SIZE, MSG_WAITALL, (struct sockaddr *)&clientAddr, &len);
        buffer[n] = '\0';
        std::cout << "Client: " << buffer << std::endl;

        sendto(sockfd, (const char *)buffer, strlen(buffer), 0, (const struct sockaddr *)&clientAddr, len);
        std::cout << "Echo message sent." << std::endl;
    }

    close(sockfd);
    return 0;
}