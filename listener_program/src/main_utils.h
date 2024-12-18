#pragma once
#include "pch.h"

struct PipelineVariables{
    float amplitudeDetectionThreshold = 0;
    float energyDetectionThreshold = 0;
    float speedOfSound = 0;
    std::string filterWeightsPath = "";
    std::string receiverPositionsPath = "";
    bool enableTracking = false;
    int clusterFrequencyInSeconds = 0;
    int clusterWindowInSeconds = 0;
    bool useImu = false;
    std::string onnxModelPath = "";
    std::string onnxModelNormalizationPath = "";

};

struct SocketVariables{
    std::string udpIp = "";
    int udpPort = -1;
};

/**
 * @brief Parses a JSON configuration file and returns initialized structs for pipeline and socket variables.
 *
 * This function reads a JSON configuration file, extracts parameters for the `SocketVariables`
 * and `PipelineVariables` structs, and returns them as a tuple.
 * If the configuration file or required fields are missing, the function throws exceptions.
 *
 * @param jsonFilePath The path to the JSON configuration file.
 * @return A tuple containing `SocketVariables` and `PipelineVariables` structs.
 * @throws std::runtime_error If the JSON file cannot be opened or required fields are missing.
 */
std::tuple<SocketVariables, PipelineVariables> parseJsonConfig(const std::string &jsonFilePath) {
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
    pipelineVariables.clusterFrequencyInSeconds = jsonConfig.at("Cluster_Frequency_In_Seconds").get<int>();
    pipelineVariables.clusterWindowInSeconds = jsonConfig.at("Cluster_Window_In_Seconds").get<int>();
    pipelineVariables.useImu = jsonConfig.at("Firmware_with_IMU").get<bool>();
    pipelineVariables.onnxModelPath = jsonConfig.at("ONNX_model_path").get<std::string>();
    pipelineVariables.onnxModelNormalizationPath = jsonConfig.at("ONNX_model_normalization").get<std::string>();

    return std::make_tuple(socketVariables, pipelineVariables);
}

void printMode(){
#ifdef DEBUG
    std::cout << "Running Debug Mode" << std::endl;
#else
    std::cout << "Running Release Mode" << std::endl;
#endif
}