#include "gcc_phat.h"

Eigen::VectorXcf GCC_PHAT::mNormalizedCrossSpectra;
Eigen::VectorXf GCC_PHAT::mCrossCorrBuffer;

void GCC_PHAT::initialize(int paddedLength)
{
    if (mNormalizedCrossSpectra.size() == 0)
    { // Prevent reinitialization
        mNormalizedCrossSpectra = Eigen::VectorXcf::Zero(paddedLength);
        mCrossCorrBuffer = Eigen::VectorXf::Zero(paddedLength);
    }
}

/**
 * @brief Constructs a GCC_PHAT instance.
 *
 * @param paddedLength Length of zero-padding applied to the signals for FFT.
 */
GCC_PHAT::GCC_PHAT(int paddedLength, int numChannels, int sampleRate)
    : mPaddedLength(paddedLength),
      mNumChannels(numChannels),
      mSampleRate(sampleRate)
{
    GCC_PHAT::initialize(paddedLength);

    mNumTdoas = numChannels * (numChannels - 1) / 2;
    mMaxShift = paddedLength / 2; // change this if we have knowledge of max time offset

    mInverseFftPlan = fftwf_plan_dft_c2r_1d(
        paddedLength,
        reinterpret_cast<fftwf_complex *>(mNormalizedCrossSpectra.data()),
        mCrossCorrBuffer.data(),
        FFTW_ESTIMATE);
}

/**
 * @brief Destructor for GCC_PHAT. Cleans up FFTW resources.
 */
GCC_PHAT::~GCC_PHAT()
{
    if (mInverseFftPlan != nullptr)
    {
        fftwf_destroy_plan(mInverseFftPlan);
    }
}

/**
 * @brief Compute TDOAs using GCC-PHAT method.
 */
std::tuple<Eigen::VectorXf, Eigen::VectorXf> GCC_PHAT::process(const Eigen::MatrixXcf &savedFfts)
{
    Eigen::VectorXf tdoaEstimates(mNumTdoas);
    Eigen::VectorXf crossCorrPeaks(mNumTdoas);

    int pairCounter = 0;
    for (int signal1Index = 0; signal1Index < mNumChannels - 1; ++signal1Index)
    {
        for (int signal2Index = signal1Index + 1; signal2Index < mNumChannels; ++signal2Index)
        {
            const auto &signal1 = savedFfts.col(signal1Index);
            const auto &signal2 = savedFfts.col(signal2Index);
            //std::cout << "signal1 length: " << signal1.size() << std::endl;
            //std::cout << "signal1: " << signal1.head(10) << std::endl;

            // Compute normalized cross-spectra
            calculateNormalizedCrossSpectra(signal1, signal2);

            // Perform the inverse FFT to compute cross-correlation
            fftwf_execute(mInverseFftPlan);

            // Estimate TDOA from the computed cross-correlation
            std::tie(tdoaEstimates(pairCounter), crossCorrPeaks(pairCounter)) =
                estimateTdoaAndPeak();

            ++pairCounter;
        }
    }

    return std::make_tuple(tdoaEstimates, crossCorrPeaks);
}

/**
 * @brief Computes the normalized cross-spectrum of two signals.
 *
 * @param inputSignal1 The first input signal in frequency domain.
 * @param inputSignal2 The second input signal in frequency domain.
 */
void GCC_PHAT::calculateNormalizedCrossSpectra(const Eigen::VectorXcf &inputSignal1,
                                               const Eigen::VectorXcf &inputSignal2)
{

    // Compute cross-spectrum
    Eigen::VectorXcf crossSpectrum = inputSignal1.array() * inputSignal2.conjugate().array();

    // Compute magnitude and handle zeros
    Eigen::VectorXf crossSpectrumMagnitude = crossSpectrum.cwiseAbs();
    crossSpectrumMagnitude = crossSpectrumMagnitude.unaryExpr([](float magnitude)
                                                              { return (magnitude == 0.0f) ? 1.0f : magnitude; });
    // Validate that all values are finite
    if (!crossSpectrumMagnitude.allFinite())
    {
        throw std::runtime_error("Cross-spectrum contains invalid values (inf or NaN).");
    }

    // Normalize to get phase transform
    mNormalizedCrossSpectra = crossSpectrum.array() / crossSpectrumMagnitude.array();
}

/**
 * @brief Processes the cross-correlation buffer to estimate TDOA and find the peak value.
 *
 * @param bufferLength Total length of the padded cross-correlation buffer.
 * @param samplingRate Sampling rate in Hz.
 * @return A tuple (tdoaInSeconds, peakValue).
 */
std::tuple<float, float> GCC_PHAT::estimateTdoaAndPeak()
{

    // Rearrange cross-correlation data
    Eigen::VectorXf rearrangedCrossCorrelation(2 * mMaxShift);
    rearrangedCrossCorrelation.head(mMaxShift) = mCrossCorrBuffer.tail(mMaxShift);
    rearrangedCrossCorrelation.tail(mMaxShift) = mCrossCorrBuffer.head(mMaxShift);

    // Identify the peak
    Eigen::Index peakIndex;
    float peakValue = rearrangedCrossCorrelation.maxCoeff(&peakIndex);

    double shift = static_cast<double>(peakIndex) - mMaxShift;
    double tdoa = shift / static_cast<double>(mSampleRate);

    return {static_cast<float>(tdoa), peakValue};
}