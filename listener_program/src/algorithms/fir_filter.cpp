#include "fir_filter.h"

Eigen::MatrixXcf FrequencyDomainFilterStrategy::mSavedFFTs;  // frequency domain data (outputs)

void FrequencyDomainFilterStrategy::initialize(int fftOutputSize)
{
    mSavedFFTs = Eigen::MatrixXcf::Zero(fftOutputSize, mNumChannels);
}

FrequencyDomainFilterStrategy::FrequencyDomainFilterStrategy(const std::string& filterWeightsPath, int channelSize,
                                                             int numChannels)
    : mNumChannels(numChannels)

{
    std::vector<float> filterWeights = readFirFilterFile(filterWeightsPath);

    // padded length needed for circular convolution to be same as linear convolution
    mPaddedLength = filterWeights.size() + channelSize - 1;

    mFftOutputSize = (mPaddedLength / 2) + 1;
    FrequencyDomainFilterStrategy::initialize(mFftOutputSize);

    initializeFilterWeights(filterWeights);
}

/**
 * @brief class destructor: ensure that the memory allocated for the FFTW forward plan is safely and correctly released
 **/
FrequencyDomainFilterStrategy::~FrequencyDomainFilterStrategy()
{
    if (mForwardFftPlan)
    {
        fftwf_destroy_plan(mForwardFftPlan);  // Release the FFTW plan resources
        mForwardFftPlan = nullptr;            // Avoid dangling pointer
    }
}

void FrequencyDomainFilterStrategy::initialize(Eigen::MatrixXf& channelData)
{
    mForwardFftPlan =
        fftwf_plan_many_dft_r2c(1,                   // Rank of the transform (1D)
                                &mPaddedLength,      // Pointer to the size of the transform
                                mNumChannels,        // Number of transforms (channels)
                                channelData.data(),  // Input data pointer
                                nullptr,             // No embedding
                                mNumChannels,  // Input stride: elements of a single transform are spaced by numChannels
                                1,             // Input distance: distance between the start of consecutive transforms
                                reinterpret_cast<fftwf_complex*>(mSavedFFTs.data()),  // Output data pointer
                                nullptr,                                              // No embedding
                                1,                                                    // Output stride
                                mFftOutputSize,                                       // Output distance
                                FFTW_ESTIMATE  // Flag to measure and optimize the plan
        );
}

void FrequencyDomainFilterStrategy::apply()
{
    fftwf_execute(mForwardFftPlan);

    for (int channelIndex = 0; channelIndex < mNumChannels; channelIndex++)
    {
        mSavedFFTs.col(channelIndex) = mSavedFFTs.col(channelIndex).array() * mFilterFreq.array();
    }
}

auto FrequencyDomainFilterStrategy::readFirFilterFile(const std::string& filePath) -> std::vector<float>
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
            } catch (const std::invalid_argument& e)
            {
                std::cerr << "Invalid numeric value: " << token << std::endl;
            }
        }
    }

    return filterCoefficients;
}

int FrequencyDomainFilterStrategy::getPaddedLength()
{
    return mPaddedLength;  // Return a reference to the original matrix
}

Eigen::MatrixXcf& FrequencyDomainFilterStrategy::getFrequencyDomainData()
{
    return mSavedFFTs;  // Return a reference to the original matrix
}

void FrequencyDomainFilterStrategy::initializeFilterWeights(const std::vector<float>& filterWeights)
{
    mFilterFreq.resize(mFftOutputSize);

    // Zero-pad filter weights to the required length
    std::vector<float> paddedFilterWeights(mPaddedLength, 0.0f);
    std::copy(filterWeights.begin(), filterWeights.end(), paddedFilterWeights.begin());

    // Create frequency-domain filter
    fftwf_plan fftFilter = fftwf_plan_dft_r2c_1d(mPaddedLength, paddedFilterWeights.data(),
                                                 reinterpret_cast<fftwf_complex*>(mFilterFreq.data()), FFTW_ESTIMATE);
    fftwf_execute(fftFilter);
    fftwf_destroy_plan(fftFilter);
}

/*
No filter strategy
*/

Eigen::MatrixXcf FrequencyDomainNoFilterStrategy::mSavedFFTs;  // frequency domain data (outputs)

void FrequencyDomainNoFilterStrategy::initialize(int fftOutputSize)
{
    mSavedFFTs = Eigen::MatrixXcf::Zero(fftOutputSize, mNumChannels);
}

FrequencyDomainNoFilterStrategy::FrequencyDomainNoFilterStrategy(int channelSize, int numChannels)
    : mNumChannels(numChannels)

{
    // padded length needed for circular convolution to be same as linear convolution
    mPaddedLength = channelSize;
    mFftOutputSize = (mPaddedLength / 2) + 1;
    FrequencyDomainNoFilterStrategy::initialize(mFftOutputSize);
}

/**
 * @brief class destructor: ensure that the memory allocated for the FFTW forward plan is safely and correctly released
 **/
FrequencyDomainNoFilterStrategy::~FrequencyDomainNoFilterStrategy()
{
    if (mForwardFftPlan)
    {
        fftwf_destroy_plan(mForwardFftPlan);  // Release the FFTW plan resources
        mForwardFftPlan = nullptr;            // Avoid dangling pointer
    }
}

void FrequencyDomainNoFilterStrategy::initialize(Eigen::MatrixXf& channelData)
{
    mForwardFftPlan =
        fftwf_plan_many_dft_r2c(1,                   // Rank of the transform (1D)
                                &mPaddedLength,      // Pointer to the size of the transform
                                mNumChannels,        // Number of transforms (channels)
                                channelData.data(),  // Input data pointer
                                nullptr,             // No embedding
                                mNumChannels,  // Input stride: elements of a single transform are spaced by numChannels
                                1,             // Input distance: distance between the start of consecutive transforms
                                reinterpret_cast<fftwf_complex*>(mSavedFFTs.data()),  // Output data pointer
                                nullptr,                                              // No embedding
                                1,                                                    // Output stride
                                mFftOutputSize,                                       // Output distance
                                FFTW_ESTIMATE  // Flag to measure and optimize the plan
        );
}
void FrequencyDomainNoFilterStrategy::apply() { fftwf_execute(mForwardFftPlan); }

Eigen::MatrixXcf& FrequencyDomainNoFilterStrategy::getFrequencyDomainData()
{
    return mSavedFFTs;  // Return a reference to the original matrix
}

int FrequencyDomainNoFilterStrategy::getPaddedLength()
{
    return mPaddedLength;  // Return a reference to the original matrix
}