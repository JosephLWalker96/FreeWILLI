#pragma once

#include "pch.h"
#include "ML/onnx_model.h"
#include "tracker/tracker.h"
#include "io/socket_manager.h"
#include "algorithms/IMU_processor.h"
#include "algorithms/fir_filter.h"

using TimePoint = std::chrono::system_clock::time_point;
using namespace std::chrono_literals;

class RuntimeConfig
{
public:
    Eigen::MatrixXf channelData; // time domain data (inputs)

    std::string detectionOutputFile = "";

    std::chrono::seconds programRuntime;
    TimePoint programStartTime;
    float energyDetectionThreshold = 100.0f;    // 28.0f;
    float amplitudeDetectionThreshold = 100.0f; // 28.0f;
    float speedOfSound = 1482.965459;

    std::string receiverPositionsPath = "";


    // Buffer objects
    std::vector<std::vector<uint8_t>> dataBytesSaved;
    std::vector<std::chrono::system_clock::time_point> dataTimes; // timestamps of UDP packet

    std::unique_ptr<SocketManager> socketManger = nullptr;
    std::unique_ptr<IFrequencyDomainStrategy> filter = nullptr;
    std::unique_ptr<ONNXModel> onnxModel = nullptr;
    std::unique_ptr<Tracker> tracker = nullptr;
    std::unique_ptr<ImuProcessor> imuManager = nullptr;
};
