#include "main_utils.h"
#include "runtime_config.h"
#include "firmware_config.h"
#include "session.h"
#include "threads/listener_thread.h"
#include "threads/processor_thread.h"
#include "io/socket_manager.h"

// valgrind --log-file=grind2.txt --leak-check=yes --show-possibly-lost=no ./debug/HarpListenDebug self 1045 1240 100 30
// valgrind --tool=massif --pages-as-heap=yes ./bin/HarpListen self 1045 1240 100 10000
//  profiling the stack (--stacks=yes) slows down performance significantly
//  --pages-as-heap=yes likely to also slow down program
//  perf record -e cycles,instructions -g ./debug/HarpListenDebug config_files/volumetric.json 120

int main(int argc, char *argv[])
{
    printMode(); // print debug or release mode

    // Instantiate firmware configuration varaibles and socket manager class
    FirmwareConfig firmwareConfig;
    SocketManager socketManager;

    // Main processing loop
    while (true)
    {
        Session sess;
        RuntimeConfig runtimeConfig;

        // Initialize the socket and populate runtimeConfig entries
        parseJsonConfig(socketManager, runtimeConfig, argv);

        initializeRuntimeObjects(runtimeConfig, firmwareConfig);

        socketManager.restartListener(); // Reset the socket

        runtimeConfig.programStartTime = std::chrono::system_clock::now(); // Start experiment timer

        // Create threads for listening to UDP packets and processing data
        std::thread listenerThread(udpListener, std::ref(sess), std::ref(socketManager), firmwareConfig.PACKET_SIZE);
        std::thread processorThread(dataProcessor, std::ref(sess), std::ref(firmwareConfig), std::ref(runtimeConfig));

        // Wait for threads to finish
        listenerThread.join();
        processorThread.join();

        std::cout << "Restarting threads..." << std::endl;
    }
    return 0;
}
