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

class IFrequencyDomainDetectorFactory
{
   public:
    static std::unique_ptr<IFrequencyDomainDetector> create(const PipelineVariables& pipelineVariables)
    {
        if (pipelineVariables.frequencyDomainDetector == "AverageEnergy")
        {
            return std::make_unique<AverageMagnitudeDetector>(pipelineVariables.energyDetectionThreshold);
        } else if (pipelineVariables.frequencyDomainDetector == "None")
        {
            return std::make_unique<NoFrequencyDomainDetector>();
        } else
        {
            throw std::invalid_argument("Unknown TimeDomainDetector type: " +
                                        pipelineVariables.frequencyDomainDetector);
        }
    }
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