#pragma once
#include "../pch.h"
class GCC_PHAT
{
   public:
    GCC_PHAT(int paddedLength, int spectraLength, int numChannels, int sampleRate);
    ~GCC_PHAT();

    std::tuple<Eigen::VectorXf, Eigen::VectorXf> process(const Eigen::MatrixXcf& savedFfts);

   private:
    void calculateNormalizedCrossSpectra(const Eigen::VectorXcf& s1, const Eigen::VectorXcf& s2);
    std::tuple<float, float> estimateTdoaAndPeak();

    int mPaddedLength;
    int mNumChannels;
    int mSampleRate;
    int mNumTdoas;
    int mMaxShift;

    Eigen::VectorXcf mNormalizedCrossSpectra;
    Eigen::VectorXf mCrossCorrBuffer;

    fftwf_plan mInverseFftPlan;
};