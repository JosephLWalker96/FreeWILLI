#include "onnx_model.h"
#include "onnx_model.h"
#include <iostream>
#include <fstream>
#include <nlohmann/json.hpp> // Use a JSON library to load JSON data

/**
 * @brief Constructs the ONNXModel object, initializes the ONNX session, and loads the scaler parameters.
 * @param modelPath Path to the ONNX model file.
 * @param scalerParamsPath Path to the JSON file containing scaling parameters.
 */
ONNXModel::ONNXModel(const std::string &modelPath, const std::string &scalerParamsPath)
{
    std::cout << "Initializing ONNX model" << std::endl;

    Ort::Env env(ORT_LOGGING_LEVEL_WARNING, "ONNXRuntime");
    mSessionOptions.SetIntraOpNumThreads(1);
    mSessionOptions.SetGraphOptimizationLevel(GraphOptimizationLevel::ORT_ENABLE_EXTENDED);
    mSession = Ort::Session(env, modelPath.c_str(), mSessionOptions);

    mInputNodeDims = getInputNodeInfo();
    loadScalerParams(scalerParamsPath);

    for (const std::string &str : mInputNodeNames)
    {
        mCstrVec.push_back(str.c_str());
    }
}

/**
 * @brief Retrieves input node dimensions and names from the ONNX model.
 * @return A vector of integers representing the dimensions of the input node.
 */
std::vector<int64_t> ONNXModel::getInputNodeInfo()
{
    Ort::AllocatorWithDefaultOptions allocator;
    size_t numInputNodes = mSession.GetInputCount();
    std::vector<int64_t> inputDims;

    for (size_t i = 0; i < numInputNodes; i++)
    {
        auto inputName = mSession.GetInputNameAllocated(i, allocator);
        mInputNodeNames.push_back(inputName.get());

        Ort::TypeInfo typeInfo = mSession.GetInputTypeInfo(i);
        auto tensorInfo = typeInfo.GetTensorTypeAndShapeInfo();
        inputDims = tensorInfo.GetShape();
    }

    return inputDims;
}

/**
 * @brief Loads scaling parameters (mean and scale) from a JSON file.
 * @param filePath Path to the JSON file containing scaling parameters.
 * @throws std::runtime_error If the file cannot be opened.
 */
void ONNXModel::loadScalerParams(const std::string &filePath)
{
    std::ifstream file(filePath);
    if (!file.is_open())
    {
        throw std::runtime_error("Error opening scaler params file.");
    }

    nlohmann::json scalerJson;
    file >> scalerJson;

    mMean = scalerJson["mean"].get<std::vector<float>>();
    mScale = scalerJson["scale"].get<std::vector<float>>();
}

/**
 * @brief Runs inference on the provided input tensor values.
 * @param inputTensorValues A vector of input tensor values.
 * @return A vector of output tensor values (inference result).
 */
std::vector<float> ONNXModel::runInference(std::vector<float> &inputTensorValues)
{
    Ort::MemoryInfo memoryInfo = Ort::MemoryInfo::CreateCpu(OrtArenaAllocator, OrtMemTypeDefault);

    normalizeData(inputTensorValues);

    Ort::Value inputTensor = Ort::Value::CreateTensor<float>(
        memoryInfo, const_cast<float *>(inputTensorValues.data()), inputTensorValues.size(),
        mInputNodeDims.data(), mInputNodeDims.size());

    const char *outputNodeNames[] = {"output"};
    auto outputTensors = mSession.Run(
        Ort::RunOptions{nullptr}, mCstrVec.data(), &inputTensor, 1, outputNodeNames, 1);

    float *outputData = outputTensors[0].GetTensorMutableData<float>();
    auto outputTensorInfo = outputTensors[0].GetTensorTypeAndShapeInfo();
    std::vector<int64_t> outputShape = outputTensorInfo.GetShape();
    int numClasses = outputShape[1];

    return std::vector<float>(outputData, outputData + numClasses);
}

/**
 * @brief Normalizes the input data using the loaded scaling parameters.
 * @param data A vector of data values to be normalized.
 */
void ONNXModel::normalizeData(std::vector<float> &data) const
{
    for (size_t i = 0; i < data.size(); ++i)
    {
        data[i] = (data[i] - mMean[i]) / mScale[i];
    }
}

/**
 * @brief Returns an example echolocation click spectra as a vector.
 * @return A vector containing an example input tensor for the ONNX model.
 */
auto getExampleClick() -> std::vector<float>
{
    std::vector<float> input_tensor_values = {
        92.1948, 96.1963, 101.122, 104.773, 107.151, 108.565, 109.293, 109.541, 109.451, 109.168,
        108.921, 108.999, 109.475, 110.056, 110.395, 110.349, 109.977, 109.466, 109.018, 108.68,
        108.28, 107.571, 106.516, 105.596, 105.651, 106.584, 107.503, 108.037, 108.182, 107.898,
        107.157, 105.957, 104.561, 103.774, 104.144, 104.945, 105.38, 105.203, 104.43, 103.139,
        101.439, 99.5627, 97.9943, 97.3363, 97.4403, 97.6794, 97.7522, 97.6871, 97.5253, 97.2252,
        96.8271, 96.475, 96.1248, 95.6222, 94.9328, 94.1597, 93.6609, 93.7281, 94.0786, 94.1562,
        93.6164, 92.3879, 90.4895, 88.4291, 87.6331, 88.309, 88.8974, 88.7232, 88.0315, 87.4041,
        86.8748, 85.9485, 84.4688, 82.9816, 81.7751, 79.9937, 78.3869, 79.4843, 80.6382, 80.135,
        78.6906, 78.3676, 80.49, 83.8795, 86.785, 88.5948, 89.3266, 89.1901, 88.2955, 86.7553,
        84.8847, 83.6774, 84.5187, 86.541, 88.1887, 89.0825, 89.3545, 89.3343, 89.3493, 89.5171,
        89.751, 89.7528, 89.2266, 88.0493, 86.4237, 85.5187, 86.7017, 88.6813, 90.1444, 90.944,
        91.2794, 91.3301, 91.2617, 91.0691, 90.6381, 89.8405, 88.4528, 86.3709, 84.3802, 84.9515,
        87.2957, 89.235, 90.4691, 91.233, 91.6813, 91.9263, 92.2548, 92.8246, 93.2115, 92.905,
        91.6683, 89.3315, 85.8135, 81.2775, 76.8763, 77.3899, 81.2361, 84.1679, 85.5111, 85.1557,
        82.9623, 81.2194, 84.8278, 88.5298, 90.6789, 91.8252, 92.4424, 92.7654, 92.9029, 92.9495,
        92.9146, 92.6751, 92.1435, 91.2272, 90.0295, 89.3202, 89.9183, 91.1307, 91.9888, 92.2035,
        91.9298, 91.4551, 91.174, 91.2775, 91.702, 92.2776, 92.9613, 93.6848, 94.2458, 94.4455,
        94.2079, 93.6266, 92.9577, 92.4459, 92.1938, 92.2134, 92.6957, 93.7765, 95.1637, 96.4559,
        97.3677};
    return input_tensor_values;
}