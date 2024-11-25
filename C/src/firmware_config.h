#pragma once

#include "pch.h"

// forward declarations
void processSegmentInterleaved(std::span<float> data, Eigen::MatrixXf &channelData, const int NUM_CHAN);

using TimePoint = std::chrono::system_clock::time_point;

struct FirmwareConfig
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
    
    // the number of samples per channel within a dataSegment
    static constexpr int CHANNEL_SIZE = DATA_SEGMENT_LENGTH / NUM_CHAN;

    const std::function<void(std::span<float>, Eigen::MatrixXf &, unsigned int)> ProcessFncPtr = processSegmentInterleaved;
};
