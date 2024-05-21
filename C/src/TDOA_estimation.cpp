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

arma::Col<double> GCC_PHAT(arma::Mat<double>& data, int& interp, sp::FFTW& fftw, int& fftLength, unsigned int& NUM_CHAN, const unsigned int& SAMPLE_RATE){
    /**
    * @brief Computes the Generalized Cross-Correlation with Phase Transform (GCC-PHAT) between pairs of signals.
    *
    * @param data A reference to an Armadillo matrix containing the input signals. Each column represents a signal.
    * @param interp An integer specifying the interpolation factor used in the computation.
    * @param fftw An instance of the FFTW object from SigPack library used for Fast Fourier Transform (FFT) computations.
    * @param fftLength An integer specifying the length of the FFT.
    *
    * @return A column vector of doubles containing the computed TDOA estimates for all unique pairs of signals.
    */
    
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

Eigen::MatrixXd GCC_PHAT_Eigen(Eigen::MatrixXd& data, int interp, unsigned int& NUM_CHAN, const unsigned int& SAMPLE_RATE) {
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
            CCnew.maxCoeff(&maxIndex);
            
            double shift = static_cast<double>(maxIndex) - max_shift;
            double tau = shift / (interp * SAMPLE_RATE);

            tau_matrix(sig2_ind, sig1_ind) = tau;
        }
    }
    return tau_matrix;
}

arma::Col<double> DOA_EstimateVerticalArray(arma::Col<double>& TDOAs, const double& soundSpeed, arma::Col<int>& chanSpacing){
    /**
    * @brief Estimates the vertical direction of arrival (DOA) using time difference of arrivals (TDOAs) 
    * between microphone channels in an array.
    * 
    * @param TDOAs (arma::Col<double>): Array of time difference of arrivals (TDOAs) between microphone pairs.
    * @param soundSpeed (double): Speed of sound in the medium, in meters per second.
    * @param chanSpacing (arma::Col<double>): Array of vertical separation between pairwise microphone channels, in meters.
    * 
    * @return arma::Col<double> Array containing the estimated vertical DOA angles in degrees.
    */
    
    arma::Col<double> vals = (TDOAs * soundSpeed)  / chanSpacing;             //calculate the distance differences and normalize
    
    double max = arma::max(arma::abs(vals));
    if (max > 1){
        cerr << "Out of bounds values: " << endl;
        cerr << vals.t() << endl; 
        cerr << "TDOAs: " << TDOAs.t() << endl;
        //throw std::runtime_error("Bad TDOA values encountered!");
    }
    
    vals.clamp( -1, 1);                                                       // ensure that values are between -1 and 1 for arcsin
    return arma::asin(vals) * 180.0 / 3.141592653;                            // convert angle to degrees 
}


/*
void ApplyKalman(KF& kalman){


  // Number of samples
  arma::uword Nsamp = 120;
  arma::uword N     = 6; // Nr of states
  arma::uword M     = 2; // Nr of measurements
  arma::uword L     = 1; // Nr of inputs
  // Instatiate a Kalman Filter
  KF kalman(N, M, L);
  // Initialisation and setup of system
  double P0 = 100;
  double Q0 = 0.00003;
  double R0 = 25;
  // Meas interval
  double dT = 0.1;
  arma::mat x = {0, 10, 1, 50, -0.08, -9};
  kalman.set_state_vec(0.9 * x.t());
  // [X,Y,Vx,Vy,Ax,Ay] use position, velocity and acceleration as states
  arma::mat A = {{1, 0, dT, 0, dT * dT / 2, 0},
                 {0, 1, 0, dT, 0, dT * dT / 2},
                 {0, 0, 1, 0, dT, 0},
                 {0, 0, 0, 1, 0, dT},
                 {0, 0, 0, 0, 1, 0},
                 {0, 0, 0, 0, 0, 1}};
  kalman.set_trans_mat(A);
  arma::mat H = {{1, 0, 0, 0, 0, 0}, {0, 1, 0, 0, 0, 0}};
  kalman.set_meas_mat(H);
  arma::mat P = P0 * arma::eye(N, N);
  kalman.set_err_cov(P);
  arma::mat Q     = arma::zeros(N, N);
  
  Q(N - 2, N - 2) = 1;
  Q(N - 1, N - 1) = 1;
  Q               = Q0 * Q;
  kalman.set_proc_noise(Q);
  arma::mat R = R0 * arma::eye(M, M);
  kalman.set_meas_noise(R);
  // Create simulation data
  arma::mat z(M, Nsamp, arma::fill::zeros);
  arma::mat z0(M, Nsamp, arma::fill::zeros);
  arma::mat xx(N, 1, arma::fill::zeros);
  xx = x.t();
  for (arma::uword n = 1; n < Nsamp; n++)
  {
    xx        = A * xx + 0.1 * Q * arma::randn(N, 1);
    z0.col(n) = H * xx; //
  }
  z.row(0) = z0.row(0) + 0.001 * R0 * arma::randn(1, Nsamp);
  z.row(1) = z0.row(1) + 0.8 * R0 * arma::randn(1, Nsamp);
  arma::mat x_log(N, Nsamp);
  arma::mat e_log(M, Nsamp);
  arma::cube P_log(N, N, Nsamp);
  arma::mat xs_log(M, Nsamp);
  arma::cube Ps_log(N, N, Nsamp);
  // Kalman filter loop
  for (arma::uword n = 0; n < Nsamp; n++)
  {
    kalman.predict();
    kalman.update(z.col(n));
    x_log.col(n)   = kalman.get_state_vec();
    e_log.col(n)   = kalman.get_err();
    P_log.slice(n) = kalman.get_err_cov();
  }
}
*/
