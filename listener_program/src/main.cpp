#include "firmware_1240.h"
#include "io/socket_manager.h"
#include "shared_data_manager.h"
#include "threads/listener_thread.h"
#include "threads/pipeline.h"
#include "utils.h"

int main(int argc, char* argv[])
{
    printMode();

    auto [socketVariables, pipelineVars] = parseJsonConfig(std::string(argv[1]));

    std::unique_ptr<ISocketManager> socketManager = std::make_unique<SocketManager>(socketVariables);

    while (true)
    {
        socketManager->restartListener();

        SharedDataManager sharedDataManager;
        OutputManager outputManager(
            std::chrono::seconds(std::stoi(argv[2])), pipelineVars.integrationTesting, pipelineVars.loggingDirectory);

        Pipeline pipeline(outputManager, sharedDataManager, pipelineVars);

        // Create threads for listening for incoming data packets and processing data
        std::thread producerThread(runListenerLoop, std::ref(sharedDataManager), socketManager.get());
        std::thread consumerThread(&Pipeline::process, &pipeline);

        // Wait for threads to finish
        producerThread.join();
        consumerThread.join();

        std::cout << "Restarting threads..." << std::endl;
    }
    return 0;
}
