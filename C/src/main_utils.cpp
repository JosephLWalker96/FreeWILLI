#include "io/socket_manager.h"
#include "runtime_config.h"
#include "firmware_config.h"
#include "pch.h"
#include "ML/onnx_model.h"
#include "algorithms/IMU_processor.h"

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
void parseJsonConfig(RuntimeConfig &runtimeConfig, const std::string& jsonFilePath)
{
    std::ifstream inputFile(jsonFilePath);
    if (!inputFile.is_open())
    {
        throw std::runtime_error("Unable to open JSON file: " + jsonFilePath);
    }

    nlohmann::json jsonConfig;
    inputFile >> jsonConfig;

    // Configure SocketManager parameters
    runtimeConfig.udpIp = jsonConfig.at("IPAddress").get<std::string>();
    if (runtimeConfig.udpIp == "self")
    {
        runtimeConfig.udpIp = "127.0.0.1";
    }
    runtimeConfig.udpPort = jsonConfig.at("Port").get<int>();

    // Configure RuntimeConfig parameters
    runtimeConfig.speedOfSound = jsonConfig.at("SpeedOfSound").get<float>();
    runtimeConfig.energyDetectionThreshold = jsonConfig.at("EnergyDetectionThreshold").get<float>();
    runtimeConfig.amplitudeDetectionThreshold = jsonConfig.at("AmplitudeDetectionThreshold").get<float>();
    runtimeConfig.filterWeightsPath = jsonConfig.at("FilterWeights").get<std::string>();
    runtimeConfig.receiverPositionsPath = jsonConfig.at("ReceiverPositions").get<std::string>();
    runtimeConfig.onnxModelPath = jsonConfig.at("ONNX_model_path").get<std::string>();
    runtimeConfig.onnxModelNormalizationPath = jsonConfig.at("ONNX_model_normalization").get<std::string>();

    runtimeConfig.enableTracking = jsonConfig.at("Enable_Tracking").get<bool>();
    runtimeConfig.trackerClusteringFrequency = std::chrono::seconds(jsonConfig.at("Cluster_Frequency_In_Seconds").get<int>());
    runtimeConfig.trackerClusteringWindow = std::chrono::seconds(jsonConfig.at("Cluster_Window_In_Seconds").get<int>());

    runtimeConfig.useImu = jsonConfig.at("Use_IMU").get<bool>();
}

void initializeRuntimeObjects(RuntimeConfig &runtimeConfig, const FirmwareConfig &firmwareConfig)
{

    // Initialize ONNX model if model path provided
    if (!runtimeConfig.onnxModelPath.empty())
    {
        runtimeConfig.onnxModel = std::make_unique<ONNXModel>(runtimeConfig.onnxModelPath, runtimeConfig.onnxModelNormalizationPath);
    }

    // Initialize tracker if specified
    if (runtimeConfig.enableTracking)
    {
        runtimeConfig.tracker = std::make_unique<Tracker>(0.04f, 15, 4, "",
                                                          runtimeConfig.trackerClusteringFrequency,
                                                          runtimeConfig.trackerClusteringWindow);
    }

    // Initialize IMU manager
    if (runtimeConfig.useImu)
    {
        runtimeConfig.imuManager = std::make_unique<ImuProcessor>(firmwareConfig.IMU_BYTE_SIZE);
    }
}