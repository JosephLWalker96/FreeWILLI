#include "gcc_phat.h"
GCC_PHAT::GCC_PHAT(int paddedLength, int spectraLength, int numChannels, int sampleRate)
    : mPaddedLength(paddedLength),
      mNumChannels(numChannels),
      mSampleRate(sampleRate),
      mNumTdoas(numChannels * (numChannels - 1) / 2),
      mMaxShift(paddedLength / 2),
      mNormalizedCrossSpectra(Eigen::VectorXcf::Zero(spectraLength)),
      mCrossCorrBuffer(Eigen::VectorXf::Zero(paddedLength))
{
    mInverseFftPlan = fftwf_plan_dft_c2r_1d(
        mPaddedLength, reinterpret_cast<fftwf_complex*>(mNormalizedCrossSpectra.data()), mCrossCorrBuffer.data(),
        FFTW_ESTIMATE);
}

GCC_PHAT::~GCC_PHAT() { fftwf_destroy_plan(mInverseFftPlan); }

std::tuple<Eigen::VectorXf, Eigen::VectorXf> GCC_PHAT::process(const Eigen::MatrixXcf& savedFfts)
{
    Eigen::VectorXf tdoaEstimates(mNumTdoas);
    Eigen::VectorXf crossCorrPeaks(mNumTdoas);

    int pairIndex = 0;
    for (int ch1 = 0; ch1 < mNumChannels - 1; ++ch1)
    {
        for (int ch2 = ch1 + 1; ch2 < mNumChannels; ++ch2)
        {
            calculateNormalizedCrossSpectra(savedFfts.col(ch1), savedFfts.col(ch2));

            fftwf_execute(mInverseFftPlan);
            auto [tdoa, peak] = estimateTdoaAndPeak();
            tdoaEstimates(pairIndex) = tdoa;
            crossCorrPeaks(pairIndex) = peak;
            ++pairIndex;
        }
    }

    return {tdoaEstimates, crossCorrPeaks};
}

void GCC_PHAT::calculateNormalizedCrossSpectra(const Eigen::VectorXcf& s1, const Eigen::VectorXcf& s2)
{
    Eigen::VectorXcf crossSpectrum = s1.array() * s2.conjugate().array();
    Eigen::VectorXf magnitudes = crossSpectrum.cwiseAbs().unaryExpr([](float x) { return (x == 0.0f) ? 1.0f : x; });

    if (!magnitudes.allFinite())
    {
        throw std::runtime_error("Cross-spectrum contains invalid (inf/NaN) values.");
    }
    assert(
        mNormalizedCrossSpectra.size() == crossSpectrum.size() &&
        "Sizes of mNormalizedCrossSpectra and crossSpectrum do not match");

    mNormalizedCrossSpectra.array() = crossSpectrum.array() / magnitudes.array();
    // mNormalizedCrossSpectra.array() = crossSpectrum.array();
}

std::tuple<float, float> GCC_PHAT::estimateTdoaAndPeak()
{
    Eigen::VectorXf rearranged(2 * mMaxShift);
    rearranged.head(mMaxShift) = mCrossCorrBuffer.tail(mMaxShift);
    rearranged.tail(mMaxShift) = mCrossCorrBuffer.head(mMaxShift);

    Eigen::Index peakIndex;
    float peakVal = rearranged.maxCoeff(&peakIndex);

    float shift = static_cast<float>(peakIndex) - mMaxShift;
    float tdoa = shift / static_cast<float>(mSampleRate);

    return {tdoa, peakVal};
}