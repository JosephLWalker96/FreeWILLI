#pragma once

#include "firmware_1240_fs2x.h"
#include "imu_processor_1240.h"

/**
 * @brief  Class for firmware 1240 configuration with 200-kHz sample rate & IMU data, providing constants and utility methods.
 */
class Firmware1240Fs2xIMU : public Firmware1240Fs2x
{
   public:
    int imuByteSize() const override { return mImuByteSize; }

    IImuProcessor* getImuManager() const override { return imuManager.get(); }

   private:
    static constexpr int mImuByteSize = 32;
    std::unique_ptr<IImuProcessor> imuManager = std::make_unique<ImuProcessor1240>(mImuByteSize);
};
