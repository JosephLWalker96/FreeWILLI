#include "TDOA_estimation.h"

using std::cout;
using std::endl;
using std::cerr;
using std::vector;


Eigen::VectorXf GCC_PHAT_FFTW_E(Eigen::MatrixXcf& savedFFTs, fftwf_plan& inverseFFT, const int& interp, int& paddedLength, unsigned int& NUM_CHAN, const unsigned int& SAMPLE_RATE) {
    /**
    * @brief Computes the Generalized Cross-Correlation with Phase Transform (GCC-PHAT) between pairs of signals.
    *
    * @param savedFFTs A reference to an Eigen matrix containing the input signals. Each column represents a signal.
    * @param ip1 An FFTW plan for inverse FFT computations.
    * @param interp An integer specifying the interpolation factor used in the computation.
    * @param fftLength An integer specifying the length of the FFT.
    * @param NUM_CHAN An unsigned integer specifying the number of channels.
    * @param SAMPLE_RATE An unsigned integer specifying the sample rate.
    *
    * @return An Eigen vector of doubles containing the computed TDOA estimates for all unique pairs of signals.
    */

    int fftLength = savedFFTs.col(0).size();
    int numTDOAs = NUM_CHAN * (NUM_CHAN - 1) / 2;
    Eigen::VectorXf tauVector(numTDOAs);
    Eigen::VectorXcf SIG1(fftLength);
    Eigen::VectorXcf SIG2(fftLength);

    static Eigen::VectorXcf crossSpectraMagnitudeNorm(fftLength);
    static Eigen::VectorXf crossCorr(paddedLength);

    if (inverseFFT == nullptr) {
        inverseFFT = fftwf_plan_dft_c2r_1d(paddedLength, reinterpret_cast<fftwf_complex*>(crossSpectraMagnitudeNorm.data()), crossCorr.data(), FFTW_ESTIMATE);
    }

    int pairCounter = 0;
    for (unsigned int sig1_ind = 0; sig1_ind < (NUM_CHAN - 1); sig1_ind++) {
        for (unsigned int sig2_ind = sig1_ind + 1; sig2_ind < NUM_CHAN; sig2_ind++) {
            SIG1 = savedFFTs.col(sig1_ind);
            SIG2 = savedFFTs.col(sig2_ind);

            Eigen::VectorXcf crossSpectra = SIG1.array() * SIG2.conjugate().array();
            Eigen::VectorXf crossSpectraMagnitude = crossSpectra.cwiseAbs();
            
            if ((crossSpectraMagnitude.array() == 0).any()) {
                crossSpectraMagnitude = crossSpectraMagnitude.unaryExpr([](float x) { return x == 0 ? 1.0f : x; });
            }

            if ((crossSpectraMagnitude.array().isInf()).any()) {
                throw GCC_Value_Error("FFTW crossSpectraMagnitude contains inf value");
            } else if ((crossSpectraMagnitude.array().isNaN()).any()) {
                throw GCC_Value_Error("FFTW crossSpectraMagnitude contains nan value");
            }
            crossSpectraMagnitudeNorm = crossSpectra.array() / crossSpectraMagnitude.array();
            
            
            fftwf_execute(inverseFFT);

            int maxShift = (interp * (paddedLength / 2));
            Eigen::VectorXf back = crossCorr.tail(maxShift);
            Eigen::VectorXf front = crossCorr.head(maxShift);

            Eigen::VectorXf crossCorrInverted(maxShift * 2);
            crossCorrInverted << back, front;

            Eigen::Index maxIndex;
            crossCorrInverted.maxCoeff(&maxIndex);
            
            double shift = static_cast<double>(maxIndex) - maxShift;
            double timeDelta = shift / (interp * SAMPLE_RATE);
            tauVector(pairCounter) = timeDelta;
            pairCounter++;
        }
    }
    return tauVector;
}
Eigen::VectorXf DOA_EstimateVerticalArray(Eigen::VectorXf& TDOAs, const double& soundSpeed, vector<float>& chanSpacing) {
    /**
    * @brief Estimates the vertical direction of arrival (DOA) using time difference of arrivals (TDOAs) 
    * between microphone channels in an array.
    * 
    * @param TDOAs (Eigen::VectorXd): Array of time difference of arrivals (TDOAs) between microphone pairs.
    * @param soundSpeed (double): Speed of sound in the medium, in meters per second.
    * @param chanSpacing (Eigen::VectorXi): Array of vertical separation between pairwise microphone channels, in meters.
    * 
    * @return Eigen::VectorXd Array containing the estimated vertical DOA angles in degrees.
    */
    
    Eigen::VectorXf vals = (TDOAs * soundSpeed).array(); // calculate the distance differences and normalize
    
    // Convert std::vector<double> to Eigen::VectorXd
    Eigen::VectorXf divisorEigen = Eigen::Map<Eigen::VectorXf>(chanSpacing.data(), chanSpacing.size());

    // Perform element-wise division
    vals = vals.array() / divisorEigen.array();
    float max = vals.array().abs().maxCoeff();
    if (max > 1.0f) {
        std::cerr << "Out of bounds values: " << std::endl;
        std::cerr << vals.transpose() << std::endl; 
        std::cerr << "TDOAs: " << TDOAs.transpose() << std::endl;
        // throw std::runtime_error("Bad TDOA values encountered!");
    }

    // Ensure that values are between -1 and 1 for arcsin
    vals = vals.array().min(1).max(-1);

    // Convert angle to degrees
    return vals.array().acos() * 180.0f / 3.1415f;
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
