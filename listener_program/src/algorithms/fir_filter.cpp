#include "fir_filter.h"

Eigen::MatrixXcf FrequencyDomainStrategy::mSavedFFTs; // frequency domain data (outputs)

void FrequencyDomainStrategy::initialize(int fftOutputSize)
{
    mSavedFFTs = Eigen::MatrixXcf::Zero(fftOutputSize, mNumChannels);
}

FrequencyDomainStrategy::FrequencyDomainStrategy(const std::string &filterWeightsPath, Eigen::MatrixXf &channelData, int channelSize, int numChannels)
    : mNumChannels(numChannels)

{
    // Load FIR filter weights
    std::vector<float> filterWeights = readFirFilterFile(filterWeightsPath);
    // std::cout << "filterWeightsSize: " << filterWeights.size() << " chan size: " << channelSize << std::endl;

    // calcualate the padded length needed for linear convolution
    mPaddedLength = filterWeights.size() + channelSize - 1;

    int fftOutputSize = (mPaddedLength / 2) + 1;
    FrequencyDomainStrategy::initialize(fftOutputSize);
    mFilterFreq.resize(fftOutputSize);

    std::cout << "Padded size: " << mPaddedLength << std::endl;
    initializeFilterWeights(filterWeights);
    channelData.resize(mPaddedLength, numChannels);
    // Create forward FFT plan for channel data
    mForwardFftPlan = fftwf_plan_many_dft_r2c(
        1,                                                    // Rank of the transform (1D)
        &mPaddedLength,                                       // Pointer to the size of the transform
        numChannels,                                          // Number of transforms (channels)
        channelData.data(),                                   // Input data pointer
        nullptr,                                              // No embedding
        1,                                                    // Stride between successive elements in input
        mPaddedLength,                                        // Stride between successive channels in input
        reinterpret_cast<fftwf_complex *>(mSavedFFTs.data()), // Output data pointer
        nullptr,                                              // No embedding
        1,                                                    // Stride between successive elements in output
        fftOutputSize,                                        // Stride between successive channels in output
        FFTW_ESTIMATE);                                       // Flag to measure and optimize the plan
}

void FrequencyDomainStrategy::apply()
{
    if (!mForwardFftPlan)
    {
        throw std::runtime_error("Error: FFTW plan is not initialized.");
    }
    fftwf_execute(mForwardFftPlan);

    
    for (int channelIndex = 0; channelIndex < mNumChannels; channelIndex++)
    {
        mSavedFFTs.col(channelIndex) = mSavedFFTs.col(channelIndex).array() * mFilterFreq.array();
    }
    
}

auto FrequencyDomainStrategy::readFirFilterFile(const std::string &filePath) -> std::vector<float>
{
    std::ifstream inputFile(filePath);
    if (!inputFile.is_open())
    {
        throw std::ios_base::failure("Error: Unable to open filter file '" + filePath + "'.");
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
                float value = std::stof(token);
                filterCoefficients.push_back(value);
            }
            catch (const std::invalid_argument &e)
            {
                std::cerr << "Invalid numeric value: " << token << std::endl;
            }
        }
    }

    return filterCoefficients;
}

Eigen::MatrixXcf &FrequencyDomainStrategy::getFrequencyDomainData()
{
    return mSavedFFTs; // Return a reference to the original matrix
}

void FrequencyDomainStrategy::initializeFilterWeights(const std::vector<float> &filterWeights)
{
    // Zero-pad filter weights to the required length
    std::vector<float> paddedFilterWeights(mPaddedLength, 0.0f);
    std::copy(filterWeights.begin(), filterWeights.end(), paddedFilterWeights.begin());

    // Create frequency-domain filter
    fftwf_plan fftFilter = fftwf_plan_dft_r2c_1d(
        mPaddedLength,
        paddedFilterWeights.data(),
        reinterpret_cast<fftwf_complex *>(mFilterFreq.data()),
        FFTW_ESTIMATE);
    fftwf_execute(fftFilter);
    fftwf_destroy_plan(fftFilter);
}
