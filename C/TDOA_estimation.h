/*

This file contains all function prototypes for TDOA_estimation.cpp

*/

#ifndef TDOA_ESTIMATION
#define TDOA_ESTIMATION

#include <armadillo>
#include <eigen3/Eigen/Dense>
//#include <Eigen/Dense>
void GCC_PHAT(arma::Mat<double>& data, int interp);
void GCC_PHAT_Eigen(Eigen::MatrixXd& data, int interp);

#endif
