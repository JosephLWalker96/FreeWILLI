#include "firmware_config.h"
#include "shared_data_manager.h"
#include "threads/listener_thread.h"
#include "threads/processor_thread.h"
#include "io/socket_manager.h"
#include "main_utils.h"

int main(int argc, char *argv[])
{
    printMode();

    while (true)
    {

        auto [socketVariables, pipelineVars] = parseJsonConfig(std::string(argv[1]));
        SocketManager socketManager(socketVariables);
        
        SharedDataManager sess;
        Pipeline pipeline(sess, pipelineVars);

        pipeline.programRuntime = std::chrono::seconds(std::stoi(argv[2])); // program run duration
        pipeline.programStartTime = std::chrono::system_clock::now(); // Start experiment timer

        // Create threads for listening to UDP packets and processing data
        std::thread listenerThread(udpListener, std::ref(sess), std::ref(socketManager));
        std::thread processorThread(&Pipeline::process, &pipeline);
        // Wait for threads to finish
        listenerThread.join();
        processorThread.join();

        std::cout << "Restarting threads..." << std::endl;
    }
    return 0;
}
