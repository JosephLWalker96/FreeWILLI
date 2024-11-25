#pragma once
#include "../pch.h"

/**
 * @brief Manages UDP socket operations, including creating, restarting, and configuring the listener.
 */
class SocketManager
{
public:
    SocketManager();

    void restartListener();

    int mDatagramSocket; ///< UDP socket descriptor.
    int mUdpPort;        ///< Port number for the UDP connection.
    std::string mUdpIp;  ///< IP address of the data logger or simulator.
};