#pragma once
#include "../pch.h"

class IFrequencyDomainStrategy
{
   public:
    virtual ~IFrequencyDomainStrategy() = default;

    virtual void apply() = 0;
    virtual int getPaddedLength() const = 0;

    virtual Eigen::MatrixXcf& getFrequencyDomainData() = 0;

    Eigen::MatrixXcf mBeforeFilter;
};

class FrequencyDomainFilterStrategy : public IFrequencyDomainStrategy
{
   public:
    FrequencyDomainFilterStrategy(const std::string& filterPath, Eigen::MatrixXf& channelData, int numChannels);
    ~FrequencyDomainFilterStrategy();

    void apply() override;
    int getPaddedLength() const override;
    Eigen::MatrixXcf& getFrequencyDomainData() override;

   private:
    void initializeFilterWeights(const std::vector<float>& filterWeights);
    void createFftPlan(Eigen::MatrixXf& channelData);
    std::vector<float> readFirFilterFile(const std::string& filePath);

   private:
    int mNumChannels;
    int mPaddedLength;
    int mFftOutputSize;
    Eigen::VectorXcf mFilterFreq;

    fftwf_plan mForwardFftPlan = nullptr;

    Eigen::MatrixXcf mSavedFFTs;
};

class FrequencyDomainNoFilterStrategy : public IFrequencyDomainStrategy
{
   public:
    FrequencyDomainNoFilterStrategy(Eigen::MatrixXf& channelData, int numChannels) : mNumChannels(numChannels)
    {
        // Suppose you don't need padding, so just take channelData.cols() as is
        mPaddedLength = channelData.cols();
        mFftOutputSize = (mPaddedLength / 2) + 1;

        mSavedFFTs = Eigen::MatrixXcf::Zero(mFftOutputSize, mNumChannels);

        mForwardFftPlan = fftwf_plan_many_dft_r2c(
            1, &mPaddedLength, mNumChannels, channelData.data(), nullptr, mNumChannels, 1,
            reinterpret_cast<fftwf_complex*>(mSavedFFTs.data()), nullptr, 1, mFftOutputSize, FFTW_ESTIMATE);
    }

    ~FrequencyDomainNoFilterStrategy()
    {
        if (mForwardFftPlan) fftwf_destroy_plan(mForwardFftPlan);
    }

    void apply() override { fftwf_execute(mForwardFftPlan); }
    int getPaddedLength() const override { return mPaddedLength; }
    Eigen::MatrixXcf& getFrequencyDomainData() override { return mSavedFFTs; }

   private:
    int mNumChannels;
    int mPaddedLength;
    int mFftOutputSize;
    fftwf_plan mForwardFftPlan = nullptr;
    Eigen::MatrixXcf mSavedFFTs;
};

class IFrequencyDomainStrategyFactory
{
   public:
    static std::unique_ptr<IFrequencyDomainStrategy> create(
        const std::string& frequencyDomainStrategy, const std::string& filterWeightsPath,
        Eigen::MatrixXf& channelData,  // pass by ref, so it can be resized if needed
        int numChannels)
    {
        if (frequencyDomainStrategy == "None")
        {
            return std::make_unique<FrequencyDomainNoFilterStrategy>(channelData, numChannels);
        }
        else if (frequencyDomainStrategy == "Filter")
        {
            return std::make_unique<FrequencyDomainFilterStrategy>(filterWeightsPath, channelData, numChannels);
        }
        else
        {
            throw std::invalid_argument("Unknown frequency domain strategy: " + frequencyDomainStrategy);
        }
    }
};