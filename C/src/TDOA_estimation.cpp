#include "TDOA_estimation.h"

auto GCC_PHAT(const Eigen::MatrixXcf &savedFFTs, fftwf_plan &inverseFFT,
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
    int maxShift = interp * (500 / 2);
    Eigen::VectorXf crossCorrInverted(maxShift * 2);

    static Eigen::VectorXcf crossSpectraMagnitudeNorm(500);
    static Eigen::VectorXf crossCorr(500);

    if (inverseFFT == nullptr)
    {
        inverseFFT = fftwf_plan_dft_c2r_1d(500, reinterpret_cast<fftwf_complex *>(crossSpectraMagnitudeNorm.data()), crossCorr.data(), FFTW_ESTIMATE);
        crossSpectraMagnitudeNorm.setZero();
        crossCorr.setZero();
    }

    int pairCounter = 0;
    for (unsigned int sig1_ind = 0; sig1_ind < (NUM_CHAN - 1); sig1_ind++)
    {
        for (unsigned int sig2_ind = sig1_ind + 1; sig2_ind < NUM_CHAN; sig2_ind++)
        {
            std::cout << "original size: " << savedFFTs.col(sig1_ind).size() << std::endl;
            const auto &SIG1 = savedFFTs.col(sig1_ind).tail(500);
            const auto &SIG2 = savedFFTs.col(sig2_ind).tail(500);

            Eigen::VectorXcf crossSpectra = SIG1.array() * SIG2.conjugate().array();
            Eigen::VectorXf crossSpectraMagnitude = crossSpectra.cwiseAbs();

            // Handle zero magnitudes by replacing zeros with ones
            crossSpectraMagnitude = crossSpectraMagnitude.unaryExpr([](float x)
                                                                    { return x == 0.0f ? 1.0f : x; });

            // Check for inf and nan values
            if (!crossSpectraMagnitude.allFinite())
            {
                throw GCC_Value_Error("FFTW crossSpectraMagnitude contains inf or nan value");
            }

            crossSpectraMagnitudeNorm = crossSpectra.array() / crossSpectraMagnitude.array();

            fftwf_execute(inverseFFT);

            // Rearrange cross-correlation data without extra allocations
            crossCorrInverted.head(maxShift) = crossCorr.tail(maxShift);
            crossCorrInverted.tail(maxShift) = crossCorr.head(maxShift);

            // Find the maximum value and its index
            Eigen::Index maxIndex;
            float maxValue = crossCorrInverted.maxCoeff(&maxIndex);

            // Compute time delay
            double shift = static_cast<double>(maxIndex) - maxShift;
            double timeDelta = shift / (interp * SAMPLE_RATE);
            tauVector(pairCounter) = static_cast<float>(timeDelta);
            XCorrPeaks(pairCounter) = maxValue;

            pairCounter++;
        }
    }
    return std::make_tuple(tauVector, XCorrPeaks);
}

// Precompute the SVD decomposition
Eigen::JacobiSVD<Eigen::MatrixXf> SVD(const Eigen::MatrixXf &H)
{
    return Eigen::JacobiSVD<Eigen::MatrixXf>(H, Eigen::ComputeThinU | Eigen::ComputeThinV);
}

// Precompute P = V * Sigma^+
Eigen::MatrixXf precomputeInverse(const Eigen::JacobiSVD<Eigen::MatrixXf> &svd)
{
    // Obtain matrices from SVD decomposition
    Eigen::MatrixXf V = svd.matrixV();
    Eigen::MatrixXf U = svd.matrixU();
    Eigen::VectorXf singularValues = svd.singularValues();

    // Compute Sigma^+
    Eigen::MatrixXf Sigma_pseudo_inv = Eigen::MatrixXf::Zero(V.cols(), U.cols());
    for (int i = 0; i < singularValues.size(); ++i)
    {
        if (singularValues(i) > 1e-10)
        { // Use a threshold to avoid division by zero
            Sigma_pseudo_inv(i, i) = 1.0 / singularValues(i);
        }
    }

    // Precompute P = V * Sigma^+
    Eigen::MatrixXf P = V * Sigma_pseudo_inv;
    return P;
}

// Function to normalize only the necessary components based on the rank
void normalizeDOA(Eigen::VectorXf &doa, int rank)
{
    if (rank == 3)
    {
        // Full 3D normalization
        doa /= doa.norm();
    }
    else if (rank == 2)
    {
        // Normalize only the x and y components
        double xy_norm = std::sqrt(doa(0) * doa(0) + doa(1) * doa(1));
        if (xy_norm > 1e-6)
        { // Avoid division by zero
            doa(0) /= xy_norm;
            doa(1) /= xy_norm;
        }
    }
    // If rank == 1, normalization might not be meaningful at all
}
// Function to determine if the DOA should be normalized
int GetRank(const Eigen::MatrixXf &H, double tolerance)
{
    // Perform SVD decomposition
    Eigen::JacobiSVD<Eigen::MatrixXf> svd(H, Eigen::ComputeThinU | Eigen::ComputeThinV);
    Eigen::VectorXf singularValues = svd.singularValues();

    // Check how many singular values are effectively non-zero
    int rank = 0;
    for (int i = 0; i < singularValues.size(); ++i)
    {
        if (singularValues(i) > tolerance)
        {
            ++rank;
        }
    }
    return rank;
}

Eigen::VectorXf TDOA_To_DOA_Optimized(const Eigen::MatrixXf &P, const Eigen::MatrixXf &U, const float speedOfSound, const Eigen::VectorXf &tdoa, int rank)
{
    // Convert tdoa to VectorXd and scale it
    Eigen::VectorXf scaled_tdoa = tdoa * speedOfSound;

    // Compute U^T * scaled_tdoa
    Eigen::VectorXf Ut_tdoa = U.transpose() * scaled_tdoa;

    // Compute DOA using the precomputed matrix P
    Eigen::VectorXf doa = P * Ut_tdoa;

    // Normalize the DOA vector
    if (rank > 1)
    {
        normalizeDOA(doa, rank);
    }
    return doa;
}

Eigen::VectorXf DOA_to_ElAz(Eigen::VectorXf &doa)
{

    // Calculate elevation and azimuth
    float el = static_cast<float>(180.0 - std::acos(doa(2)) * 180.0 / M_PI);
    float az = static_cast<float>(std::atan2(doa(1), doa(0)) * 180.0 / M_PI);

    // Create a result vector containing el and az
    Eigen::VectorXf result_vector(2);
    result_vector << el, az;

    return result_vector;
}
