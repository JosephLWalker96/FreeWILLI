#pragma once

#include "../pch.h"
#include "firmware_1240.h"

/**
 * @brief Class for firmware 1240 configuration with 200-kHz sample rate, providing constant utility and methods.
 */
class Firmware1240Fs2x : public Firmware1240
{
    public:
     int sampleRate() const override { return SAMPLE_RATE; }
      
     int numPacketsToDetect() const override { return static_cast<int>(TIME_WINDOW * SAMPLE_RATE / SAMPS_PER_CHANNEL); }
 
     int channelSize() const override { return numPacketsToDetect() * SAMPS_PER_CHANNEL; }
 
     int packetSize() const override { return DATA_SIZE + HEAD_SIZE + imuByteSize(); }
    
    private:
     const int NUM_CHAN = 4;
     const int SAMPLE_RATE = 2e5; // DOUBLED FOR FS2XSS
     const int MICRO_INCR = 1240;  // microseconds between data packets
     const int SAMPS_PER_CHANNEL = 248;  // Samples per packet per channel - DOUBLED FOR FS2X
     const float TIME_WINDOW = 0.01;  // Fraction of a second for cross-correlation
     const int HEAD_SIZE = 12;  // Packet head size (bytes)
     const int BYTES_PER_SAMP = 2;  // Bytes per sample
     const float SAMPLE_OFFSET = 32768.0f;  // value used to convert unsigned 16 bit to signed
 
     const int SAMPS_PER_PACKET = SAMPS_PER_CHANNEL * NUM_CHAN;  // Samples per packet
     const int DATA_SIZE = SAMPS_PER_PACKET * BYTES_PER_SAMP;  // Packet data size (bytes)
     
};