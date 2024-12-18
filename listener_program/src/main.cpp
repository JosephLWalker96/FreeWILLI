#include "main_utils.h"
#include "runtime_config.h"
#include "firmware_config.h"
#include "shared_data_manager.h"
#include "threads/listener_thread.h"
#include "threads/processor_thread.h"
#include "io/socket_manager.h"

int main(int argc, char *argv[])
{
    printMode(); // print debug or release mode

    // Instantiate firmware configuration varaibles
    FirmwareConfig firmwareConfig;

    // Main processing loop
    while (true)
    {
        SharedDataManager sess;
        RuntimeConfig runtimeConfig;
        runtimeConfig.programRuntime = std::chrono::seconds(std::stoi(argv[2])); // program run duration

        // Initialize the socket and populate runtimeConfig entries
        parseJsonConfig(firmwareConfig, runtimeConfig, std::string(argv[1]));

        runtimeConfig.socketManger->restartListener();

        runtimeConfig.programStartTime = std::chrono::system_clock::now(); // Start experiment timer

        // Create threads for listening to UDP packets and processing data
        std::thread listenerThread(udpListener, std::ref(sess), runtimeConfig.socketManger.get(), firmwareConfig.PACKET_SIZE);
        std::thread processorThread(dataProcessor, std::ref(sess), std::ref(firmwareConfig), std::ref(runtimeConfig));

        // Wait for threads to finish
        listenerThread.join();
        processorThread.join();

        std::cout << "Restarting threads..." << std::endl;
    }
    return 0;
}
