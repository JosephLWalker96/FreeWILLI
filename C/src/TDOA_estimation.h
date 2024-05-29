/*

This file contains all function prototypes for TDOA_estimation.cpp

*/

#ifndef TDOA_ESTIMATION
#define TDOA_ESTIMATION

#include <armadillo>
#include <eigen3/Eigen/Dense>
#include <sigpack.h>
#include <fftw/fftw.h>

arma::Col<double> GCC_PHAT(arma::Mat<arma::cx_double>& savedFFTs, int& interp, sp::FFTW& fftw, int& fftLength, unsigned int& NUM_CHAN, const unsigned int& SAMPLE_RATE);
Eigen::MatrixXd GCC_PHAT_Eigen(Eigen::MatrixXd& data, int& interp);
arma::Col<double> DOA_EstimateVerticalArray(arma::Col<double>& TDOAs, const double& soundSpeed, arma::Col<int>& chanSpacing);

#endif
