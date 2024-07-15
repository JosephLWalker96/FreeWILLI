/*

This file contains all function prototypes for TDOA_estimation.cpp

*/

#pragma once
#include <eigen3/Eigen/Dense>
#include <fftw3.h>

//arma::Col<double> GCC_PHAT_FFTW(arma::Mat<arma::cx_double>& savedFFTs_FFTW, fftw_plan& ip1, const int& interp, int& fftLength, unsigned int& NUM_CHAN, const unsigned int& SAMPLE_RATE);
Eigen::VectorXf GCC_PHAT_FFTW_E(Eigen::MatrixXcf& savedFFTs, fftwf_plan& ip1, const int& interp, int& paddedLength, unsigned int& NUM_CHAN, const unsigned int& SAMPLE_RATE);
Eigen::VectorXf DOA_EstimateVerticalArray(Eigen::VectorXf& TDOAs, const double& soundSpeed, std::vector<float>& chanSpacing);
