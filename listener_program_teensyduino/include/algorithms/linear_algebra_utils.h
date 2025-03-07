#pragma once

#include "Eigen/Dense"

Eigen::JacobiSVD<Eigen::MatrixXf> computeSvd(const Eigen::MatrixXf &H);
Eigen::MatrixXf precomputePseudoInverse(const Eigen::JacobiSVD<Eigen::MatrixXf> &svd);
int computeRank(const Eigen::MatrixXf &H, double tolerance);
