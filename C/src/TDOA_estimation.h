/*

This file contains all function prototypes for TDOA_estimation.cpp

*/

#pragma once

#include "custom_types.h"

auto GCC_PHAT(Eigen::MatrixXcf &savedFFTs, fftwf_plan &inverseFFT,
                   const int interp, int &paddedLength,
                   const int &NUM_CHAN, const int &SAMPLE_RATE)
    -> std::tuple<Eigen::VectorXf, Eigen::VectorXf>;
Eigen::VectorXf TDOA_To_DOA_VerticalArray(Eigen::VectorXf &TDOAs, const float &soundSpeed, 
                std::span<float> chanSpacing);
Eigen::VectorXf TDOA_To_DOA_GeneralArray(const Eigen::ColPivHouseholderQR<Eigen::MatrixXd> &qr, const float c, const Eigen::VectorXf &tdoa);
//Eigen::VectorXf TDOA_To_DOA_GeneralArray(const Eigen::MatrixXd& pseudoInv, const float c, const Eigen::VectorXf &tdoa);
Eigen::ColPivHouseholderQR<Eigen::MatrixXd> precomputedQR(const Eigen::MatrixXd &H);
Eigen::MatrixXd precomputedPseudoInverse(const Eigen::MatrixXd &H);
Eigen::MatrixXd computePseudoInverse(const Eigen::MatrixXd &H);
Eigen::JacobiSVD<Eigen::MatrixXd> SVD(const Eigen::MatrixXd &H);

Eigen::MatrixXd precomputeInverse(const Eigen::JacobiSVD<Eigen::MatrixXd> &svd);
Eigen::VectorXf TDOA_To_DOA_SVD(const Eigen::JacobiSVD<Eigen::MatrixXd> &svd, const float speedOfSound, const Eigen::VectorXf &tdoa);
Eigen::VectorXf TDOA_To_DOA_Optimized(const Eigen::MatrixXd &P, const Eigen::MatrixXd &U, const float speedOfSound, const Eigen::VectorXf &tdoa, int rank);
Eigen::VectorXf DOA_to_ElAz(Eigen::VectorXf& doa);
void normalizeDOA(Eigen::VectorXd &doa, int rank);
int GetRank(const Eigen::MatrixXd &H, double tolerance = 1e-6);
