/*

This file contains all function prototypes for TDOA_estimation.cpp

*/

#pragma once
#include <armadillo>
#include <fftw3.h>

arma::Col<double> GCC_PHAT_FFTW(arma::Mat<arma::cx_double>& savedFFTs_FFTW, fftw_plan& ip1, const int& interp, int& fftLength, unsigned int& NUM_CHAN, const unsigned int& SAMPLE_RATE);
arma::Col<double> DOA_EstimateVerticalArray(arma::Col<double>& TDOAs, const double& soundSpeed, arma::Col<int>& chanSpacing);
