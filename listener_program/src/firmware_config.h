#pragma once

#include "pch.h"

//using TimePoint = std::chrono::system_clock::time_point;

struct FirmwareConfig
{
    // UDP packet information
    static constexpr int HEAD_SIZE = 12;          // packet head size (bytes)
    static constexpr int NUM_CHAN = 4;            // number of channels per packet
    static constexpr int SAMPS_PER_CHANNEL = 124; // samples per packet per channel
    static constexpr int BYTES_PER_SAMP = 2;      // bytes per sample
    static constexpr int MICRO_INCR = 1240;       // time between packets
    static constexpr int SAMPLE_RATE = 1e5;       // sample rate in Hz
    
    static constexpr float SAMPLE_OFFSET = 32768.0f; // Define a named constant for the sample offset

    static constexpr int DATA_SIZE = SAMPS_PER_CHANNEL * NUM_CHAN * BYTES_PER_SAMP; // packet data size (bytes)
    static constexpr int REQUIRED_BYTES = DATA_SIZE + HEAD_SIZE;
    static constexpr int DATA_BYTES_PER_CHANNEL = SAMPS_PER_CHANNEL * BYTES_PER_SAMP; // number of data bytes per channel (REQUIRED_BYTES - 12) / 4 channels

    static constexpr float TIME_WINDOW = 0.01; // fraction of a second to consider when performing cross correlation
    static constexpr int NUM_PACKS_DETECT = static_cast<int>(TIME_WINDOW * SAMPLE_RATE / SAMPS_PER_CHANNEL);
    static constexpr int DATA_SEGMENT_LENGTH = NUM_PACKS_DETECT * SAMPS_PER_CHANNEL * NUM_CHAN;

    // the number of samples per channel within a dataSegment
    static constexpr int CHANNEL_SIZE = DATA_SEGMENT_LENGTH / NUM_CHAN;

    static constexpr int IMU_BYTE_SIZE = 0;

    static constexpr int PACKET_SIZE = DATA_SIZE + HEAD_SIZE + IMU_BYTE_SIZE; // the total number of bytes in a UDP packet

    /*
    * counter: how many times we’ve already called this function 
    */
    void convertAndInsertData(Eigen::MatrixXf &channelMatrix, 
                        std::span<uint8_t> dataBytes, 
                        int &counter,
                        int headSize = HEAD_SIZE,
                        int dataSize = DATA_SIZE,
                        int bytesPerSamp = BYTES_PER_SAMP,
                        float sampleOffset = SAMPLE_OFFSET)
    {
        // Each sample = 2 bytes
        const size_t numSamples = dataSize / bytesPerSamp;
        const size_t numRows = channelMatrix.rows();

        // Calculate how many columns the new data will occupy
        const size_t newCols = (numSamples + numRows - 1) / numRows;

        // Pointer to the start of the Eigen matrix's data (column-major layout)
        float* __restrict__ matrixPtr = channelMatrix.data();

        // Pointer to incoming samples (skipping the header)
        const uint8_t* __restrict__ inPtr = dataBytes.data() + headSize;

        // Compute the offset where the new columns begin based on the counter
        const size_t startOffset = counter * newCols * numRows;

        // Single-pass conversion & storage
        for (size_t i = 0; i < numSamples; ++i)
        {
            // Convert 2-byte sample to float. Bytes are in big-endian order in the buffer (high byte first)
            uint16_t sample = (static_cast<uint16_t>(inPtr[bytesPerSamp * i]) << 8) | inPtr[bytesPerSamp * i + 1];
            float value = static_cast<float>(sample) - sampleOffset;

            matrixPtr[startOffset + i] = value;
        }
    }
};
