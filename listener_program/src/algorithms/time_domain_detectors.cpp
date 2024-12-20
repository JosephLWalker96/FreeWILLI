#include "../pch.h"
#include "time_domain_detectors.h"

PeakAmplitudeDetector::PeakAmplitudeDetector(float threshold) : detectionThreshold(threshold) {}

bool PeakAmplitudeDetector::detect(const Eigen::VectorXf &timeDomainData) const {
    int peakIndex = 0;

    // Find the peak amplitude and its index in the data
    float peakAmplitude = timeDomainData.maxCoeff(&peakIndex);

    // Return true if the peak amplitude meets or exceeds the threshold
    return peakAmplitude >= detectionThreshold;
}


float PeakAmplitudeDetector::getLastDetection() const{
    return lastDetection;
}