#include "IMU_processor.h"

ImuProcessor::ImuProcessor(int IMU_BYTE_SIZE) : mImuByteSize(IMU_BYTE_SIZE), mRotationMatrix(Eigen::Matrix3f::Identity()) {}

/**
 * @brief Updates the rotation matrix based on the latest IMU data bytes.
 *
 * This function reads raw IMU data from a vector of bytes, constructs a data vector,
 * calibrates the IMU sensor readings, and computes the rotation matrix using
 * accelerometer and magnetometer data.
 *
 * @param dataBytes     A vector of bytes containing the raw IMU data.
 * @param imuByteSize   The size in bytes of each IMU data packet.
 */
void ImuProcessor::setRotationMatrix(const std::vector<uint8_t> &dataBytes)
{
    auto dataVector = getImuDataFromBytes(dataBytes, mImuByteSize);

    if (!dataVector) // if no IMU data is available
    {
        return;
    }

    // Dereference dataVector to access the Eigen::VectorXf
    Eigen::VectorXf &imuData = *dataVector;

    // Calibrate IMU data
    calibrateImuData(imuData);

    // Extract magnetometer data from indices 11-13
    Eigen::Vector3f magnetometerData = imuData.segment<mDataWidth>(mMagnetometerDataIndex);

    // Extract gyroscope data from indices 14-16 (not used here)
    // Eigen::Vector3f gyroscopeData = imuData.segment<mDataWidth>(mGyroscopeDataIndex);

    // Extract accelerometer data from indices 17-19
    Eigen::Vector3f accelerometerData = imuData.segment<mDataWidth>(mAccelerometerDataIndex);

    // Calculate rotation matrix using accelerometer and magnetometer data
    mRotationMatrix = calculateRotationMatrix(accelerometerData, magnetometerData);
}

/**
 * @brief Extracts and parses IMU data from a byte block.
 *
 * This function reads a block of bytes, validates the header, and extracts IMU data including
 * frame synchronization, timestamp, magnetometer, gyroscope, and accelerometer readings.
 * If the header is invalid, it returns an empty optional.
 *
 * @param byteBlock A vector of bytes containing IMU data.
 * @param imuByteSize The expected size of the IMU data block in bytes.
 * @return std::optional<Eigen::VectorXf> Parsed IMU data in a 20-element vector, or std::nullopt if the header is invalid.
 */
std::optional<Eigen::VectorXf> ImuProcessor::getImuDataFromBytes(const std::vector<uint8_t> &byteBlock, int imuByteSize)
{
    Eigen::VectorXf imuData(20); // Reserve space for 20 elements
    std::vector<uint8_t> block(byteBlock.end() - imuByteSize, byteBlock.end());

    // Check for valid header ('I' and 'M')
    if (block[0] == 'I' && block[1] == 'M')
    {
        uint16_t frameSync = *reinterpret_cast<const uint16_t *>(&block[2]);
        const uint8_t *timestamp = &block[4];
        uint16_t milliseconds = *reinterpret_cast<const uint16_t *>(&block[10]);
        uint16_t counter = *reinterpret_cast<const uint16_t *>(&block[12]);
        const int16_t *magnetometerData = reinterpret_cast<const int16_t *>(&block[14]);
        const int16_t *gyroscopeData = reinterpret_cast<const int16_t *>(&block[20]);
        const int16_t *accelerometerData = reinterpret_cast<const int16_t *>(&block[26]);

        // Populate the data vector
        imuData(0) = 'I';
        imuData(1) = 'M';
        imuData(2) = frameSync;

        for (int i = 0; i < 6; ++i)
            imuData(3 + i) = timestamp[i];

        imuData(9) = milliseconds;
        imuData(10) = counter;

        for (int i = 0; i < 3; ++i)
        {
            imuData(11 + i) = magnetometerData[i];
            imuData(14 + i) = gyroscopeData[i];
            imuData(17 + i) = accelerometerData[i];
        }

        return imuData;
    }

    return std::nullopt; // Return an empty optional if header is invalid
}

/**
 * @brief Calibrates IMU data using predefined calibration constants.
 *
 * @param imuData A vector containing raw IMU data.
 *        - Indices 11-13: Magnetometer readings (to be calibrated element-wise).
 *        - Indices 14-16: Gyroscope readings (to be calibrated with a scalar).
 *        - Indices 17-19: Accelerometer readings (to be calibrated with a scalar).
 */
void ImuProcessor::calibrateImuData(Eigen::VectorXf &imuData)
{
    // Apply element-wise calibration to magnetometer data (indices 11 to 13)
    imuData.segment<3>(11) = imuData.segment<3>(11).cwiseProduct(mMagnetometerCalibration);

    // Apply scalar calibration to gyroscope data (indices 14 to 16)
    imuData.segment<3>(14) *= mGyroscopeCalibration;

    // Apply scalar calibration to accelerometer data (indices 17 to 19)
    imuData.segment<3>(17) *= mAccelerometerCalibration;
}

/**
 * @brief Calculates the rotation matrix from accelerometer and magnetometer readings.
 *
 * This function computes a rotation matrix using the normalized accelerometer and magnetometer
 * vectors. The accelerometer provides the gravity direction, and the magnetometer provides the
 * magnetic field direction. The resulting rotation matrix describes the orientation of the device.
 *
 * @param accelerometerData A 3D vector representing normalized accelerometer readings (gravity direction).
 * @param magnetometerData A 3D vector representing normalized magnetometer readings (magnetic field direction).
 * @return Eigen::Matrix3f The calculated rotation matrix, where:
 *         - Column 0 is the North vector (Y-axis),
 *         - Column 1 is the East vector (X-axis),
 *         - Column 2 is the Down vector (Z-axis).
 */
Eigen::Matrix3f ImuProcessor::calculateRotationMatrix(const Eigen::Vector3f &accelerometerData, const Eigen::Vector3f &magnetometerData)
{
    // Normalize accelerometer vector (gravity direction)
    Eigen::Vector3f gravityDirection = accelerometerData.normalized();

    // Normalize magnetometer vector (magnetic field direction)
    Eigen::Vector3f magneticFieldDirection = magnetometerData.normalized();

    // Compute the East vector (perpendicular to gravity and magnetic field)
    Eigen::Vector3f eastVector = (gravityDirection.cross(magneticFieldDirection)).normalized();

    // Compute the North vector (perpendicular to gravity and East vector)
    Eigen::Vector3f northVector = eastVector.cross(gravityDirection);

    // Construct the rotation matrix
    Eigen::Matrix3f rotationMatrix;
    rotationMatrix.col(0) = northVector;      // Y-axis (North)
    rotationMatrix.col(1) = eastVector;       // X-axis (East)
    rotationMatrix.col(2) = gravityDirection; // Z-axis (Down)

    return rotationMatrix;
}