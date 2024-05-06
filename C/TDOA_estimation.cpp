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
#include <sigpack.h>

#include "my_globals.h"
#include "TDOA_estimation.h"

using std::cout;
using std::endl;
using std::cerr;
using std::vector;
using std::string;
using TimePoint = std::chrono::system_clock::time_point;


void GCC_PHAT(arma::Mat<double>& data, int interp){
    double tau_matrix[NUM_CHAN][NUM_CHAN] = {0.0};

    for (int sig1_ind = 0; sig1_ind < (NUM_CHAN - 1); sig1_ind++ ){
        for (int sig2_ind = sig1_ind + 1; sig2_ind < NUM_CHAN; sig2_ind++) {

            arma::vec sig1 = arma::abs(data.col(sig1_ind));
            arma::vec sig2 = arma::abs(data.col(sig2_ind));
            //cout << "Made it to col insert " << endl;

            int n = sig1.n_elem + sig2.n_elem;

            arma::cx_vec SIG1 = arma::fft(sig1,n);
            arma::cx_vec SIG2 = arma::fft(sig2,n);
            //cout << "Made it to fft " << endl;

            arma::cx_vec R = SIG1 % arma::conj(SIG2);
            //cout << "made it to conj" << endl;
            arma::cx_vec R_normed = R / arma::abs(R);

            //cout << "made it to R_norm" << endl;
            arma::cx_vec CC = arma::ifft(R_normed, interp * n);
            
            //cout << "CC: " << CC.n_elem << endl;
            
            int max_shift = interp * n / 2;

            arma::cx_vec sub1 = CC.subvec(CC.n_elem - max_shift, CC.n_elem- 1);
            arma::cx_vec sub2 = CC.subvec(0, max_shift);
            
            //cout << "made it to subvecs" << endl;

            arma::cx_vec CCnew = arma::join_cols(sub1,sub2); // join_rows
            
            //cout << "made it to concatenate: " << CCnew.n_elem << endl;

            //argmax(abs(CC)) - shift
            //tau = shift / (float(interp) * fs)

            int shift = arma::index_max(arma::abs(CCnew)) - max_shift;

            //cout << "made it to shift : " << endl;
            double tau = shift / (double(interp) * SAMPLE_RATE );

            //cout << "tau: " << tau << endl;

            tau_matrix[sig2_ind][sig1_ind] = tau;

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
}
