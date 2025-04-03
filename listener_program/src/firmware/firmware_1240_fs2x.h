#pragma once

#include "firmware_1240.h"

/**
 * @brief Class for firmware 1240 Fs2x configuration, providing constants and utility methods.
 */
class Firmware1240Fs2x : public Firmware1240
{
   public:
    // UDP packet information
    static constexpr int sampleRate() { return 2e5; }
};
