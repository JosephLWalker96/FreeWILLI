#include <vector>
#include <algorithm>
#include <numeric>
#include <cmath>
#include <cstdlib>
#include <fstream>
#include <chrono>
//#include <eigen3/Eigen/Dense>
#include <iostream>
#include <armadillo> //https://www.uio.no/studier/emner/matnat/fys/FYS4411/v13/guides/installing-armadillo/
#include <iomanip> // for output formatting
//#include <sigpack.h>

#include <eigen3/Eigen/Dense>
//#include <Eigen/Dense>
#include <eigen3/unsupported/Eigen/FFT>
//#include <unsupported/Eigen/FFT>
//#include <eigen3/Eigen/FFT>  // Include FFT module

#include "my_globals.h"
#include "TDOA_estimation.h"

using std::cout;
using std::endl;
using std::cerr;
using std::vector;
using std::string;
using TimePoint = std::chrono::system_clock::time_point;

arma::Mat<double> GCC_PHAT(arma::Mat<double>& data, int interp){
    
    arma::Mat<double> tau_matrix(NUM_CHAN, NUM_CHAN, arma::fill::zeros);

    for (int sig1_ind = 0; sig1_ind < (NUM_CHAN - 1); sig1_ind++ ){
        for (int sig2_ind = sig1_ind + 1; sig2_ind < NUM_CHAN; sig2_ind++) {
            //cout << "combo: " << sig1_ind << " " << sig2_ind << endl; 
            arma::vec sig1 = arma::abs(data.col(sig1_ind));
            arma::vec sig2 = arma::abs(data.col(sig2_ind));
            //cout << "sig1 len: " << sig1.n_elem << endl;

            int n = sig1.n_elem + sig2.n_elem;

            arma::cx_vec SIG1 = arma::fft(sig1,n);
            arma::cx_vec SIG2 = arma::fft(sig2,n);
            
            cout << "SIG1: "; 
            for (int i = 0; i < 20; i++){
                cout << SIG1(i) << " ";
            }
            cout << endl;

            arma::cx_vec R = SIG1 % arma::conj(SIG2);
            arma::cx_vec R_normed = R / arma::abs(R);

            arma::cx_vec CC_blah = arma::ifft(R_normed, interp * n);
            arma::vec CC = arma::abs(CC_blah);
            
            int max_shift = interp * n / 2;

            arma::vec sub1 = CC.subvec(CC.n_elem - max_shift, CC.n_elem- 1);
            arma::vec sub2 = CC.subvec(0, max_shift);
            
            arma::vec CCnew = arma::join_cols(sub1,sub2); // join_rows

            double shift = (double)arma::index_max(arma::abs(CCnew)) - max_shift;
            double tau = shift / (interp * SAMPLE_RATE );

            tau_matrix(sig2_ind, sig1_ind) = tau;

    // Print the matrix (optional)
    /*
    for (int i = 0; i < 4; ++i) {
        for (int j = 0; j < 4; ++j) {
            std::cout << tau_matrix[i][j] << " ";
        }
        std::cout << std::endl;
    }
    */


        }
    }
    return tau_matrix;
}

Eigen::MatrixXd GCC_PHAT_Eigen(Eigen::MatrixXd& data, int interp) {

  // Pre-allocate Eigen matrices (consider using Eigen::aligned_vector for performance)
  Eigen::VectorXd sig1(data.rows());
  Eigen::VectorXd sig2(data.rows());
  Eigen::VectorXcd SIG1(data.rows() + interp * data.rows());  // Combined vector for FFT
  Eigen::VectorXcd SIG2(data.rows() + interp * data.rows());
  Eigen::VectorXcd R(data.rows() + interp * data.rows());
  Eigen::VectorXcd R_normed(data.rows() + interp * data.rows());
  Eigen::VectorXcd CC_blah(data.rows() + interp * data.rows());
  Eigen::VectorXd CC(data.rows() + interp * data.rows());

  Eigen::FFT<double> fft;  // Create FFT object

  Eigen::MatrixXd tau_matrix(NUM_CHAN, NUM_CHAN);
  tau_matrix.setZero();
  
    for (int sig1_ind = 0; sig1_ind < (NUM_CHAN - 1); sig1_ind++) {
        for (int sig2_ind = sig1_ind + 1; sig2_ind < NUM_CHAN; sig2_ind++) {

        // Extract signal columns
        sig1 = data.col(sig1_ind).array().cwiseAbs();
        sig2 = data.col(sig2_ind).array().cwiseAbs();
        //cout << "After col" << endl;

        // Combine for FFT (zero-padding)
        SIG1.setZero();
        SIG1.head(data.rows()) = sig1;
        SIG2.setZero();
        SIG2.head(data.rows()) = sig2;
        //cout << "After head" << endl;
        // Perform FFTs
        fft.fwd(SIG1, SIG1);
        fft.fwd(SIG2, SIG2);
        
        cout << "SIG1: "; 
        for (int i = 0; i < 20; i++){
            cout << SIG1(i) << " ";
        }
        cout << endl;

        // Calculate cross-correlation
        R = SIG2.array() * SIG1.conjugate().array();
        //cout << "After conj" << endl;
        //R_normed = R / R.abs();
        R_normed = R.array() / R.cwiseAbs().array();
        //cout << "After R_normed" << endl;

        // Perform IFFT with zero-padding
        fft.inv(CC_blah, R_normed);
        //CC = CC_blah.real().abs();
        CC = CC_blah.real().cwiseAbs();

        int max_shift = interp * data.rows() / 2;

        // Extract relevant parts for peak finding
        Eigen::VectorXd sub1 = CC.tail(max_shift);
        Eigen::VectorXd sub2 = CC.head(max_shift);

        // Combine and find peak index
        Eigen::VectorXd CCnew(sub1.size() + sub2.size());
        CCnew << sub1, sub2;
        //Eigen::VectorXd CCnew = sub1.join(sub2);

        //int peak_idx = CCnew.abs().argmax();
        //int peak_idx = CCnew.cwiseAbs().argmax();
        Eigen::Index maxIndex;
        double throwAway = CCnew.maxCoeff(&maxIndex);

        // Calculate time delay
        double shift = static_cast<double>(maxIndex) - max_shift;
        double tau = shift / (interp * SAMPLE_RATE);

        tau_matrix(sig2_ind, sig1_ind) = tau;
        }
    }
    return tau_matrix;
}
