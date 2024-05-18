#include <vector>
#include <algorithm>
#include <numeric>
#include <cmath>
#include <cstdlib>
#include <fstream>
#include <chrono>
#include <iostream>
#include <armadillo> //https://www.uio.no/studier/emner/matnat/fys/FYS4411/v13/guides/installing-armadillo/
#include <iomanip> // for output formatting
#include <exception>
#include <eigen3/Eigen/Dense>
#include <eigen3/unsupported/Eigen/FFT>
#include <eigen3/Eigen/Core>

#include "my_globals.h"
#include "TDOA_estimation.h"

#include <sigpack.h>
#include <fftw/fftw.h>

using std::cout;
using std::endl;
using std::cerr;
using std::vector;
using std::string;
using TimePoint = std::chrono::system_clock::time_point;

arma::Col<double> GCC_PHAT(arma::Mat<double>& data, int interp, sp::FFTW& fftw, int fftLength){
    
    arma::Mat<double> tau_matrix(NUM_CHAN, NUM_CHAN, arma::fill::zeros);
    int n = fftLength;    

    //int n = data.col(0).n_elem + data.col(1).n_elem;
    arma::Col<double> tauVector(6);

    int pairCounter = 0;
    for (int sig1_ind = 0; sig1_ind < (NUM_CHAN - 1); sig1_ind++ ){
        for (int sig2_ind = sig1_ind + 1; sig2_ind < NUM_CHAN; sig2_ind++) {
            
            arma::vec sig1 = data.col(sig1_ind);
            arma::vec sig2 = data.col(sig2_ind);
            //sig2(20) = arma::datum::nan;
            // Perform the FFT using SigPack's FFTW object
            arma::cx_vec SIG1 = fftw.fft(sig1);
            arma::cx_vec SIG2 = fftw.fft(sig2);

            arma::cx_vec R = SIG1 % arma::conj(SIG2);
            arma::cx_vec R_normed = R / arma::abs(R);

            arma::cx_vec CC_blah = fftw.ifft_cx(R_normed);
            arma::vec CC = arma::abs(CC_blah);
            
            int max_shift = interp * n / 2;

            arma::vec sub1 = CC.subvec(CC.n_elem - max_shift, CC.n_elem- 1);
            arma::vec sub2 = CC.subvec(0, max_shift);
            
            arma::vec CCnew = arma::join_cols(sub1,sub2); // join_rows

            double shift = (double)arma::index_max(CCnew) - max_shift;
            double tau = shift / (interp * SAMPLE_RATE );

            //tau_matrix(sig2_ind, sig1_ind) = tau;
            tauVector(pairCounter) = tau;
            pairCounter++;
        }
    }
    return tauVector;
}

Eigen::MatrixXd GCC_PHAT_Eigen(Eigen::MatrixXd& data, int interp) {
    // This is the same algorithm as GCC_PHAT above but implemented using the Eigen library
    Eigen::VectorXd sig1(data.rows());
    Eigen::VectorXd sig2(data.rows());
    Eigen::VectorXcd SIG1(data.rows() + interp * data.rows());  // Combined vector for FFT
    Eigen::VectorXcd SIG1_freq(data.rows() + interp * data.rows());  // Combined vector for FFT
    Eigen::VectorXcd SIG2(data.rows() + interp * data.rows());
    Eigen::VectorXcd SIG2_freq(data.rows() + interp * data.rows());
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
        sig1 = data.col(sig1_ind);
        sig2 = data.col(sig2_ind);

        // Combine for FFT (zero-padding)
        SIG1.setZero();
        SIG1.head(data.rows()) = sig1;
        SIG2.setZero();
        SIG2.head(data.rows()) = sig2;
        // Perform FFTs
        fft.fwd(SIG1_freq, SIG1);
        fft.fwd(SIG2_freq, SIG2);
       
        // Calculate cross-correlation
        R = SIG1_freq.array() * SIG2_freq.conjugate().array();

        R_normed = R.array() / R.cwiseAbs().array();

        // Perform IFFT with zero-padding
        fft.inv(CC_blah, R_normed);
        CC = CC_blah.real().cwiseAbs();
        
        int max_shift = interp * data.rows() / 2;

        // Extract relevant parts for peak finding
        Eigen::VectorXd sub1 = CC.tail(max_shift);
        Eigen::VectorXd sub2 = CC.head(max_shift);

        // Combine and find peak index
        Eigen::VectorXd CCnew(sub1.size() + sub2.size());
        CCnew << sub1, sub2;

        Eigen::Index maxIndex;
        double throwAway = CCnew.maxCoeff(&maxIndex);
        
        double shift = static_cast<double>(maxIndex) - max_shift;
        double tau = shift / (interp * SAMPLE_RATE);

        tau_matrix(sig2_ind, sig1_ind) = tau;
        }
    }
    return tau_matrix;
}

arma::Col<double> DOA_EstimateVerticalArray(arma::Col<double>& TDOAs, double soundSpeed, arma::Col<int> chanSpacing){
    /*
    Estimates the vertical direction of arrival (DOA) using time difference of arrivals (TDOAs) 
    between microphone channels in an array.

    Parameters:
    - TDOAs (arma::Col<double>): Array of time difference of arrivals (TDOAs) between microphone pairs.
    - soundSpeed (double): Speed of sound in the medium, in meters per second.
    - chanSpacing (arma::Col<double>): Array of vertical separation between pairwise microphone channels, in meters.

    Returns:
    - arma::Col<double>: Array containing the estimated vertical DOA angles in degrees.
    
    */

    arma::Col<double> vals = (TDOAs * soundSpeed)  / chanSpacing;             //calculate the distance differences and normalize
    
    double max = arma::max(arma::abs(vals));
    if (max > 1){
        cerr << "Out of bounds values: " << endl;
        cerr << vals.t() << endl; 
        cerr << "TDOAs: " << TDOAs.t() << endl;
        //throw std::exception("Bad TDOA values encountered");
        throw std::runtime_error("Bad TDOA values encountered!");
    }
    
    vals.clamp( -1, 1);                                                       // ensure that values are between -1 and 1 for arcsin
 
    return arma::asin(vals) * 180.0 / 3.141592653;                            // convert angle to degrees 
}
