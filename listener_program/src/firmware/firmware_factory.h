#pragma once

#include "firmware_1240.h"
#include "firmware_1240_imu.h"
#include "firmware_1240_fs2x.h"
#include "firmware_1240_fs2x_imu.h"

class FirmwareFactory
{
   public:
    static std::unique_ptr<const Firmware1240> create(const std::string& firmware)
    {
        if (firmware == "FW1240")
        {
            return std::make_unique<const Firmware1240>();
        }
        else if (firmware == "FW1240-IMU")
        {
            return std::make_unique<const Firmware1240IMU>();
        }
        else if (firmware == "FW1240-Fs2x")
        {
            return std::make_unique<const Firmware1240Fs2x>();
        }
        else if (firmware = "FW1240-Fs2x-IMU")
        {
            return std::make_unique<const Firmware1240Fs2xIMU>();
        }
        else
        {
            std::throw // check on this :)
        }
    }
};