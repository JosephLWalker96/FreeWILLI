#pragma once

#include "pch.h"
#include "ML/onnx_model.h"
#include "tracker/tracker.h"
#include "algorithms/IMU_processor.h"

using TimePoint = std::chrono::system_clock::time_point;
using namespace std::chrono_literals;

class RuntimeConfig
{
public:
    std::string detectionOutputFile = "";
    bool enableTracking = false;
    bool useImu = false;
    std::chrono::seconds programRuntime;
    TimePoint programStartTime;
    float energyDetectionThreshold = 100.0f;    // 28.0f;
    float amplitudeDetectionThreshold = 100.0f; // 28.0f;
    float speedOfSound = 1482.965459;

    std::string udpIp;
    int udpPort;

    std::string filterWeightsPath = "";
    std::string receiverPositionsPath = "";
    std::string onnxModelPath = "";
    std::string onnxModelNormalizationPath = "";

    fftwf_plan forwardFFT = nullptr;

    std::unique_ptr<ONNXModel> onnxModel = nullptr;

    std::unique_ptr<Tracker> tracker = nullptr;
    std::chrono::seconds trackerClusteringFrequency = 60s;
    std::chrono::seconds trackerClusteringWindow = 30s;

    std::unique_ptr<ImuProcessor> imuManager = nullptr;
};
