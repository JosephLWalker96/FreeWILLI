/*
@file main.cpp
@brief A program for receiving and processing UDP packets in real-time.

This program sets up a UDP listener to receive packets and processes them concurrently using multi-threading. 
It handles configuration dynamically and logs processed data. 
In case of errors, it attempts to restart to maintain continuous operation.

@note Requires FFTW3 for FFT operations and Eigen for linear algebra.

Example usage:
    ./HarpListen <IP> <Port> <FirmwareVersion> <EnergyDetectionThreshold> <RuntimeSeconds>
*/

#include "custom_types.h"
#include "process_data.h"
#include "TDOA_estimation.h"
#include "utils.h"
#include "listener_thread.h"
#include "socket_manager.h"
#include "session.h"
#include "processor_thread.h"
#include "onnx_model.h"

//valgrind --log-file=grind2.txt --leak-check=yes --show-possibly-lost=no ./debug/HarpListenDebug self 1045 1240 100 30
//valgrind --tool=massif --pages-as-heap=yes ./bin/HarpListen self 1045 1240 100 10000
// profiling the stack (--stacks=yes) slows down performance significantly
// --pages-as-heap=yes likely to also slow down program
// perf record -e cycles,instructions -g ./debug/HarpListenDebug config_files/volumetric.json 120

int main(int argc, char *argv[]) 
{
    PrintMode(); // print debug or release

    // Instantiate classes for configuration and socket handling
    ExperimentConfig expConfig;
    SocketManager socketManager;
    
    // Main processing loop
    while (true) 
    {
        Session sess;
        ExperimentRuntime expRuntime;
        
        // Initialize the session and experiment
        ParseJSONConfig(socketManager, expRuntime, argv);
        
        // Initialize ONNX model if model path provided
        if (!expRuntime.onnxModelPath.empty()) {
            expRuntime.onnxModel = std::make_unique<ONNXModel>(expRuntime.onnxModelPath, expRuntime.onnxModelScaling);
        }
        
        socketManager.RestartListener(); // Reset the listener state
        
        expRuntime.programStartTime = std::chrono::system_clock::now(); // Start experiment timer

        // Create threads for listening to UDP packets and processing data
        std::thread listenerThread(UdpListener, std::ref(sess), std::ref(socketManager), expConfig.PACKET_SIZE);
        std::thread processorThread(DataProcessor, std::ref(sess), std::ref(expConfig), std::ref(expRuntime));
        
        // Wait for threads to finish
        listenerThread.join();
        processorThread.join();

        // Handle any errors during execution
        if (sess.errorOccurred) {
            std::cout << "Restarting threads..." << std::endl;
        } 
        else {
            std::cout << "Unknown problem occurred" << std::endl;
        }
    }
    return 0;
}
