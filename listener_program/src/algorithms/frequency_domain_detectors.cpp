#include "../pch.h"
#include "frequency_domain_detectors.h"

AverageAmplitudeDetector::AverageAmplitudeDetector(float threshold) : detectionThreshold(threshold) {}

bool AverageAmplitudeDetector::detect(const Eigen::VectorXcf &frequencyDomainData) const {
    // Compute the average amplitude of the frequency-domain data
    float averageAmplitude = frequencyDomainData.array().abs().sum() / frequencyDomainData.size();

    // Return true if the average amplitude meets or exceeds the threshold
    return averageAmplitude >= detectionThreshold;
}