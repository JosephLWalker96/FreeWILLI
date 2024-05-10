/*

This file contains all function prototypes for TDOA_estimation.cpp

*/

#ifndef FILTERS
#define FILTERS

#include <armadillo>
#include <eigen3/Eigen/Dense>
#include <sigpack.h>
#include <fftw/fftw.h>
//#include <Eigen/Dense>
void filterWithFIR(arma::Col<double>& ch1, arma::Col<double>& ch2, arma::Col<double>& ch3, arma::Col<double>& ch4, sp::FIR_filt<double, double, double>& fir_filt);
void filterWithIIR(arma::Col<double>& ch1, arma::Col<double>& ch2, arma::Col<double>& ch3, arma::Col<double>& ch4, sp::IIR_filt<double, double, double>& iir_filt);
void filterWithLiquidFIR(arma::Col<double>& ch1, arma::Col<double>& ch2, arma::Col<double>& ch3, arma::Col<double>& ch4, sp::FIR_filt<double, double, double>& fir_filt);

#endif
