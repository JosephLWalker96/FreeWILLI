#include "socket_manager.h"

/**
 * @brief Constructs a new SocketManager and initializes the UDP socket.
 * @throws std::runtime_error If the socket cannot be created.
 */
SocketManager::SocketManager()
    : mDatagramSocket(socket(AF_INET, SOCK_DGRAM, 0))
{
    if (mDatagramSocket == -1)
    {
        throw std::runtime_error("Error creating socket\n");
    }
}

/**
 * @brief (Re)starts the UDP listener by closing the existing socket and creating a new one.
 * Configures the socket based on the assigned IP and port.
 * @throws std::runtime_error If the socket cannot be closed, created, or bound.
 */
void SocketManager::restartListener()
{
    std::cout << "Restarting listener:\n";

    // Close the existing socket
    if (close(mDatagramSocket) == -1)
    {
        throw std::runtime_error("Failed to close socket\n");
    }

    // Create a new socket
    mDatagramSocket = socket(AF_INET, SOCK_DGRAM, 0);
    if (mDatagramSocket == -1)
    {
        throw std::runtime_error("Error creating socket\n");
    }

    // Prepare the address structure
    struct sockaddr_in serverAddr;
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_addr.s_addr = inet_addr(mUdpIp.c_str());
    serverAddr.sin_port = htons(mUdpPort);

    // Handle specific IP cases
    if (mUdpIp == "192.168.100.220")
    {
        std::cout << "Sending wake-up data to IP address of data logger\n";
        const char *message1 = "Open";
        unsigned char message2[96] = {0};
        unsigned char fullMessage[100];
        std::memcpy(fullMessage, message1, 4);
        std::memcpy(fullMessage + 4, message2, 96);
        if (sendto(mDatagramSocket, fullMessage, sizeof(fullMessage), 0,
                   (struct sockaddr *)&serverAddr, sizeof(serverAddr)) < 0)
        {
            throw std::runtime_error("Error sending wake-up data packet to data logger\n");
        }
    }
    // Bind the socket if it's not a data logger IP
    else if (bind(mDatagramSocket, (struct sockaddr *)&serverAddr, sizeof(serverAddr)) == -1)
    {
        throw std::runtime_error("Error binding socket\n");
    }
}