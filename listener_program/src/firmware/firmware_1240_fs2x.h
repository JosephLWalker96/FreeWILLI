#pragma once

#include "../pch.h"
#include "firmware_1240.h"

/**
 * @brief Class for firmware 1240 configuration with 200-kHz sample rate, providing constant utility and methods.
 */
class Firmware1240Fs2x : public Firmware1240
{
    public:
     int sampleRate() const override { return mSampleRate; }
    
    private:
     static constexpr int mSampleRate = 200000;
};