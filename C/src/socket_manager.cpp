#include "socket_manager.h"
/*
#include <iostream>
#include <stdexcept>
#include <cstring>
#include <unistd.h>     // For close()
#include <sys/types.h>  // For socket functions
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
*/
SocketManager::SocketManager()
    : datagramSocket(socket(AF_INET, SOCK_DGRAM, 0))
{
    if (datagramSocket == -1) {
        throw std::runtime_error("Error creating socket\n");
    }
}

void SocketManager::RestartListener() {
    /**
     * @brief (Re)starts the UDP listener. It closes the existing socket connection and creates a new one.
     * Additionally, it handles specific IP cases for data loggers.
     */
    std::cout << "Restarting listener:\n";

    // Close the existing socket
    if (close(datagramSocket) == -1) {
        throw std::runtime_error("Failed to close socket\n");
    }

    // Create a new socket
    datagramSocket = socket(AF_INET, SOCK_DGRAM, 0);
    if (datagramSocket == -1) {
        throw std::runtime_error("Error creating socket\n");
    }

    // Prepare the address structure
    struct sockaddr_in serverAddr;
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_addr.s_addr = inet_addr(UDP_IP.c_str());
    serverAddr.sin_port = htons(UDP_PORT);

    // Handle specific IP cases
    if (UDP_IP == "192.168.100.220") {
        std::cout << "Sending wake-up data to IP address of data logger\n";
        const char* m1 = "Open";
        unsigned char m2[96] = {0};
        unsigned char message[100];
        std::memcpy(message, m1, 4);
        std::memcpy(message + 4, m2, 96);
        if (sendto(datagramSocket, message, sizeof(message), 0,
                   (struct sockaddr*)&serverAddr, sizeof(serverAddr)) < 0) {
            throw std::runtime_error("Error sending wake-up data packet to data logger\n");
        }
    }
    // Bind the socket if it's not a data logger IP
    else if (bind(datagramSocket, (struct sockaddr*)&serverAddr, sizeof(serverAddr)) == -1) {
        throw std::runtime_error("Error binding socket\n");
    }
}
