#include "../pch.h"
#include "threshold_detectors.h"

using TimePoint = std::chrono::system_clock::time_point;

/**
 * @brief Detects peaks in the time-domain input data that exceed a specified threshold.
 *
 * This function analyzes the time-domain input data to detect peaks above the given threshold.
 * If a peak is detected, it records the peak index, amplitude, and the corresponding timestamp
 * in a `DetectionResult` structure.
 *
 * @param timeDomainData A reference to an Eigen vector containing the input time-domain data.
 * @param timestamps A span of TimePoint objects representing the timestamps corresponding to the input data.
 * @param detectionThreshold The threshold value above which peaks are detected.
 * @param sampleRate The sample rate of the input data in samples per second.
 * @return A `DetectionResult` structure containing information about the detected peak, or an empty result if no peak is detected.
 */
DetectionResult detectTimeDomainThreshold(const Eigen::VectorXf &timeDomainData, const std::span<TimePoint> timestamps,
                                          const float &detectionThreshold, const int &sampleRate)
{
    DetectionResult result{};
    int peakIndex = 0;

    // Find the peak amplitude and its index in the data
    float peakAmplitude = timeDomainData.maxCoeff(&peakIndex);

    if (peakAmplitude >= detectionThreshold)
    {
        result.minPeakIndex = peakIndex;
        result.maxPeakIndex = peakIndex;
        result.peakAmplitude = peakAmplitude;

        // Compute the timestamp for the detected peak
        unsigned int microseconds = static_cast<unsigned int>(peakIndex * (1e6 / sampleRate));
        auto peakTime = timestamps[0] + std::chrono::microseconds(microseconds);
        result.peakTimes = peakTime;
    }
    return result;
}

/**
 * @brief Detects peaks in the frequency-domain input data that exceed a specified threshold.
 *
 * This function analyzes the frequency-domain input data to detect energy levels that exceed
 * the given threshold. If a significant peak is detected, it records the peak amplitude and
 * relevant indices in a `DetectionResult` structure.
 *
 * @param frequencyDomainData A reference to an Eigen vector containing the input frequency-domain data.
 * @param timestamps A span of TimePoint objects representing the timestamps corresponding to the input data.
 * @param detectionThreshold The threshold value above which energy peaks are detected.
 * @param sampleRate The sample rate of the input data in samples per second.
 * @return A `DetectionResult` structure containing information about the detected peak, or an empty result if no peak is detected.
 */
DetectionResult detectFrequencyDomainThreshold(const Eigen::VectorXcf &frequencyDomainData, const std::span<TimePoint> timestamps,
                                               const float &detectionThreshold, const int &sampleRate)
{
    DetectionResult result{};

    // Compute the average amplitude of the frequency-domain data
    int peakIndex = 0; // Placeholder for further peak index refinement, if needed
    float averageAmplitude = frequencyDomainData.array().abs().sum() / frequencyDomainData.size();

    if (averageAmplitude >= detectionThreshold)
    {
        result.minPeakIndex = peakIndex;
        result.maxPeakIndex = peakIndex;
        result.peakAmplitude = averageAmplitude;
    }
    return result;
}