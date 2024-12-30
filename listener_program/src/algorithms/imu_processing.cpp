#include "imu_processing.h"

ImuProcessor::ImuProcessor(int IMU_BYTE_SIZE)
    : mImuByteSize(IMU_BYTE_SIZE), mRotationMatrix(Eigen::Matrix3f::Identity())
{
}

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
void ImuProcessor::setRotationMatrix(const std::vector<uint8_t>& dataBytes)
{
    auto dataVector = getImuDataFromBytes(dataBytes, mImuByteSize);

    if (!dataVector)  // if no IMU data is available
    {
        return;
    }

    // Dereference dataVector to access the Eigen::VectorXf
    Eigen::VectorXf& imuData = *dataVector;

    // Calibrate IMU data
    calibrateImuData(imuData);

    // Extract magnetometer data from indices
    Eigen::Vector3f magnetometerData = imuData.segment<mDataWidth>(mMagnetometerDataIndex);

    // Extract gyroscope data from indices
    Eigen::Vector3f gyroscopeData = imuData.segment<mDataWidth>(mGyroscopeDataIndex);

    // Extract accelerometer data from indices
    Eigen::Vector3f accelerometerData = imuData.segment<mDataWidth>(mAccelerometerDataIndex);

    // Calculate rotation matrix using accelerometer and magnetometer data
    mRotationMatrix = calculateRotationMatrix(accelerometerData, magnetometerData);

    madgwickUpdate(accelerometerData, magnetometerData, gyroscopeData, 0.0, 0.04f);
    std::cout << "Mag Filter: " << std::endl;
    std::cout << magFilterState.toRotationMatrix() << std::endl;
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
 * @return std::optional<Eigen::VectorXf> Parsed IMU data in a 20-element vector, or std::nullopt if the header is
 * invalid.
 */
std::optional<Eigen::VectorXf> ImuProcessor::getImuDataFromBytes(const std::vector<uint8_t>& byteBlock, int imuByteSize)
{
    Eigen::VectorXf imuData(20);  // Reserve space for 20 elements
    std::vector<uint8_t> block(byteBlock.end() - imuByteSize, byteBlock.end());

    // Check for valid header ('I' and 'M')
    if (block[0] == 'I' && block[1] == 'M')
    {
        uint16_t frameSync = *reinterpret_cast<const uint16_t*>(&block[2]);
        const uint8_t* timestamp = &block[4];
        uint16_t milliseconds = *reinterpret_cast<const uint16_t*>(&block[10]);
        uint16_t counter = *reinterpret_cast<const uint16_t*>(&block[12]);
        const int16_t* magnetometerData = reinterpret_cast<const int16_t*>(&block[14]);
        const int16_t* gyroscopeData = reinterpret_cast<const int16_t*>(&block[20]);
        const int16_t* accelerometerData = reinterpret_cast<const int16_t*>(&block[26]);

        // Populate the data vector
        imuData(0) = 'I';
        imuData(1) = 'M';
        imuData(2) = frameSync;

        for (int i = 0; i < 6; ++i) imuData(3 + i) = timestamp[i];

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

    return std::nullopt;  // Return an empty optional if header is invalid
}

/**
 * @brief Calibrates IMU data using predefined calibration constants.
 *
 * @param imuData A vector containing raw IMU data.
 *        - Indices 11-13: Magnetometer readings (to be calibrated element-wise).
 *        - Indices 14-16: Gyroscope readings (to be calibrated with a scalar).
 *        - Indices 17-19: Accelerometer readings (to be calibrated with a scalar).
 */
void ImuProcessor::calibrateImuData(Eigen::VectorXf& imuData)
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
Eigen::Matrix3f ImuProcessor::calculateRotationMatrix(const Eigen::Vector3f& accelerometerData,
                                                      const Eigen::Vector3f& magnetometerData)
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
    rotationMatrix.col(0) = northVector;       // Y-axis (North)
    rotationMatrix.col(1) = eastVector;        // X-axis (East)
    rotationMatrix.col(2) = gravityDirection;  // Z-axis (Down)

    return rotationMatrix;
}

/**
 * @brief Madgwick filter update function (9DoF: gyro + accel + mag).
 *
 * This version fuses gyroscope, accelerometer, and magnetometer data
 * to update the orientation quaternion @p q based on the algorithm
 * described by S. Madgwick in:
 *     "An efficient orientation filter for inertial and inertial/magnetic
 *      sensor arrays" (2010).
 *
 * @param[in,out] q   On input, the current orientation estimate (w, x, y, z).
 *                    On output, updated orientation after fusing sensor data.
 * @param gx,gy,gz    Gyroscope measurements in rad/s (body frame).
 * @param ax,ay,az    Accelerometer measurements in any linear scale (body frame).
 * @param mx,my,mz    Magnetometer measurements in any linear scale (body frame).
 * @param beta        Algorithm gain (commonly 0.01 - 0.3 depending on use case).
 * @param dt          Time step in seconds since last update.
 */
void ImuProcessor::madgwickUpdate(const Eigen::Vector3f& acc, const Eigen::Vector3f& mag, const Eigen::Vector3f& gyr,
                                  float beta, float dt)
{
    // Extract the quaternion components for convenience
    float q1 = magFilterState.w();
    float q2 = magFilterState.x();
    float q3 = magFilterState.y();
    float q4 = magFilterState.z();

    float gx = gyr[0];
    float gy = gyr[1];
    float gz = gyr[2];
    float ax = acc[0];
    float ay = acc[1];
    float az = acc[2];
    float mx = mag[0];
    float my = mag[1];
    float mz = mag[2];

    // Local helper variables
    float _2q1 = 2.0f * q1;
    float _2q2 = 2.0f * q2;
    float _2q3 = 2.0f * q3;
    float _2q4 = 2.0f * q4;
    float _4q1 = 4.0f * q1;
    float _4q2 = 4.0f * q2;
    float _4q3 = 4.0f * q3;
    float _8q2 = 8.0f * q2;
    float _8q3 = 8.0f * q3;
    float q1q1 = q1 * q1;
    float q2q2 = q2 * q2;
    float q3q3 = q3 * q3;
    float q4q4 = q4 * q4;

    float q1q2 = q1 * q2;
    float q1q3 = q1 * q3;
    float q1q4 = q1 * q4;
    float q2q3 = q2 * q3;
    float q2q4 = q2 * q4;
    float q3q4 = q3 * q4;

    // Normalise accelerometer measurement
    float normAcc = std::sqrt(ax * ax + ay * ay + az * az);
    if (normAcc > 0.0f)
    {
        ax /= normAcc;
        ay /= normAcc;
        az /= normAcc;
    }

    // Normalise magnetometer measurement
    float normMag = std::sqrt(mx * mx + my * my + mz * mz);
    if (normMag > 0.0f)
    {
        mx /= normMag;
        my /= normMag;
        mz /= normMag;
    }

    // Reference direction of Earth's magnetic field
    float _2q1mx = 2.0f * q1 * mx;
    float _2q1my = 2.0f * q1 * my;
    float _2q1mz = 2.0f * q1 * mz;
    float _2q2mx = 2.0f * q2 * mx;

    // Compute the reference direction of the Earth's magnetic field
    float hx =
        mx * q1q1 - _2q1my * q4 + _2q1mz * q3 + mx * q2q2 + _2q2 * my * q3 + _2q2 * mz * q4 - mx * q3q3 - mx * q4q4;
    float hy = _2q1mx * q4 + my * q1q1 - _2q1mz * q2 + _2q2mx * q3 - my * q2q2 + my * q3q3 + my * q4q4 - _2q4 * mz * q3;
    float _2bx = std::sqrt(hx * hx + hy * hy);
    float _2bz = -_2q1mx * q3 + _2q1my * q2 + mz * q1q1 + _2q2mx * q4 - mz * q2q2 + mz * q3q3 + mz * q4q4;
    float _4bx = 2.0f * _2bx;
    float _4bz = 2.0f * _2bz;

    // Gradient descent algorithm corrective step
    float s1 = -_2q3 * (2.0f * q2q2 + 2.0f * q3q3 - 1.0f - ax) + _2q2 * (2.0f * q1q1 + 2.0f * q2q2 - 1.0f - ay) -
               _2bz * q3 * (_2bx * (0.5f - q3q3 - q4q4) + _2bz * (q2q4 - q1q3) - mx) +
               (-_2bx * q4 + _2bz * q2) * (_2bx * (q2q3 - q1q4) + _2bz * (q1q2 + q3q4) - my) +
               _2bx * q3 * (_2bx * (q1q3 + q2q4) + _2bz * (0.5f - q2q2 - q3q3) - mz);
    float s2 = _2q4 * (2.0f * q2q2 + 2.0f * q3q3 - 1.0f - ax) + _2q1 * (2.0f * q1q1 + 2.0f * q2q2 - 1.0f - ay) -
               4.0f * q2 * (1.0f - 2.0f * q2q2 - 2.0f * q3q3 - az) +
               _2bz * q4 * (_2bx * (0.5f - q3q3 - q4q4) + _2bz * (q2q4 - q1q3) - mx) +
               (_2bx * q3 + _2bz * q1) * (_2bx * (q2q3 - q1q4) + _2bz * (q1q2 + q3q4) - my) +
               (_2bx * q4 - _4bz * q2) * (_2bx * (q1q3 + q2q4) + _2bz * (0.5f - q2q2 - q3q3) - mz);
    float s3 = -_2q1 * (2.0f * q2q2 + 2.0f * q3q3 - 1.0f - ax) + _2q4 * (2.0f * q1q1 + 2.0f * q2q2 - 1.0f - ay) -
               4.0f * q3 * (1.0f - 2.0f * q2q2 - 2.0f * q3q3 - az) +
               (-_4bx * q3 - _2bz * q1) * (_2bx * (0.5f - q3q3 - q4q4) + _2bz * (q2q4 - q1q3) - mx) +
               (_2bx * q2 + _2bz * q4) * (_2bx * (q2q3 - q1q4) + _2bz * (q1q2 + q3q4) - my) +
               (_2bx * q1 - _4bz * q3) * (_2bx * (q1q3 + q2q4) + _2bz * (0.5f - q2q2 - q3q3) - mz);
    float s4 = _2q2 * (2.0f * q1q1 + 2.0f * q2q2 - 1.0f - ay) - _2q3 * (2.0f * q1q1 + 2.0f * q2q2 - 1.0f - ax) +
               (-_4bx * q4 + _2bz * q2) * (_2bx * (0.5f - q3q3 - q4q4) + _2bz * (q2q4 - q1q3) - mx) +
               (-_2bx * q1 + _2bz * q3) * (_2bx * (q2q3 - q1q4) + _2bz * (q1q2 + q3q4) - my) +
               (_2bx * q2) * (_2bx * (q1q3 + q2q4) + _2bz * (0.5f - q2q2 - q3q3) - mz);

    // Normalise step magnitude
    float normS = std::sqrt(s1 * s1 + s2 * s2 + s3 * s3 + s4 * s4);
    if (normS > 1.0e-9f)
    {
        s1 /= normS;
        s2 /= normS;
        s3 /= normS;
        s4 /= normS;
    }

    // Compute rate of change of quaternion
    float qDot1 = 0.5f * (-q2 * gx - q3 * gy - q4 * gz) - beta * s1;
    float qDot2 = 0.5f * (q1 * gx + q3 * gz - q4 * gy) - beta * s2;
    float qDot3 = 0.5f * (q1 * gy - q2 * gz + q4 * gx) - beta * s3;
    float qDot4 = 0.5f * (q1 * gz + q2 * gy - q3 * gx) - beta * s4;

    // Integrate to yield quaternion
    q1 += qDot1 * dt;
    q2 += qDot2 * dt;
    q3 += qDot3 * dt;
    q4 += qDot4 * dt;

    // Normalise quaternion
    float normQ = std::sqrt(q1 * q1 + q2 * q2 + q3 * q3 + q4 * q4);
    if (normQ > 1.0e-9f)
    {
        q1 /= normQ;
        q2 /= normQ;
        q3 /= normQ;
        q4 /= normQ;
    } else
    {
        // Fallback: if norm is tiny, reset to identity or skip
        q1 = 1.0f;
        q2 = 0.0f;
        q3 = 0.0f;
        q4 = 0.0f;
    }

    // Write back to the Eigen quaternion
    magFilterState.w() = q1;
    magFilterState.x() = q2;
    magFilterState.y() = q3;
    magFilterState.z() = q4;
}