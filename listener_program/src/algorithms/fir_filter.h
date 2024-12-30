#pragma once
//#include "../main_utils.h"
#include "../pch.h"

class IFrequencyDomainStrategy
{
   public:
    virtual ~IFrequencyDomainStrategy() = default;

    virtual void initialize(Eigen::MatrixXf& channelData) = 0;
    virtual void apply() = 0;
    virtual int getPaddedLength() = 0;

    virtual Eigen::MatrixXcf& getFrequencyDomainData() = 0;
};

class FrequencyDomainFilterStrategy : public IFrequencyDomainStrategy
{
   public:
    FrequencyDomainFilterStrategy(const std::string& filename, int channelSize, int numChannels);
    ~FrequencyDomainFilterStrategy();
    Eigen::MatrixXcf& getFrequencyDomainData() override;

    int getPaddedLength() override;
    void initialize(Eigen::MatrixXf& channelData) override;
    void apply() override;

   private:
    void initializeFilterWeights(const std::vector<float>& filterWeights);
    auto readFirFilterFile(const std::string& filePath) -> std::vector<float>;
    void initialize(int fftOutputSize);

    static Eigen::MatrixXcf mSavedFFTs;  // frequency domain data (outputs)
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
    Eigen::MatrixXcf& getFrequencyDomainData() override;

    int getPaddedLength() override;
    void initialize(Eigen::MatrixXf& channelData) override;
    void apply() override;

   private:
    void initialize(int fftOutputSize);

    static Eigen::MatrixXcf mSavedFFTs;  // frequency domain data (outputs)
    int mNumChannels;
    int mPaddedLength;
    int mFftOutputSize;
    Eigen::VectorXcf mFilterFreq;
    fftwf_plan mForwardFftPlan = nullptr;
};

class IFrequencyDomainStrategyFactory
{
   public:
    static std::unique_ptr<IFrequencyDomainStrategy> create(const std::string& frequencyDomainStrategy,
                                                            const std::string& filterWeightsPath, int channelSize,
                                                            int numChannel)
    {
        if (frequencyDomainStrategy == "None")
        {
            return std::make_unique<FrequencyDomainNoFilterStrategy>(channelSize, numChannel);
        } else if (frequencyDomainStrategy == "Filter")
        {
            return std::make_unique<FrequencyDomainFilterStrategy>(filterWeightsPath, channelSize, numChannel);
        } else
        {
            throw std::invalid_argument("Unknown frequency domain strategy: " + std::string(frequencyDomainStrategy));
        }
    }
};