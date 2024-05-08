/*

This file contains all function prototypes for TDOA_estimation.cpp

*/

#ifndef TDOA_ESTIMATION
#define TDOA_ESTIMATION

#include <armadillo>
#include <eigen3/Eigen/Dense>
//#include <Eigen/Dense>
arma::Mat<double> GCC_PHAT(arma::Mat<double>& data, int interp);
Eigen::MatrixXd GCC_PHAT_Eigen(Eigen::MatrixXd& data, int interp);

#endif
