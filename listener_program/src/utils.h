#pragma once
#include "pch.h"

struct PipelineVariables
{
    bool integrationTesting = false;
    float timeDomainThreshold = 0;
    std::string loggingDirectory = "";
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

struct SocketVariables
{
    std::string udpIp = "";
    int udpPort = -1;
};

auto parseJsonConfig(const std::string& jsonFilePath) -> std::tuple<SocketVariables, PipelineVariables>;

void printMode();

std::string convertTimePointToString(const TimePoint& timePoint);
