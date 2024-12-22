#pragma once
#include "../pch.h"

Eigen::MatrixXf loadHydrophonePositionsFromFile(const std::string &filename);

Eigen::MatrixXf calculateRelativePositions(const Eigen::MatrixXf &positions);

Eigen::MatrixXf getHydrophoneRelativePositions(const std::string &filename);

auto hydrophoneMatrixDecomposition(const Eigen::MatrixXf& hydrophonePositions) -> std::tuple<Eigen::MatrixXf, Eigen::MatrixXf, int>;