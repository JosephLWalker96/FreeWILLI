#pragma once
// #include "../main_utils.h"
#include "../pch.h"

class IFrequencyDomainStrategy
{
   public:
    virtual ~IFrequencyDomainStrategy() = default;

    // We no longer need an 'initialize' method; everything can happen in the constructor
    virtual void apply() = 0;
    virtual int getPaddedLength() const = 0;

    virtual Eigen::MatrixXcf& getFrequencyDomainData() = 0;

    // If you still want to keep 'mBeforeFilter' accessible, make it protected or provide a getter
    Eigen::MatrixXcf mBeforeFilter;
};

class FrequencyDomainFilterStrategy : public IFrequencyDomainStrategy
{
   public:
    // Now we pass channelData by reference (or pointer).
    // The constructor itself will figure out filter length, resize channelData, and create plan.
    FrequencyDomainFilterStrategy(const std::string& filterPath, Eigen::MatrixXf& channelData, int numChannels);
    ~FrequencyDomainFilterStrategy();

    void apply() override;
    int getPaddedLength() const override;
    Eigen::MatrixXcf& getFrequencyDomainData() override;

   private:
    void initializeFilterWeights(const std::vector<float>& filterWeights);
    std::vector<float> readFirFilterFile(const std::string& filePath);
    void createFftPlan(Eigen::MatrixXf& channelData);

   private:
    int mNumChannels;
    int mPaddedLength;
    int mFftOutputSize;
    Eigen::VectorXcf mFilterFreq;

    // The FFT plan
    fftwf_plan mForwardFftPlan = nullptr;

    // We'll store the frequency-domain data in a non-static member
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

        mForwardFftPlan = fftwf_plan_many_dft_r2c(1, &mPaddedLength, mNumChannels, channelData.data(), nullptr,
                                                  mNumChannels, 1, reinterpret_cast<fftwf_complex*>(mSavedFFTs.data()),
                                                  nullptr, 1, mFftOutputSize, FFTW_ESTIMATE);
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
        } else if (frequencyDomainStrategy == "Filter")
        {
            return std::make_unique<FrequencyDomainFilterStrategy>(filterWeightsPath, channelData, numChannels);
        } else
        {
            throw std::invalid_argument("Unknown frequency domain strategy: " + frequencyDomainStrategy);
        }
    }
};