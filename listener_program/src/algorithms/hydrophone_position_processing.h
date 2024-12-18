#pragma once
#include "../pch.h"

Eigen::MatrixXf loadHydrophonePositionsFromFile(const std::string &filename);

Eigen::MatrixXf calculateRelativePositions(const Eigen::MatrixXf &positions);

Eigen::MatrixXf getHydrophoneRelativePositions(const std::string &filename);

void hydrophoneMatrixDecomposition(const Eigen::MatrixXf hydrophonePositions, Eigen::MatrixXf &precomputedP,
                                   Eigen::MatrixXf &basisMatrixU, int &rankOfHydrophoneMatrix);