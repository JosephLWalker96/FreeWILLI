#pragma once
#include "../pch.h"

class IFrequencyDomainStrategy
{
public:
    virtual ~IFrequencyDomainStrategy() = default;

    virtual void initialize(Eigen::MatrixXf &channelData) = 0;
    virtual void apply(Eigen::MatrixXf &channelData) = 0;
    virtual int getPaddedLength() = 0;

    virtual Eigen::MatrixXcf &getFrequencyDomainData() = 0;
};


class FrequencyDomainFilterStrategy : public IFrequencyDomainStrategy
{
public:

    FrequencyDomainFilterStrategy(const std::string& filename, int channelSize, int numChannels);
    ~FrequencyDomainFilterStrategy();
    Eigen::MatrixXcf &getFrequencyDomainData() override;
    
    int getPaddedLength() override;
    void initialize(Eigen::MatrixXf &channelData) override;
    void apply(Eigen::MatrixXf &channelData) override;

private:
    void initializeFilterWeights(const std::vector<float> &filterWeights);
    auto readFirFilterFile(const std::string &filePath) -> std::vector<float>;
    void initialize(int fftOutputSize);

    static Eigen::MatrixXcf mSavedFFTs; // frequency domain data (outputs)
    int mNumChannels;
    int mPaddedLength;
    int mFftOutputSize;
    Eigen::VectorXcf mFilterFreq;
    fftwf_plan mForwardFftPlan = nullptr;
};




class FrequencyDomainNoFilterStrategy : public IFrequencyDomainStrategy
{
public:

    FrequencyDomainNoFilterStrategy(int channelSize, int numChannels);
    ~FrequencyDomainNoFilterStrategy();
    Eigen::MatrixXcf &getFrequencyDomainData() override;
    
    int getPaddedLength() override;
    void initialize(Eigen::MatrixXf &channelData) override;
    void apply(Eigen::MatrixXf &channelData) override;

private:
    void initialize(int fftOutputSize);

    static Eigen::MatrixXcf mSavedFFTs; // frequency domain data (outputs)
    int mNumChannels;
    int mPaddedLength;
    int mFftOutputSize;
    Eigen::VectorXcf mFilterFreq;
    fftwf_plan mForwardFftPlan = nullptr;
};