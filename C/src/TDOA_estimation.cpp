#include <vector>
#include <cmath>
#include <cstdlib>
#include <chrono>
#include <iostream>
#include <armadillo> //https://www.uio.no/studier/emner/matnat/fys/FYS4411/v13/guides/installing-armadillo/

#include "custom_types.h"
#include "TDOA_estimation.h"
#include "utils.h"

#include <fftw3.h>

using std::cout;
using std::endl;
using std::cerr;
using std::vector;




arma::Col<double> GCC_PHAT_FFTW(arma::Mat<arma::cx_double>& savedFFTs, fftw_plan& ip1, const int& interp, int& fftLength, unsigned int& NUM_CHAN, const unsigned int& SAMPLE_RATE) {
    /**
    * @brief Computes the Generalized Cross-Correlation with Phase Transform (GcrossCorr-PHAT) between pairs of signals.
    *
    * @param data A reference to an Armadillo matrix containing the input signals. Each column represents a signal.
    * @param interp An integer specifying the interpolation factor used in the computation.
    * @param fftw An instance of the FFTW object from SigPack library used for Fast Fourier Transform (FFT) computations.
    * @param fftLength An integer specifying the length of the FFT.
    *
    * @return A column vector of doubles containing the computed TDOA estimates for all unique pairs of signals.
    */
    
    //arma::Mat<double> tau_matrix(NUM_CHAN, NUM_CHAN, arma::fill::zeros);

    //int n = data.col(0).n_elem + data.col(1).n_elem;
  //
    arma::Col<double> tauVector(6); // 4 channels produces 6 unique pairings
    arma::Col<arma::cx_double> SIG1(fftLength);
    arma::Col<arma::cx_double> SIG2(fftLength);

    // IFFT input and output
    //cout << "before static " << endl;
    //static fftw_plan ip1 = nullptr;
    //cout << "after static " << endl;
    static arma::cx_vec crossSpectraMagnitudeNorm(497);
    static arma::vec crossCorr(992);
    
    // Get a pointer to the data of crossSpectraMagnitudeNorm for FFTW
    //std::complex<double>* dataPtr = crossSpectraMagnitudeNorm.memptr();

    // Create the plan once if it doesn't exist
    //cout << "before if == nullptr " << endl;
    if (ip1 == nullptr) {
        ip1 = fftw_plan_dft_c2r_1d(992, reinterpret_cast<fftw_complex*>(crossSpectraMagnitudeNorm.memptr()), crossCorr.memptr(), FFTW_ESTIMATE);
    }
    //cout << "after if == nullptr " << endl;

    int pairCounter = 0;
    for (int sig1_ind = 0; sig1_ind < (NUM_CHAN - 1); sig1_ind++ ){
        for (int sig2_ind = sig1_ind + 1; sig2_ind < NUM_CHAN; sig2_ind++) {
            
            
            // Uncomment lines bellow for manual testing
            //sig2(2) =0;
            //sig2(2) = arma::datum::nan; 
            //sig2(2) = arma::datum::inf;
            
            
            SIG1 = savedFFTs.col(sig1_ind);
            SIG2 = savedFFTs.col(sig2_ind);
            /*
            cout << "SIG1:";
            for (int i = 0; i < 5; i++){
                cout << SIG1(i) << " ";
            }
            cout << endl;
            
            cout << "SIG2:";
            for (int i = 0; i < 5; i++){
                cout << SIG2(i) << " ";
            }
            cout << endl;
            */


            arma::cx_vec crossSpectra = SIG1 % arma::conj(SIG2);
            arma::vec crossSpectraMagnitude = arma::abs(crossSpectra);
            
            /*
            cout << "crossSpectraMagnitude:";
            for (int i = 0; i < 5; i++){
                cout << crossSpectraMagnitude(fftLength - (i+1)) << " ";
            }
            cout << endl;
            */

            // Uncomment lines bellow for testing
            //R_abs(2) =0;
            //R_abs(2) = arma::datum::nan; 
            //R_abs(2) = arma::datum::inf;

            if  (crossSpectraMagnitude.has_inf()) [[unlikely]]{
                throw GCC_Value_Error("FFTW R_abs contains inf value");
            }
            else if (crossSpectraMagnitude.has_nan()) [[unlikely]] {
                throw GCC_Value_Error("FFTW R_abs contains nan value");
            }
            else if (arma::any(crossSpectraMagnitude == 0)) [[unlikely]] {
                throw GCC_Value_Error("FFTW R_abs contains 0 value");
            }

            crossSpectraMagnitudeNorm = crossSpectra / crossSpectraMagnitude;
            //cout << "Hello before ifft" << endl;
            fftw_execute(ip1);
            //cout << "Hello after ifft" << endl;
            //arma::vec crossCorr = fftw.ifft(crossSpectraMagnitudeNorm);
            
            //int maxShift = interp * fftLength / 2;
            int maxShift = (interp * (992 / 2));
            //if (fftLength % 2 != 0) {
            //    maxShift = interp * ((992 - 1) / 2);
            //}
            //cout << "Max shift: " << maxShift << endl;
            //cout << "Hello before subvec" << endl;
            //crossCorr /= 992;
            arma::vec back = crossCorr.subvec(crossCorr.n_elem - maxShift, crossCorr.n_elem- 1);
            arma::vec front = crossCorr.subvec(0, maxShift);
            //cout << "Hello after subvec" << endl;
            
            arma::vec crossCorrInverted = arma::join_cols(back,front);

            double shift = (double)arma::index_max(crossCorrInverted) - maxShift;
            double timeDelta = shift / (interp * SAMPLE_RATE );

            //tau_matrix(sig2_ind, sig1_ind) = tau;
            tauVector(pairCounter) = timeDelta;
            pairCounter++;
        }
    }
    //cout << "returning from GCC FFTW" << endl;
    return tauVector;
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
    

    // The following code is designed to test program behavior in the event of a segmentation fault
    /*
    if (WithProbability(0.0005)){
        cout << "Crashing here!!" << endl;
        int* ptr = nullptr;
        *ptr = 42; // Dereference null pointer to cause a crash
    }
    */

    vals.clamp( -1, 1);                                                       // ensure that values are between -1 and 1 for arcsin
    return arma::acos(vals) * 180.0 / 3.141592653;                            // convert angle to degrees 
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
