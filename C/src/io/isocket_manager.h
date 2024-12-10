#pragma once
#include "../pch.h"
#include <vector>
#include <string>

class ISocketManager {
public:
    virtual ~ISocketManager() = default;

    virtual void restartListener() = 0;
    virtual int getDatagramSocket() const = 0;
    virtual int getUdpPort() const = 0;
    virtual std::string getUdpIp() const = 0;

    virtual int receiveData(int flags, struct sockaddr *addr, socklen_t *addrlen) = 0;

    // New methods to manage the buffer of received data
    virtual std::vector<uint8_t>& getReceivedData() = 0;
    virtual void setReceiveBufferSize(size_t newSize) = 0;
};