#pragma once
#include "pch.h"

struct PipelineVariables
{
    std::chrono::seconds clusterFrequencyInSeconds;
    std::chrono::seconds clusterWindowInSeconds;

    float timeDomainThreshold = 0;
    float energyDetectionThreshold = 0;
    float speedOfSound = 0;

    bool integrationTesting = false;
    bool enableTracking = false;
    bool useImu = false;

    std::string firmware = "";
    std::string loggingDirectory = "";
    std::string timeDomainDetector = "";
    std::string frequencyDomainDetector = "";
    std::string frequencyDomainStrategy = "";
    std::string filterWeightsPath = "";
    std::string receiverPositionsPath = "";
    std::string onnxModelPath = "";
    std::string onnxModelNormalizationPath = "";
};