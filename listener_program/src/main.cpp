#include "firmware_1240.h"
#include "shared_data_manager.h"
#include "threads/listener_thread.h"
#include "threads/pipeline.h"
#include "io/socket_manager.h"
#include "main_utils.h"

int main(int argc, char *argv[])
{
    printMode();

    auto [socketVariables, pipelineVars] = parseJsonConfig(std::string(argv[1]));
    std::unique_ptr<ISocketManager> socketManager = std::make_unique<SocketManager>(socketVariables); 

    while (true)
    {
        socketManager->restartListener();
        SharedDataManager sess;
        Pipeline pipeline(sess, pipelineVars);

        pipeline.programRuntime = std::chrono::seconds(std::stoi(argv[2])); // program run duration
        pipeline.programStartTime = std::chrono::system_clock::now(); // Start experiment timer

        // Create threads for listening to UDP packets and processing data
        std::thread producerThread(udpListener, std::ref(sess), socketManager.get());
        std::thread consumerThread(&Pipeline::process, &pipeline);
        
        // Wait for threads to finish
        producerThread.join();
        consumerThread.join();

        std::cout << "Restarting threads..." << std::endl;
    }
    return 0;
}
