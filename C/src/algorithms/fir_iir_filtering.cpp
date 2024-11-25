#include "fir_iir_filtering.h"

/**
 * @brief Performs frequency domain FIR filtering on multi-channel time-domain data.
 *
 * This function applies a frequency domain FIR filter to multi-channel time-domain data
 * that has been zero-padded. It transforms the input data into the frequency domain using
 * FFT, applies the filter, and stores the filtered frequency domain data.
 *
 * @param timeDomainChannelData A matrix containing the zero-padded time-domain data for all channels.
 *                              Each column represents a single channel.
 * @param frequencyDomainFilter A vector representing the FIR filter in the frequency domain.
 * @param forwardFftPlan An FFTW plan used to compute the forward FFT of the input data.
 * @param filteredFrequencyData A matrix to store the filtered frequency domain data for all channels.
 */
void performFrequencyDomainFIRFiltering(
    const Eigen::MatrixXf &timeDomainChannelData,
    const Eigen::VectorXcf &frequencyDomainFilter,
    fftwf_plan &forwardFftPlan,
    Eigen::MatrixXcf &filteredFrequencyData)
{
    int numChannels = timeDomainChannelData.cols();

    // Perform FFT on the input time-domain data
    fftwf_execute(forwardFftPlan);

    // Apply the frequency domain filter to each channel
    for (int channelIndex = 0; channelIndex < numChannels; channelIndex++)
    {
        filteredFrequencyData.col(channelIndex) = filteredFrequencyData.col(channelIndex).array() * frequencyDomainFilter.array();
    }
}
