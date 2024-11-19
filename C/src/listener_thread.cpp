
#include "custom_types.h"
#include "process_data.h"
#include "TDOA_estimation.h"
#include "utils.h"
#include "socket_manager.h"
#include "session.h"
#include "pch.h"

//forward declarations
//class SocketManager;
//class Session;
//class Session;

// #include "udp_listener.h"
int packetCounter = 0; // this should only be used inside the UDPListener function, as it is not protected by a mutex
void UdpListener(Session &sess, SocketManager &socketManager, const int PACKET_SIZE)
{
    /**
     * @brief Listens for UDP packets and processes them, storing data in a buffer.
     *
     * This function continuously listens for incoming UDP packets on a specified socket.
     * It receives data and stores the received data into a buffer (sess.dataBuffer).
     * Statistics about the received packets are printed to the console.
     *
     * @param exp Reference to an Experiment object, which contains configuration details like PACKET_SIZE.
     * @param sess Reference to a Session object, which contains session-specific details and state, such as the datagram socket and buffers.
     *
     * @throws std::runtime_error if there is an error in receiving data from the socket.
     */

    try
    {
        struct sockaddr_in addr;
        socklen_t addrLength = sizeof(addr);
        int bytesReceived;
        const int printInterval = 500;
        const int receiveSize = PACKET_SIZE + 1; // + 1 to detect if more data than expected is being received
        int queueSize;
        auto startPacketTime = std::chrono::steady_clock::now();
        auto endPacketTime = startPacketTime;
        std::chrono::duration<double> durationPacketTime; // stores the average amount of time (seconds) between successive UDP packets, averaged over 'printInterval' packets
        std::vector<uint8_t> dataBytes(receiveSize);

        while (!sess.errorOccurred)
        {

            bytesReceived = recvfrom(socketManager.datagramSocket, dataBytes.data(), receiveSize, 0, (struct sockaddr *)&addr, &addrLength);

            if (bytesReceived == -1)
                throw std::runtime_error("Error in recvfrom: bytesReceived is -1");

            dataBytes.resize(bytesReceived); // Adjust size based on actual bytes received
            queueSize = sess.pushDataToBuffer(dataBytes);
            
            packetCounter += 1;
            if (packetCounter % printInterval == 0)
            {
                endPacketTime = std::chrono::steady_clock::now();
                durationPacketTime = endPacketTime - startPacketTime;
                std::stringstream msg; // compose message to dispatch
                msg << "Packets rec: " << packetCounter << " duration: " << std::fixed << std::setprecision(6) << durationPacketTime.count() / printInterval
                    << " queue size: " << queueSize << " processed packets: " << packetCounter - queueSize << " detections: " << sess.detectionCounter << std::endl;

                std::cout << msg.str(); // using one instance of "<<" makes the operation atomic
                startPacketTime = std::chrono::steady_clock::now();
            }

            if (queueSize > 1000)
            { // check if buffer has grown to an unacceptable size
                throw std::runtime_error("Buffer overflowing \n");
            }
        }
    }
    catch (const std::exception &e)
    {
        std::cerr << "Error occured in UDP Listener Thread: \n";

        std::stringstream msg; // compose message to dispatch
        msg << e.what() << std::endl;
        std::cerr << msg.str();

        sess.errorOccurred = true;
    }
}