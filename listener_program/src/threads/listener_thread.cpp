#include "../io/isocket_manager.h"
#include "../io/socket_manager.h"
#include "../pch.h"
#include "../shared_data_manager.h"

int packetCounter = 0;  // this should only be used inside the UDPListener
                        // function, as it is not protected by a mutex

/**
 * @brief Logs packet statistics at regular intervals and resets the packet
 * timer.
 *
 * @param packetCounter The total number of packets received so far.
 * @param printInterval The interval (in packets) at which to log statistics.
 * @param startPacketTime Start time of the last logged interval.
 * @param queueSize Current size of the shared queue.
 * @param processedPackets Number of processed packets.
 * @param detectionCount Number of detections recorded by the session.
 */
void logPacketStatistics(int packetCounter, int printInterval, std::chrono::steady_clock::time_point& startPacketTime,
                         int queueSize, int processedPackets, int detectionCount)
{
    auto endPacketTime = std::chrono::steady_clock::now();
    std::chrono::duration<double> durationPacketTime = endPacketTime - startPacketTime;

    // Compose log message using fixed formatting
    printf(
        "Packets rec: %d duration: %.6f queue size: %d processed packets: %d "
        "detections: %d\n",
        packetCounter, durationPacketTime.count() / printInterval, queueSize, processedPackets, detectionCount);

    startPacketTime = std::chrono::steady_clock::now();
}

/**
 * @brief Listens for incoming UDP packets, processes them, and stores data in a
 * buffer.
 *
 * @param sharedDataManager Reference to a SharedDataManager object for managing shared data across threads
 * @param socketManager Reference to a SocketManager object that manages the UDP
 * socket.
 * @throws std::runtime_error if there is an error receiving data from the
 * socket or if the buffer overflows.
 */
void runListenerLoop(SharedDataManager& sharedDataManager, ISocketManager* socketManager)
{
    try
    {
        struct sockaddr_in addr;
        socklen_t addrLength = sizeof(addr);
        constexpr int printInterval = 500;
        const int receiveSize = 2048;  // some max size

        // We let the SocketManager handle resizing internally as it receives
        // data
        socketManager->setReceiveBufferSize(receiveSize);

        auto startPacketTime = std::chrono::steady_clock::now();

        while (!sharedDataManager.errorOccurred)
        {
            // Receive data
            int bytesReceived = socketManager->receiveData(0, (struct sockaddr*)&addr, &addrLength);

            if (bytesReceived == -1) throw std::runtime_error("Error in receiveData: bytesReceived is -1");

            // The data is already resized in the SocketManager after reception,
            // or we can resize here if needed (commented out because
            // SocketManager does it):
            // socketManager.setReceiveBufferSize(bytesReceived);

            const std::vector<uint8_t>& dataBytes = socketManager->getReceivedData();

            int queueSize = sharedDataManager.pushDataToBuffer(dataBytes);

            packetCounter += 1;
            if (packetCounter % printInterval == 0)
            {
                logPacketStatistics(packetCounter, printInterval, startPacketTime, queueSize, packetCounter - queueSize,
                                    sharedDataManager.detectionCounter);
            }

            if (queueSize > 1000)
            {
                throw std::runtime_error("Buffer overflowing \n");
            }
        }
    } catch (const std::exception& e)
    {
        std::cerr << "Error occured in UDP Listener Thread: \n";
        std::stringstream msg;
        msg << e.what() << std::endl;
        std::cerr << msg.str();

        sharedDataManager.errorOccurred = true;
    }
}
