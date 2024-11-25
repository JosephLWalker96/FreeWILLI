#include "gcc_phat.h"
#include <fftw3.h>

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

/**
 * @brief Processes the cross-correlation buffer to estimate the time delay of arrival (TDOA)
 *        and find the peak cross-correlation value.
 *
 * @param crossCorrelationBuffer The cross-correlation values computed from the inverse FFT.
 * @param bufferLength The total length of the padded cross-correlation buffer.
 * @param samplingRate The sampling rate of the signals in Hz.
 *
 * @return A tuple containing:
 *         - float: The estimated time delay of arrival (TDOA) in seconds.
 *         - float: The peak cross-correlation value.
 */
std::tuple<float, float> estimateTdoaAndPeak(const Eigen::VectorXf &crossCorrelationBuffer,
                                             int bufferLength, int samplingRate)
{
    // Compute the maximum shift range
    int maxShift = bufferLength / 2;

    // Rearrange the cross-correlation data for linear TDOA interpretation
    /*
    Eigen::VectorXf rearrangedCrossCorrelation(2 * maxShift);
    rearrangedCrossCorrelation.head(maxShift) = crossCorrelationBuffer.tail(maxShift);
    rearrangedCrossCorrelation.tail(maxShift) = crossCorrelationBuffer.head(maxShift);
    */
    Eigen::VectorXf back = crossCorrelationBuffer.tail(maxShift);
    Eigen::VectorXf front = crossCorrelationBuffer.head(maxShift);
    Eigen::VectorXf rearrangedCrossCorrelation(maxShift * 2);
    rearrangedCrossCorrelation << back, front;

    // Identify the peak cross-correlation value and its index
    Eigen::Index peakIndex;
    float peakValue = rearrangedCrossCorrelation.maxCoeff(&peakIndex);

    // Calculate the time delay corresponding to the peak index
    double shift = static_cast<double>(peakIndex) - maxShift;
    double tdoa = shift / samplingRate;

    return {static_cast<float>(tdoa), peakValue};
}

/**
 * @brief Computes the normalized cross-spectrum of two signals.
 *
 * @param inputSignal1 The first input signal in the frequency domain, represented as a column of FFT coefficients.
 * @param inputSignal2 The second input signal in the frequency domain, represented as a column of FFT coefficients.
 *
 * @throws std::runtime_error If the cross-spectrum contains invalid values (e.g., infinity or NaN).
 */
Eigen::VectorXcf calculateNormalizedCrossSpectra(const Eigen::VectorXcf &inputSignal1,
                                                 const Eigen::VectorXcf &inputSignal2)
{

    // Compute the cross-spectrum of the two signals
    Eigen::VectorXcf crossSpectrum = inputSignal1.array() * inputSignal2.conjugate().array();

    // Compute the magnitude of the cross-spectrum and replace zero values with one to avoid division by zero
    Eigen::VectorXf crossSpectrumMagnitude = crossSpectrum.cwiseAbs();
    crossSpectrumMagnitude = crossSpectrumMagnitude.unaryExpr([](float magnitude)
                                                              { return magnitude == 0.0f ? 1.0f : magnitude; });

    // Validate that all values in the cross-spectrum are finite
    if (!crossSpectrumMagnitude.allFinite())
    {
        throw std::runtime_error("Cross-spectrum contains invalid values (inf or NaN).");
    }

    // Normalize the cross-spectrum to compute the phase transform
    return crossSpectrum.array() / crossSpectrumMagnitude.array();
}

/**
 * @brief Computes the Generalized Cross-Correlation with Phase Transform (GCC-PHAT) between pairs of signals.
 *
 * @param savedFfts A reference to an Eigen matrix containing the Fourier Transform of the FIR-filtered signals.
 *                  Each column represents a signal.
 * @param inverseFftPlan FFTW plan for inverse FFT computations.
 * @param paddedLength Length of the FFT or padding applied to the signals.
 * @param numChannels Number of input signal channels.
 * @param sampleRate Sampling rate of the signals in Hz.
 *
 * @return A tuple containing:
 *         - Eigen::VectorXf: Estimated time delays of arrival (TDOA) for all unique signal pairs.
 *         - Eigen::VectorXf: Peak cross-correlation values for each signal pair.
 */
auto computeGccPhat(Eigen::MatrixXcf &savedFfts, fftwf_plan &inverseFftPlan,
                    int &paddedLength, const int &numChannels, const int &sampleRate)
    -> std::tuple<Eigen::VectorXf, Eigen::VectorXf>
{

    int fftLength = savedFfts.col(0).size();
    int numTdoas = numChannels * (numChannels - 1) / 2;
    Eigen::VectorXf tdoaEstimates(numTdoas);
    Eigen::VectorXf crossCorrPeaks(numTdoas);

    Eigen::VectorXcf signal1(fftLength);
    Eigen::VectorXcf signal2(fftLength);

    // Static buffers for efficient memory reuse
    static Eigen::VectorXcf normalizedCrossSpectra(fftLength);
    static Eigen::VectorXf crossCorrBuffer(paddedLength);

    // Initialize FFTW plan if not already set
    if (inverseFftPlan == nullptr)
    {
        inverseFftPlan = fftwf_plan_dft_c2r_1d(
            paddedLength,
            reinterpret_cast<fftwf_complex *>(normalizedCrossSpectra.data()),
            crossCorrBuffer.data(),
            FFTW_ESTIMATE);
        // normalizedCrossSpectra.setZero();
        // crossCorrBuffer.setZero();
    }

    int pairCounter = 0;
    for (int signal1Index = 0; signal1Index < numChannels - 1; ++signal1Index)
    {
        for (int signal2Index = signal1Index + 1; signal2Index < numChannels; ++signal2Index)
        {
            // Retrieve signals
            signal1 = savedFfts.col(signal1Index);
            signal2 = savedFfts.col(signal2Index);

            normalizedCrossSpectra = calculateNormalizedCrossSpectra(signal1, signal2);

            // Perform the inverse FFT to compute the cross-correlation
            assert(normalizedCrossSpectra.size() == fftLength);
            assert(crossCorrBuffer.size() == paddedLength);
            assert(fftLength == paddedLength);
            fftwf_execute(inverseFftPlan);

            // Rearrange cross-correlation data for TDOA estimation
            // Process the cross-correlation data and compute TDOA
            std::tie(tdoaEstimates(pairCounter), crossCorrPeaks(pairCounter)) =
                estimateTdoaAndPeak(crossCorrBuffer, paddedLength, sampleRate);

            ++pairCounter;
        }
    }
    std::cout << "TDOAS: " << tdoaEstimates << std::endl;
    return std::make_tuple(tdoaEstimates, crossCorrPeaks);
}