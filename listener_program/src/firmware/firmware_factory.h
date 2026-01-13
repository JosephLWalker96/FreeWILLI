#pragma once

#include "firmware_1240.h"
#include "firmware_1240_imu.h"
#include "firmware_1240_fs2x.h"
#include "firmware_1240_fs2x_imu.h"

class FirmwareFactory
{
   public:
    static const std::shared_ptr<const IFirmware> create(const std::string& firmwareToUse)
    {
        if (firmwareToUse == "1240_imu")
        {
            return std::make_shared<const Firmware1240IMU>();
        }
        else if (firmwareToUse == "1240")
        {
            return std::make_shared<const Firmware1240>();
        }
        else if (firmwareToUse == "1240_fs2x")
        {
            return std::make_unique<const Firmware1240Fs2x>();
        }
        else if (firmwareToUse == "1240_fs2x_imu")
        {
            return std::make_unique<const Firmware1240Fs2xIMU>();
        }
        else
        {
            std::cerr << "Specified firmware version not recognized \n";
            std::exit(1);
        }
    }
};