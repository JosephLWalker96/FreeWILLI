#pragma once
#include "../pch.h"
#include "madgwick_marg.h"

/**
 * @class ImuProcessor
 * @brief A class for processing IMU data.
 *
 * This class provides methods to extract, calibrate, and process IMU data from raw byte streams,
 * and compute the rotation matrix based on accelerometer and magnetometer readings.
 */
class ImuProcessor
{
   public:
    explicit ImuProcessor(int IMU_BYTE_SIZE);

    void setRotationMatrix(const std::vector<uint8_t>& dataBytes);

    Eigen::Matrix3f mRotationMatrix;
    MadgwickMARG margFilter;

   private:
    // Calibration constants
    int mImuByteSize;

    static constexpr int mDataWidth = 3;
    static constexpr int mMagnetometerDataIndex = 11;
    static constexpr int mGyroscopeDataIndex = 14;
    static constexpr int mAccelerometerDataIndex = 17;

    const float mAccelerometerCalibration = 2.0f / 32768.0f;
    const float mGyroscopeCalibration = 2000.0f / 32768.0f;
    const Eigen::Vector3f mMagnetometerCalibration{1150.0f / 32768.0f, 1150.0f / 32768.0f, 2250.0f / 32768.0f};

    std::optional<Eigen::VectorXf> getImuDataFromBytes(const std::vector<uint8_t>& byteBlock);

    void calibrateImuData(Eigen::VectorXf& imuData);

    Eigen::Matrix3f calculateRotationMatrix(const Eigen::Vector3f& accelerometerData,
                                            const Eigen::Vector3f& magnetometerData);

    void madgwickUpdate(const Eigen::Vector3f& acc, const Eigen::Vector3f& mag, const Eigen::Vector3f& gyr, float beta,
                        float dt);

    Eigen::Quaternionf magFilterState;
};
