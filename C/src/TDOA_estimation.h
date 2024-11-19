/*

This file contains all function prototypes for TDOA_estimation.cpp

*/

#pragma once

//#include "custom_types.h"
#include "pch.h"

class GCC_Value_Error : public std::runtime_error
{
public:
    GCC_Value_Error(const std::string &message) : std::runtime_error(message) {}
};

auto GCC_PHAT(const Eigen::MatrixXcf &savedFFTs, fftwf_plan &inverseFFT,
                   const int interp, int &paddedLength,
                   const int &NUM_CHAN, const int &SAMPLE_RATE)
    -> std::tuple<Eigen::VectorXf, Eigen::VectorXf>;
Eigen::JacobiSVD<Eigen::MatrixXf> SVD(const Eigen::MatrixXf &H);

Eigen::MatrixXf precomputeInverse(const Eigen::JacobiSVD<Eigen::MatrixXf> &svd);
Eigen::VectorXf TDOA_To_DOA_Optimized(const Eigen::MatrixXf &P, const Eigen::MatrixXf &U, const float speedOfSound, const Eigen::VectorXf &tdoa, int rank);
Eigen::VectorXf DOA_to_ElAz(Eigen::VectorXf& doa);
void normalizeDOA(Eigen::VectorXf &doa, int rank);
int GetRank(const Eigen::MatrixXf &H, double tolerance = 1e-6);
