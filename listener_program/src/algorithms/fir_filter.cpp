#include "fir_filter.h"

FrequencyDomainFilterStrategy::FrequencyDomainFilterStrategy(const std::string& filterPath,
                                                             Eigen::MatrixXf& channelData, int numChannels)
    : mNumChannels(numChannels)
{
    // 1) Read filter from file, compute final length
    auto filterWeights = readFirFilterFile(filterPath);
    mPaddedLength = static_cast<int>(filterWeights.size() + channelData.cols() - 1);
    mFftOutputSize = (mPaddedLength / 2) + 1;

    // 2) Resize the user's channelData to match the new padded length
    //    Make sure we are resizing columns if channels are rows or vice versa
    //    For this example, assume channelData has shape (numChannels, originalLength)
    channelData.conservativeResize(channelData.rows(), mPaddedLength);
    channelData.setZero();

    // 3) Allocate our frequency-domain buffer
    mSavedFFTs = Eigen::MatrixXcf::Zero(mFftOutputSize, mNumChannels);

    // 4) Create the filter in frequency domain
    initializeFilterWeights(filterWeights);

    // 5) Create the FFT plan once
    createFftPlan(channelData);
}

FrequencyDomainFilterStrategy::~FrequencyDomainFilterStrategy()
{
    if (mForwardFftPlan)
    {
        fftwf_destroy_plan(mForwardFftPlan);
        mForwardFftPlan = nullptr;
    }
}

void FrequencyDomainFilterStrategy::apply()
{
    // std::cout << "apply addr mSavedFFTs: " << mSavedFFTs.data() << std::endl;
    fftwf_execute(mForwardFftPlan);
    mBeforeFilter = mSavedFFTs;
    for (int channelIndex = 0; channelIndex < mNumChannels; ++channelIndex)
    {
        mSavedFFTs.col(channelIndex) = mSavedFFTs.col(channelIndex).array() * mFilterFreq.array();
    }
}

int FrequencyDomainFilterStrategy::getPaddedLength() const { return mPaddedLength; }

Eigen::MatrixXcf& FrequencyDomainFilterStrategy::getFrequencyDomainData() { return mSavedFFTs; }

void FrequencyDomainFilterStrategy::createFftPlan(Eigen::MatrixXf& channelData)
{
    // channelData now has the final size we need
    mForwardFftPlan = fftwf_plan_many_dft_r2c(1, &mPaddedLength, mNumChannels, channelData.data(), nullptr,
                                              mNumChannels, 1, reinterpret_cast<fftwf_complex*>(mSavedFFTs.data()),
                                              nullptr, 1, mFftOutputSize, FFTW_ESTIMATE);
}

void FrequencyDomainFilterStrategy::initializeFilterWeights(const std::vector<float>& filterWeights)
{
    mFilterFreq.resize(mFftOutputSize);
    std::vector<float> paddedFilter(mPaddedLength, 0.0f);
    std::copy(filterWeights.begin(), filterWeights.end(), paddedFilter.begin());

    fftwf_plan fftFilter = fftwf_plan_dft_r2c_1d(mPaddedLength, paddedFilter.data(),
                                                 reinterpret_cast<fftwf_complex*>(mFilterFreq.data()), FFTW_ESTIMATE);
    fftwf_execute(fftFilter);
    fftwf_destroy_plan(fftFilter);
}

std::vector<float> FrequencyDomainFilterStrategy::readFirFilterFile(const std::string& filePath)
{
    std::ifstream inputFile(filePath);
    if (!inputFile.is_open())
    {
        throw std::runtime_error("Unable to open filter file: " + filePath);
    }

    std::vector<float> filterCoefficients;
    std::string line;
    while (std::getline(inputFile, line))
    {
        std::stringstream lineStream(line);
        std::string token;
        while (std::getline(lineStream, token, ','))
        {
            try
            {
                filterCoefficients.push_back(std::stof(token));
            } catch (...)
            {
                // ...
            }
        }
    }
    return filterCoefficients;
}