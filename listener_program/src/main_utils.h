#pragma once
#include "pch.h"

/**
 * @struct PipelineVariables
 * @brief Holds configuration variables for the pipeline.
 */
struct PipelineVariables {
    float amplitudeDetectionThreshold = 0;
    std::string timeDomainDetector = "";
    std::string frequencyDomainDetector = "";
    float energyDetectionThreshold = 0;
    float speedOfSound = 0;
    std::string frequencyDomainStrategy = "";
    std::string filterWeightsPath = "";
    std::string receiverPositionsPath = "";
    bool enableTracking = false;
    std::chrono::seconds clusterFrequencyInSeconds;
    std::chrono::seconds clusterWindowInSeconds;
    bool useImu = false;
    std::string onnxModelPath = "";
    std::string onnxModelNormalizationPath = "";
};

/**
 * @struct SocketVariables
 * @brief Holds configuration variables for the socket.
 */
struct SocketVariables {
    std::string udpIp = "";
    int udpPort = -1;
};

/**
 * @brief Parses a JSON configuration file and returns initialized structs for pipeline and socket variables.
 * @param jsonFilePath The path to the JSON configuration file.
 * @return A tuple containing `SocketVariables` and `PipelineVariables` structs.
 * @throws std::runtime_error If the JSON file cannot be opened or required fields are missing.
 */
auto parseJsonConfig(const std::string &jsonFilePath) -> std::tuple<SocketVariables, PipelineVariables>;

/**
 * @brief Prints the current build mode (Debug or Release).
 */
void printMode();