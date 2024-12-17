#include "io/socket_manager.h"
#include "runtime_config.h"
#include "firmware_config.h"
#include "pch.h"
#include "ML/onnx_model.h"
#include "algorithms/IMU_processor.h"
#include "algorithms/fir_filter.h"

using TimePoint = std::chrono::system_clock::time_point;

/**
 * @brief prints the mode (Release or Debug) that the program was compiled with
 *
 */
void printMode()
{
#ifdef DEBUG
    std::cout << "Running Debug Mode" << std::endl;
#else
    std::cout << "Running Release Mode" << std::endl;
#endif
}

/**
 * @brief Parses a JSON configuration file and initializes session and runtime parameters.
 *
 * This function reads a JSON configuration file, extracts parameters for the `SocketManager`
 * and `RuntimeConfig` objects, and sets up runtime values from command-line arguments.
 * If the configuration file or required fields are missing, the function throws exceptions.
 *
 * @param socketManager A reference to a `SocketManager` object to configure network settings.
 * @param runtimeConfig A reference to an `RuntimeConfig` object to initialize runtime settings.
 * @param argv Command-line arguments, where `argv[1]` is the JSON file path and `argv[2]` is the program runtime in seconds.
 * @throws std::runtime_error If the JSON file cannot be opened or required fields are missing.
 */
void parseJsonConfig(FirmwareConfig &firmwareConfig, RuntimeConfig &runtimeConfig, const std::string &jsonFilePath)
{
    std::ifstream inputFile(jsonFilePath);
    if (!inputFile.is_open())
    {
        throw std::runtime_error("Unable to open JSON file: " + jsonFilePath);
    }

    nlohmann::json jsonConfig;
    inputFile >> jsonConfig;

    // Configure SocketManager parameters
    std::string udpIp = jsonConfig.at("IPAddress").get<std::string>();
    if (udpIp == "self")
    {
        udpIp = "127.0.0.1";
    }
    int udpPort = jsonConfig.at("Port").get<int>();

    runtimeConfig.socketManger = std::make_unique<SocketManager>(udpPort, udpIp);

    // Configure RuntimeConfig parameters
    runtimeConfig.speedOfSound = jsonConfig.at("SpeedOfSound").get<float>();
    runtimeConfig.energyDetectionThreshold = jsonConfig.at("EnergyDetectionThreshold").get<float>();
    runtimeConfig.amplitudeDetectionThreshold = jsonConfig.at("AmplitudeDetectionThreshold").get<float>();

    // filtering
    std::string filterWeightsPath = jsonConfig.at("FilterWeights").get<std::string>();
    
    if (filterWeightsPath.empty()){
        std::cout << "no filter" << std::endl;
        runtimeConfig.filter = std::make_unique<FrequencyDomainNoFilterStrategy>(firmwareConfig.CHANNEL_SIZE, firmwareConfig.NUM_CHAN);
    }
    else{
        runtimeConfig.filter = std::make_unique<FrequencyDomainFilterStrategy>(filterWeightsPath, firmwareConfig.CHANNEL_SIZE, firmwareConfig.NUM_CHAN);
        std::cout << "using filter" << std::endl;
    }

    runtimeConfig.receiverPositionsPath = jsonConfig.at("ReceiverPositions").get<std::string>();

    if (jsonConfig.at("Enable_Tracking").get<bool>())
    {
        std::chrono::seconds trackerClusteringFrequency = std::chrono::seconds(jsonConfig.at("Cluster_Frequency_In_Seconds").get<int>());
        std::chrono::seconds trackerClusteringWindow = std::chrono::seconds(jsonConfig.at("Cluster_Window_In_Seconds").get<int>());
        runtimeConfig.tracker = std::make_unique<Tracker>(0.04f, 15, 4, "",
                                                          trackerClusteringFrequency,
                                                          trackerClusteringWindow);
    }

    if (jsonConfig.at("Use_IMU").get<bool>())
    {
        runtimeConfig.imuManager = std::make_unique<ImuProcessor>(firmwareConfig.IMU_BYTE_SIZE);
    }

    // Initialize ONNX model if model path provided
    std::string onnxModelPath = jsonConfig.at("ONNX_model_path").get<std::string>();
    if (!onnxModelPath.empty())
    {
        std::string onnxModelNormalizationPath = jsonConfig.at("ONNX_model_normalization").get<std::string>();
        runtimeConfig.onnxModel = std::make_unique<ONNXModel>(onnxModelPath, onnxModelNormalizationPath);
    }
}