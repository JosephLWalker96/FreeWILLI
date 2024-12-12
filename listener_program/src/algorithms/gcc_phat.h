#pragma once
#include "../pch.h"

class GCC_PHAT
{
public:
    GCC_PHAT(int paddedLength, int numChannels, int sampleRate);

    ~GCC_PHAT();

    std::tuple<Eigen::VectorXf, Eigen::VectorXf> process(const Eigen::MatrixXcf &savedFfts);

private:
    static void initialize(int paddedLength); // Helper to initialize static members

    void calculateNormalizedCrossSpectra(const Eigen::VectorXcf &inputSignal1,
                                         const Eigen::VectorXcf &inputSignal2);

    std::tuple<float, float> estimateTdoaAndPeak();

    fftwf_plan mInverseFftPlan;
    int mPaddedLength;

    int mMaxShift;

    // Reusable buffers
    static Eigen::VectorXcf mNormalizedCrossSpectra;
    static Eigen::VectorXf mCrossCorrBuffer;

    int mNumChannels;
    int mNumTdoas;
    int mSampleRate;
};