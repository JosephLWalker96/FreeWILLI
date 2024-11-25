#pragma once
#include "../pch.h"

class ONNXModel
{
public:
    ONNXModel(const std::string &modelPath, const std::string &scalerParamsPath);
    std::vector<int64_t> getInputNodeInfo();
    void loadScalerParams(const std::string &filePath);
    std::vector<float> runInference(std::vector<float> &inputTensorValues);
    void normalizeData(std::vector<float> &data) const;

private:
    Ort::SessionOptions mSessionOptions;
    Ort::Session mSession{nullptr};
    std::vector<int64_t> mInputNodeDims;
    std::vector<std::string> mInputNodeNames;
    std::vector<const char *> mCstrVec;
    std::vector<float> mMean;
    std::vector<float> mScale;
};

auto getExampleClick() -> std::vector<float>;