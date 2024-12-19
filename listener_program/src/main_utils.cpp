#include "main_utils.h"
#include "pch.h"

/**
 * @brief Parses the JSON configuration file to initialize socket and pipeline variables.
 */
auto parseJsonConfig(const std::string &jsonFilePath) -> std::tuple<SocketVariables, PipelineVariables>{
    std::ifstream inputFile(jsonFilePath);
    if (!inputFile.is_open()) {
        throw std::runtime_error("Unable to open JSON file: " + jsonFilePath);
    }

    nlohmann::json jsonConfig;
    inputFile >> jsonConfig;

    SocketVariables socketVariables;
    PipelineVariables pipelineVariables;

    // Configure SocketVariables parameters
    socketVariables.udpIp = jsonConfig.at("IPAddress").get<std::string>();
    socketVariables.udpPort = jsonConfig.at("Port").get<int>();

    // Configure PipelineVariables parameters
    pipelineVariables.speedOfSound = jsonConfig.at("SpeedOfSound").get<float>();
    pipelineVariables.energyDetectionThreshold = jsonConfig.at("EnergyDetectionThreshold").get<float>();
    pipelineVariables.amplitudeDetectionThreshold = jsonConfig.at("AmplitudeDetectionThreshold").get<float>();
    pipelineVariables.filterWeightsPath = jsonConfig.at("FilterWeights").get<std::string>();
    pipelineVariables.receiverPositionsPath = jsonConfig.at("ReceiverPositions").get<std::string>();
    pipelineVariables.enableTracking = jsonConfig.at("Enable_Tracking").get<bool>();
    pipelineVariables.clusterFrequencyInSeconds = std::chrono::seconds(jsonConfig.at("Cluster_Frequency_In_Seconds").get<int>());
    pipelineVariables.clusterWindowInSeconds = std::chrono::seconds(jsonConfig.at("Cluster_Window_In_Seconds").get<int>());
    pipelineVariables.useImu = jsonConfig.at("Firmware_with_IMU").get<bool>();
    pipelineVariables.onnxModelPath = jsonConfig.at("ONNX_model_path").get<std::string>();
    pipelineVariables.onnxModelNormalizationPath = jsonConfig.at("ONNX_model_normalization").get<std::string>();

    return std::make_tuple(socketVariables, pipelineVariables);
}

/**
 * @brief Prints whether the program is running in Debug or Release mode.
 */
void printMode() {
#ifdef DEBUG
    std::cout << "Running Debug Mode" << std::endl;
#else
    std::cout << "Running Release Mode" << std::endl;
#endif
}