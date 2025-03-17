#include "udp_socket_manager.h"

#include "../utils.h"

/**
 * @brief Constructs a new SocketManager and initializes the UDP socket.
 * @throws std::runtime_error If the socket cannot be created.
 */
UdpSocketManager::UdpSocketManager(const SocketVariables& socketVariables)
    : mDatagramSocket(socket(AF_INET, SOCK_DGRAM, 0)),
      mUdpPort(socketVariables.port),
      mUdpIp(socketVariables.ipAddress),
      mDataBytes()
{
    if (mUdpIp == "self")
    {
        mUdpIp = "127.0.0.1";
    }

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
void UdpSocketManager::restartListener()
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

    struct sockaddr_in serverAddr;
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_addr.s_addr = inet_addr(mUdpIp.c_str());
    serverAddr.sin_port = htons(mUdpPort);

    if (mUdpIp == "192.168.100.220")
    {
        std::cout << "Sending wake-up data to IP address of data logger\n";
        const char* message1 = "Open";
        unsigned char message2[96] = {0};
        unsigned char fullMessage[100];
        std::memcpy(fullMessage, message1, 4);
        std::memcpy(fullMessage + 4, message2, 96);
        if (sendto(
                mDatagramSocket, fullMessage, sizeof(fullMessage), 0, (struct sockaddr*)&serverAddr,
                sizeof(serverAddr)) < 0)
        {
            throw std::runtime_error("Error sending wake-up data packet to data logger\n");
        }
    }
    else if (bind(mDatagramSocket, (struct sockaddr*)&serverAddr, sizeof(serverAddr)) == -1)
    {
        throw std::runtime_error("Error binding socket\n");
    }
}

/**
 * @brief Receives data from the UDP socket.
 *
 * @param buffer Pointer to the buffer to receive data into.
 * @param length Length of the buffer.
 * @param flags Flags for the recvfrom call.
 * @param addr Pointer to the sockaddr structure that will be filled with the sender address.
 * @param addrlen Pointer to the size of the addr structure.
 * @return Number of bytes received, or -1 on error.
 */
int UdpSocketManager::receiveData(int flags, struct sockaddr* addr, socklen_t* addrlen)
{
    std::cout << "[DEBUG] Calling recvfrom() on socket: " << mDatagramSocket << std::endl;

    int bytesReceived = recvfrom(mDatagramSocket, mDataBytes.data(), mDataBytes.size(), flags, addr, addrlen);
    if (bytesReceived > 0)
    {
        std::cout << "[DEBUG] Received " << bytesReceived << " bytes" << std::endl;

        mDataBytes.assign(
            static_cast<uint8_t*>(mDataBytes.data()), static_cast<uint8_t*>(mDataBytes.data()) + bytesReceived);
    }
    else
    {
        std::cout << "[DEBUG] recvfrom() returned " << bytesReceived << ", errno: " << errno << " (" << strerror(errno)
                  << ")" << std::endl;
    }
    return bytesReceived;
}