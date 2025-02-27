#include "fir_filter.h"

FrequencyDomainFilterStrategy::FrequencyDomainFilterStrategy(
    const std::string& filterPath, Eigen::MatrixXf& channelData, int numChannels)
    : mNumChannels(numChannels)
{
    // 1) Read filter from file, compute final length
    auto filterWeights = readFirFilterFile(filterPath);

    mPaddedLength = static_cast<int>(filterWeights.size() + channelData.cols() - 1);
    mFftOutputSize = (mPaddedLength / 2) + 1;

    // 2) Resize the user's channelData to match the new padded length
    //    Make sure we are resizing columns if channels are rows or vice versa
    //    For this example, assume channelData has shape (numChannels,
    //    originalLength)
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
    mForwardFftPlan = fftwf_plan_many_dft_r2c(
        1, &mPaddedLength, mNumChannels, channelData.data(), nullptr, mNumChannels, 1,
        reinterpret_cast<fftwf_complex*>(mSavedFFTs.data()), nullptr, 1, mFftOutputSize, FFTW_ESTIMATE);
}

void FrequencyDomainFilterStrategy::initializeFilterWeights(const std::vector<float>& filterWeights)
{
    mFilterFreq.resize(mFftOutputSize);
    std::vector<float> paddedFilter(mPaddedLength, 0.0f);
    std::copy(filterWeights.begin(), filterWeights.end(), paddedFilter.begin());

    fftwf_plan fftFilter = fftwf_plan_dft_r2c_1d(
        mPaddedLength, paddedFilter.data(), reinterpret_cast<fftwf_complex*>(mFilterFreq.data()), FFTW_ESTIMATE);
    fftwf_execute(fftFilter);
    fftwf_destroy_plan(fftFilter);
}

std::vector<float> FrequencyDomainFilterStrategy::readFirFilterFile(const std::string& filePath)
{
    /*
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
            }
            catch (...)
            {
                // ...
            }
        }
    }
    return filterCoefficients;
    */
    std::vector<float> taps = {
        1.74828306e-18,  5.00045521e-04,  3.26087846e-04,  -3.51028225e-04, -6.21824599e-04, 3.01953952e-18,
        7.72954111e-04,  5.38776400e-04,  -6.10068855e-04, -1.11984343e-03, -3.92734007e-18, 1.44093584e-03,
        1.00811040e-03,  -1.13872261e-03, -2.07602193e-03, 2.34343683e-17,  2.61441207e-03,  1.80583511e-03,
        -2.01286756e-03, -3.62103679e-03, -3.93142381e-17, 4.44449137e-03,  3.03346478e-03,  -3.34367192e-03,
        -5.95336041e-03, 5.04536926e-18,  7.17840475e-03,  4.86350735e-03,  -5.32739986e-03, -9.43708737e-03,
        -1.59606293e-17, 1.13061549e-02,  7.65155827e-03,  -8.38513248e-03, -1.48861921e-02, 1.05360099e-16,
        1.80237354e-02,  1.23082365e-02,  -1.36526346e-02, -2.46247806e-02, -2.13420798e-17, 3.12395004e-02,
        2.20646149e-02,  -2.55674894e-02, -4.88431839e-02, 5.33652866e-17,  7.46162882e-02,  6.18804948e-02,
        -9.32437971e-02, -3.02566854e-01, 6.00220103e-01,  -3.02566854e-01, -9.32437971e-02, 6.18804948e-02,
        7.46162882e-02,  5.33652866e-17,  -4.88431839e-02, -2.55674894e-02, 2.20646149e-02,  3.12395004e-02,
        -2.13420798e-17, -2.46247806e-02, -1.36526346e-02, 1.23082365e-02,  1.80237354e-02,  1.05360099e-16,
        -1.48861921e-02, -8.38513248e-03, 7.65155827e-03,  1.13061549e-02,  -1.59606293e-17, -9.43708737e-03,
        -5.32739986e-03, 4.86350735e-03,  7.17840475e-03,  5.04536926e-18,  -5.95336041e-03, -3.34367192e-03,
        3.03346478e-03,  4.44449137e-03,  -3.93142381e-17, -3.62103679e-03, -2.01286756e-03, 1.80583511e-03,
        2.61441207e-03,  2.34343683e-17,  -2.07602193e-03, -1.13872261e-03, 1.00811040e-03,  1.44093584e-03,
        -3.92734007e-18, -1.11984343e-03, -6.10068855e-04, 5.38776400e-04,  7.72954111e-04,  3.01953952e-18,
        -6.21824599e-04, -3.51028225e-04, 3.26087846e-04,  5.00045521e-04,  1.74828306e-18};
    return taps;
}