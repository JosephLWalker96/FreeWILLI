#pragma once

#include "../pch.h"
class FrequencyDomainStrategy
{
public:
    int mPaddedLength;

    FrequencyDomainStrategy(const std::string &filterWeightsPath, Eigen::MatrixXf &channeData, int channelSize, int numChannels);
    Eigen::MatrixXcf &getFrequencyDomainData();
    void apply();

private:
    void initializeFilterWeights(const std::vector<float> &filterWeights);
    auto readFirFilterFile(const std::string &filePath) -> std::vector<float>;
    void initialize(int fftOutputSize);

    int mNumChannels;

    static Eigen::MatrixXcf mSavedFFTs; // frequency domain data (outputs)

    Eigen::VectorXcf mFilterFreq;
    fftwf_plan mForwardFftPlan;
};