#pragma once
#include "../pch.h"
class FrequencyDomainDetector {
public:
    virtual ~FrequencyDomainDetector() = default;

    // Pure virtual method to be implemented by derived classes
    virtual bool detect(const Eigen::VectorXcf &frequencyDomainData) const = 0;
};

class AverageAmplitudeDetector : public FrequencyDomainDetector {
private:
    float detectionThreshold;

public:
    // Constructor to initialize detectionThreshold
    explicit AverageAmplitudeDetector(float threshold);

    // Override the detect method
    bool detect(const Eigen::VectorXcf &frequencyDomainData) const override;
};