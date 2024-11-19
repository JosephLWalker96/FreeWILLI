#pragma once

#include "pch.h"
#include "onnx_model.h"
#include "tracker.h"

// forward declarations
void ProcessSegmentInterleaved(std::span<float> data, Eigen::MatrixXf &channelData, const int NUM_CHAN);

using TimePoint = std::chrono::system_clock::time_point;


struct ExperimentConfig
{
    // UDP packet information
    static constexpr int HEAD_SIZE = 12;          // packet head size (bytes)
    static constexpr int NUM_CHAN = 4;            // number of channels per packet
    static constexpr int SAMPS_PER_CHANNEL = 124; // samples per packet per channel
    static constexpr int BYTES_PER_SAMP = 2;      // bytes per sample
    static constexpr int MICRO_INCR = 1240;       // time between packets
    static constexpr int SAMPLE_RATE = 1e5;       // sample rate in Hz

    static constexpr int DATA_SIZE = SAMPS_PER_CHANNEL * NUM_CHAN * BYTES_PER_SAMP; // packet data size (bytes)
    static constexpr int PACKET_SIZE = DATA_SIZE + HEAD_SIZE;                       // packet size (bytes)
    static constexpr int REQUIRED_BYTES = DATA_SIZE + HEAD_SIZE;
    static constexpr int DATA_BYTES_PER_CHANNEL = SAMPS_PER_CHANNEL * BYTES_PER_SAMP; // number of data bytes per channel (REQUIRED_BYTES - 12) / 4 channels

    static constexpr float TIME_WINDOW = 0.01; // fraction of a second to consider when performing cross correlation
    static constexpr int NUM_PACKS_DETECT = static_cast<int>(TIME_WINDOW * 100000 / SAMPS_PER_CHANNEL);
    static constexpr int DATA_SEGMENT_LENGTH = NUM_PACKS_DETECT * SAMPS_PER_CHANNEL * NUM_CHAN;
    static constexpr int interp = 1;

    const std::function<void(std::span<float>, Eigen::MatrixXf &, unsigned int)> ProcessFncPtr = ProcessSegmentInterleaved;
};

class ExperimentRuntime
{
public:
    std::string detectionOutputFile = "";
    bool trackerUse = false;
    std::chrono::seconds programRuntime;
    TimePoint programStartTime;
    float energyDetThresh = 100.0f; // 28.0f;
    float ampDetThresh = 100.0f;    // 28.0f;
    float speedOfSound = 1482.965459;

    std::string filterWeights = "filters/highpass_taps@101_cutoff@20k_window@hamming_fs@100k.txt";
    std::string receiverPositions = "../Data/SOCAL_H_72_HS_harp4chPar_recPos.txt";
    std::string onnxModelPath = "";
    std::string onnxModelScaling = "";

    fftwf_plan forwardFFT = nullptr;
    fftwf_plan inverseFFT = nullptr;

    // std::unique_ptr<ONNXModel> onnxModel = nullptr;

    std::unique_ptr<Tracker> tracker = nullptr;
    int onnxModel = 0;

    // Constructor to initialize plans, runtime-specific methods here
    ExperimentRuntime()
    {
        // Initialize FFT plans dynamically, if needed
    }

    ~ExperimentRuntime()
    {
        if (forwardFFT)
            fftwf_destroy_plan(forwardFFT);
        if (inverseFFT)
            fftwf_destroy_plan(inverseFFT);
    }
};
