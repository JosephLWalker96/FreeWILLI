#pragma once
#include "../pch.h"

auto getImuDataFromBytes(const std::vector<uint8_t> &byteBlocks,
                         const int &IMU_BYTE_SIZE) -> std::optional<Eigen::VectorXf>;

void calibrateImuData(Eigen::VectorXf &imuData);

auto calculateRotationMatrix(const Eigen::Vector3f &acc, const Eigen::Vector3f &mag) -> Eigen::Matrix3f;

void setRotationMatrix(const std::vector<uint8_t> &dataBytes, const int &IMU_BYTE_SIZE, Eigen::Matrix3f &rotationMatrix);
