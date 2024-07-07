/*

This file contains all function prototypes for TDOA_estimation.cpp

*/

#pragma once

// #include "gtest/gtest.h"
#include <eigen3/Eigen/Dense>
#include <fftw3.h>
#include <liquid/liquid.h>

//void FilterWithFIR(arma::Col<double>& ch1, arma::Col<double>& ch2, arma::Col<double>& ch3, arma::Col<double>& ch4, sp::FIR_filt<double, double, double>& fir_filt);
//void FilterWithIIR(arma::Col<double>& ch1, arma::Col<double>& ch2, arma::Col<double>& ch3, arma::Col<double>& ch4, sp::IIR_filt<double, double, double>& iir_filt);
void FilterWithLiquidFIR(Eigen::VectorXd& ch1, Eigen::VectorXd& ch2, Eigen::VectorXd& ch3, Eigen::VectorXd& ch4, 
                         firfilt_rrrf& q1, firfilt_rrrf& q2, firfilt_rrrf& q3, firfilt_rrrf& q4);
