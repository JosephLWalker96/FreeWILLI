#include "TDOA_estimation.h"

auto GCC_PHAT(Eigen::MatrixXcf &savedFFTs, fftwf_plan &inverseFFT,
                   const int interp, int &paddedLength,
                   const int &NUM_CHAN, const int &SAMPLE_RATE)
    -> std::tuple<Eigen::VectorXf, Eigen::VectorXf>
{
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
    Eigen::VectorXf XCorrPeaks(numTDOAs);
    Eigen::VectorXcf SIG1(fftLength);
    Eigen::VectorXcf SIG2(fftLength);

    static Eigen::VectorXcf crossSpectraMagnitudeNorm(fftLength);
    static Eigen::VectorXf crossCorr(paddedLength);

    if (inverseFFT == nullptr)
    {
        inverseFFT = fftwf_plan_dft_c2r_1d(paddedLength, reinterpret_cast<fftwf_complex *>(crossSpectraMagnitudeNorm.data()), crossCorr.data(), FFTW_ESTIMATE);
        crossSpectraMagnitudeNorm.setZero();
        crossCorr.setZero();
    }

    int pairCounter = 0;
    for (unsigned int sig1_ind = 0; sig1_ind < (NUM_CHAN - 1); sig1_ind++)
    {
        for (unsigned int sig2_ind = sig1_ind + 1; sig2_ind < NUM_CHAN; sig2_ind++)
        {
            SIG1 = savedFFTs.col(sig1_ind);
            SIG2 = savedFFTs.col(sig2_ind);

            Eigen::VectorXcf crossSpectra = SIG1.array() * SIG2.conjugate().array();
            Eigen::VectorXf crossSpectraMagnitude = crossSpectra.cwiseAbs();

            if ((crossSpectraMagnitude.array() == 0).any())
            {
                crossSpectraMagnitude = crossSpectraMagnitude.unaryExpr([](float x)
                                                                        { return x == 0 ? 1.0f : x; });
            }

            if ((crossSpectraMagnitude.array().isInf()).any())
            {
                throw GCC_Value_Error("FFTW crossSpectraMagnitude contains inf value");
            }
            else if ((crossSpectraMagnitude.array().isNaN()).any())
            {
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
            float maxValue = crossCorrInverted.maxCoeff(&maxIndex);

            double shift = static_cast<double>(maxIndex) - maxShift;
            double timeDelta = shift / (interp * SAMPLE_RATE);
            tauVector(pairCounter) = timeDelta;
            XCorrPeaks(pairCounter) = maxValue;

            pairCounter++;
        }
    }
    return std::make_tuple(tauVector, XCorrPeaks);
}

// Compute the Moore-Penrose pseudoinverse using the Singular Value Decomposition (SVD)
Eigen::MatrixXd computePseudoInverse(const Eigen::MatrixXd &H) {
    Eigen::JacobiSVD<Eigen::MatrixXd> svd(H, Eigen::ComputeThinU | Eigen::ComputeThinV);
    double tolerance = std::numeric_limits<double>::epsilon() * std::max(H.cols(), H.rows()) * svd.singularValues().array().abs().maxCoeff();
    Eigen::MatrixXd singularValuesInv = svd.singularValues().array().inverse().matrix().asDiagonal();

    // Only invert values above the tolerance threshold
    for (long i = 0; i < singularValuesInv.rows(); ++i) {
        if (svd.singularValues()(i) < tolerance) {
            singularValuesInv(i, i) = 0;
        }
    }
    
    return svd.matrixV() * singularValuesInv * svd.matrixU().transpose();
}

// Precompute the pseudoinverse instead of QR decomposition
Eigen::MatrixXd precomputedPseudoInverse(const Eigen::MatrixXd &H) {
    /*
    Eigen::MatrixXd mat(3,4);
        // Fill the matrix with the pseudo-inverse values
    mat << 0, 0, 0, 0,
           0, 0, 0, 0,
           -0.0666666, -0.13333333, -0.2, -0.06666666;
    return mat;
    */
    return computePseudoInverse(H);
}

// Precompute the QR decomposition
Eigen::ColPivHouseholderQR<Eigen::MatrixXd> precomputedQR(const Eigen::MatrixXd &H) {
    return H.colPivHouseholderQr();
}


// Precompute the SVD decomposition
Eigen::JacobiSVD<Eigen::MatrixXd> SVD(const Eigen::MatrixXd &H) {
    return Eigen::JacobiSVD<Eigen::MatrixXd>(H, Eigen::ComputeThinU | Eigen::ComputeThinV);
}

// Precompute P = V * Sigma^+ 
Eigen::MatrixXd precomputeInverse(const Eigen::JacobiSVD<Eigen::MatrixXd> &svd) {
    // Obtain matrices from SVD decomposition
    Eigen::MatrixXd V = svd.matrixV();
    Eigen::MatrixXd U = svd.matrixU();
    Eigen::VectorXd singularValues = svd.singularValues();

    // Compute Sigma^+
    Eigen::MatrixXd Sigma_pseudo_inv = Eigen::MatrixXd::Zero(V.cols(), U.cols());
    for (int i = 0; i < singularValues.size(); ++i) {
        if (singularValues(i) > 1e-10) {  // Use a threshold to avoid division by zero
            Sigma_pseudo_inv(i, i) = 1.0 / singularValues(i);
        }
    }

    // Precompute P = V * Sigma^+
    Eigen::MatrixXd P = V * Sigma_pseudo_inv;
    return P;
}

// Function to normalize only the necessary components based on the rank
void normalizeDOA(Eigen::VectorXd &doa, int rank) {
    if (rank == 3) {
        // Full 3D normalization
        doa /= doa.norm();
    } else if (rank == 2) {
        // Normalize only the x and y components
        double xy_norm = std::sqrt(doa(0) * doa(0) + doa(1) * doa(1));
        if (xy_norm > 1e-6) { // Avoid division by zero
            doa(0) /= xy_norm;
            doa(1) /= xy_norm;
        }
    }
    // If rank == 1, normalization might not be meaningful at all
}
// Function to determine if the DOA should be normalized
int GetRank(const Eigen::MatrixXd &H, double tolerance) {
    // Perform SVD decomposition
    Eigen::JacobiSVD<Eigen::MatrixXd> svd(H, Eigen::ComputeThinU | Eigen::ComputeThinV);
    Eigen::VectorXd singularValues = svd.singularValues();

    // Check how many singular values are effectively non-zero
    int rank = 0;
    for (int i = 0; i < singularValues.size(); ++i) {
        if (singularValues(i) > tolerance) {
            ++rank;
        }
    }
    return rank;
}

// Updated TDOA to DOA using precomputed matrices
/*
Eigen::VectorXf TDOA_To_DOA_Optimized(const Eigen::MatrixXd &P, const Eigen::MatrixXd &U, const float speedOfSound, const Eigen::VectorXf &tdoa, int rank) {
    // Convert tdoa to VectorXd and scale it
    Eigen::VectorXd scaled_tdoa = tdoa.cast<double>() * speedOfSound;

    // Compute U^T * scaled_tdoa
    Eigen::VectorXd Ut_tdoa = U.transpose() * scaled_tdoa;

    // Compute DOA using the precomputed matrix P
    Eigen::VectorXd doa = P * Ut_tdoa;

    // Normalize the DOA vector
    if (rank > 1){
        normalizeDOA(doa, rank);
    }

    // Calculate elevation and azimuth
    float el = static_cast<float>(180.0 - std::acos(doa(2)) * 180.0 / M_PI);
    float az = static_cast<float>(std::atan2(doa(1), doa(0)) * 180.0 / M_PI);

    // Create a result vector containing el and az
    Eigen::VectorXf result_vector(2);
    result_vector << el, az;

    return result_vector;
}
*/

Eigen::VectorXf TDOA_To_DOA_Optimized(const Eigen::MatrixXd &P, const Eigen::MatrixXd &U, const float speedOfSound, const Eigen::VectorXf &tdoa, int rank) {
    // Convert tdoa to VectorXd and scale it
    Eigen::VectorXd scaled_tdoa = tdoa.cast<double>() * speedOfSound;

    // Compute U^T * scaled_tdoa
    Eigen::VectorXd Ut_tdoa = U.transpose() * scaled_tdoa;

    // Compute DOA using the precomputed matrix P
    Eigen::VectorXd doa = P * Ut_tdoa;

    // Normalize the DOA vector
    if (rank > 1){
        normalizeDOA(doa, rank);
    }
    return doa.cast<float>();
}


 Eigen::VectorXf DOA_to_ElAz(Eigen::VectorXf& doa){

    // Calculate elevation and azimuth
    float el = static_cast<float>(180.0 - std::acos(doa(2)) * 180.0 / M_PI);
    float az = static_cast<float>(std::atan2(doa(1), doa(0)) * 180.0 / M_PI);

    // Create a result vector containing el and az
    Eigen::VectorXf result_vector(2);
    result_vector << el, az;

    return result_vector;
}

Eigen::VectorXf TDOA_To_DOA_GeneralArray(const Eigen::ColPivHouseholderQR<Eigen::MatrixXd> &qr, const float speedOfSound, const Eigen::VectorXf &tdoa)
{
    // Convert tdoa to VectorXd
    Eigen::VectorXd tdoa_d = tdoa.cast<double>();

    // Solve for DOA using least squares
    Eigen::VectorXd scaled_tdoa = tdoa_d * speedOfSound;
    Eigen::VectorXd doa = qr.solve(scaled_tdoa);

    // Normalize the DOA vector
    doa /= doa.norm();

    // Calculate elevation and azimuth
    float el = static_cast<float>(180.0 - std::acos(doa(2)) * 180.0 / M_PI);
    float az = static_cast<float>(std::atan2(doa(1), doa(0)) * 180.0 / M_PI);

    // Create a result vector containing el, az, and the DOA vector components
    Eigen::VectorXf result_vector(2);
    result_vector << el, az;

    return result_vector;
}

// Convert TDOAs to DOAs using SVD
Eigen::VectorXf TDOA_To_DOA_SVD(const Eigen::JacobiSVD<Eigen::MatrixXd> &svd, const float speedOfSound, const Eigen::VectorXf &tdoa) {
    // Convert tdoa to VectorXd
    Eigen::VectorXd tdoa_d = tdoa.cast<double>();

    // Solve for DOA using the pseudo-inverse from SVD
    Eigen::VectorXd scaled_tdoa = tdoa_d * speedOfSound;
    Eigen::VectorXd doa = svd.solve(scaled_tdoa);

    // Normalize the DOA vector
    std::cout << "DOA before norm: " << std::endl;
    doa /= doa.norm();
    /*
    for (int i = 0; i < 3; i++){
        std::cout << doa[i] << " ";
    }
    std::cout << std::endl;
    doa /= doa.norm();
    std::cout << "DOA after norm: " << std::endl;
    for (int i = 0; i < 3; i++){
        std::cout << doa[i] << " ";
    }
    std::cout << std::endl;
    */

    // Calculate elevation and azimuth
    float el = static_cast<float>(180.0 - std::acos(doa(2)) * 180.0 / M_PI);
    float az = static_cast<float>(std::atan2(doa(1), doa(0)) * 180.0 / M_PI);

    // Create a result vector containing el, az, and the DOA vector components
    Eigen::VectorXf result_vector(2);
    result_vector << el, az;

    return result_vector;
}

/*
// Modify TDOA to DOA calculation to work with the pseudoinverse
Eigen::VectorXf TDOA_To_DOA_GeneralArray(const Eigen::MatrixXd &pseudoInv, const float speedOfSound, const Eigen::VectorXf &tdoa) {
    // Convert tdoa to VectorXd
    Eigen::VectorXd tdoa_d = tdoa.cast<double>();

    // Scale TDOAs by the speed of sound
    Eigen::VectorXd scaled_tdoa = tdoa_d * speedOfSound;

    // Solve for DOA using the pseudoinverse
    Eigen::VectorXd doa = pseudoInv * scaled_tdoa;
    std::cout << "doa from inside function: " << std::endl;
    std::cout << doa << std::endl;

    // Normalize the DOA vector


    std::cout << "doa from inside function after norm: " << std::endl;
    std::cout << doa << std::endl;

    // Calculate elevation and azimuth
    float el = static_cast<float>(180.0 - std::acos(doa(2)) * 180.0 / M_PI);
    float az = static_cast<float>(std::atan2(doa(1), doa(0)) * 180.0 / M_PI);
    

    // Create a result vector containing el, az
    Eigen::VectorXf result_vector(2);
    result_vector << el, az;

    return result_vector;
}
*/


Eigen::VectorXf TDOA_To_DOA_VerticalArray(Eigen::VectorXf &TDOAs, const float &soundSpeed, std::span<float> chanSpacing)
{
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
    if (max > 1.0f)
    {
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
