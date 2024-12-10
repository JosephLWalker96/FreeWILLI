#pragma once

#include <eigen3/Eigen/Dense>

Eigen::JacobiSVD<Eigen::MatrixXf> computeSvd(const Eigen::MatrixXf &H);
Eigen::MatrixXf precomputePseudoInverse(const Eigen::JacobiSVD<Eigen::MatrixXf> &svd);
int computeRank(const Eigen::MatrixXf &H, double tolerance = 1e-6);
