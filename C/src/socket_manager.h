#pragma once
#include "pch.h"

class SocketManager {
public:
    // Constructor
    SocketManager();

    // Method to restart listener
    void RestartListener();

    // Data members
    int datagramSocket; // UDP socket
    int UDP_PORT;       // Listening port
    std::string UDP_IP; // IP address of data logger or simulator
};