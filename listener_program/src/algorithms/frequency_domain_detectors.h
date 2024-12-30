#pragma once
#include "../pch.h"

// Interface definition
class IFrequencyDomainDetector
{
   public:
    virtual ~IFrequencyDomainDetector() = default;

    // Pure virtual method to be implemented by derived classes
    virtual bool detect(const Eigen::VectorXcf& frequencyDomainData) const = 0;
};

class AverageMagnitudeDetector : public IFrequencyDomainDetector
{
   private:
    float detectionThreshold;

   public:
    // Constructor to initialize detectionThreshold
    explicit AverageMagnitudeDetector(float threshold);

    // Override the detect method
    bool detect(const Eigen::VectorXcf& frequencyDomainData) const override;
};

class NoFrequencyDomainDetector : public IFrequencyDomainDetector
{
   private:
    float detectionThreshold;

   public:
    // Constructor to initialize detectionThreshold
    explicit NoFrequencyDomainDetector();

    // Override the detect method
    bool detect(const Eigen::VectorXcf& frequencyDomainData) const override;
};

class IFrequencyDomainDetectorFactory
{
   public:
    static std::unique_ptr<IFrequencyDomainDetector> create(const std::string& frequencyDomainDetector,
                                                            float energyDetectionThreshold)
    {
        if (frequencyDomainDetector == "AverageEnergy")
        {
            return std::make_unique<AverageMagnitudeDetector>(energyDetectionThreshold);
        } else if (frequencyDomainDetector == "None")
        {
            return std::make_unique<NoFrequencyDomainDetector>();
        } else
        {
            throw std::invalid_argument("Unknown TimeDomainDetector type: " + frequencyDomainDetector);
        }
    }
};