#pragma once
#include "../pch.h"

using TimePoint = std::chrono::system_clock::time_point;

struct DetectionResult
{
    int minPeakIndex = -1;
    int maxPeakIndex = -1;
    TimePoint peakTimes;
    float peakAmplitude;
};

DetectionResult detectTimeDomainThreshold(const Eigen::VectorXf &timeDomainData, const std::span<TimePoint> timestamps,
                                          const float &detectionThreshold, const int &sampleRate);

DetectionResult detectFrequencyDomainThreshold(const Eigen::VectorXcf &frequencyDomainData, const std::span<TimePoint> timestamps,
                                               const float &detectionThreshold, const int &sampleRate);
