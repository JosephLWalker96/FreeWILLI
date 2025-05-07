#pragma once

#include "../pch.h"
// #include <eigen3/Eigen/Dense>

auto computeSvd(const Eigen::MatrixXf& H) -> Eigen::JacobiSVD<Eigen::MatrixXf>;
std::tuple<Eigen::MatrixXf, int> precomputePseudoInverseAndRank(
    const Eigen::JacobiSVD<Eigen::MatrixXf>& svd, float speedOfSound);
