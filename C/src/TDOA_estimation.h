/*

This file contains all function prototypes for TDOA_estimation.cpp

*/

#pragma once

#include "custom_types.h"

Eigen::VectorXf GCC_PHAT_FFTW(Eigen::MatrixXcf& savedFFTs, fftwf_plan& ip1, const int& interp, int& paddedLength, unsigned int& NUM_CHAN, const unsigned int& SAMPLE_RATE);
Eigen::VectorXf DOA_EstimateVerticalArray(Eigen::VectorXf& TDOAs, const double& soundSpeed, std::span<float> chanSpacing);
