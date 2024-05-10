/*

This file contains all function prototypes for TDOA_estimation.cpp

*/

#ifndef TDOA_ESTIMATION
#define TDOA_ESTIMATION

#include <armadillo>
#include <eigen3/Eigen/Dense>
#include <sigpack.h>
#include <fftw/fftw.h>
//#include <Eigen/Dense>
arma::Mat<double> GCC_PHAT(arma::Mat<double>& data, int interp, sp::FFTW& fftw, int fftLength);
Eigen::MatrixXd GCC_PHAT_Eigen(Eigen::MatrixXd& data, int interp);
//void filterWithFIR(arma::Col<double>& ch1, arma::Col<double>& ch2, arma::Col<double>& ch3, arma::Col<double>& ch4, sp::FIR_filt<double, double, double>& fir_filt);

#endif
