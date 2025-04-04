#pragma once

#include "firmware_1240_fs2x.h"
#include "imu_processor_1240.h"

/**
 * @brief  Class for firmware 1240 configuration with 200kHz samp rate and IMU data, providing constants and utility methods.
 */
class Firmware1240Fs2xIMU : public Firmware1240Fs2x
{
   public:
    Firmware1240Fs2xIMU() : mImuByteSize(32) { imuManager = std::make_unique<ImuProcessor1240>(mImuByteSize); }

    int imuByteSize() const override { return mImuByteSize; }

   private:
    int mImuByteSize;
};