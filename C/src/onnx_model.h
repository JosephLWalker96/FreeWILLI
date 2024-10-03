
#pragma once
#include <iostream>
#include <fstream>
#include <vector>
#include <string>
#include <onnxruntime_cxx_api.h>
#include <nlohmann/json.hpp>  // Use a JSON library to load JSON data


// Class to encapsulate ONNX model loading and inference
class ONNXModel {
public:
    
    ONNXModel(const std::string& model_path, const std::string& scaler_params_path) {
        // Initialize ONNX Runtime and create a session
        Ort::Env env(ORT_LOGGING_LEVEL_WARNING, "TestOnnxRuntime");
        session_options.SetIntraOpNumThreads(1);
        session_options.SetGraphOptimizationLevel(GraphOptimizationLevel::ORT_ENABLE_EXTENDED);
        session = Ort::Session(env, model_path.c_str(), session_options);

        // Get input node information
        input_node_dims = get_input_node_info();

        // Load scaler parameters (mean and scale)
        load_scaler_params(scaler_params_path);

        // Prepare input node names as const char*
        for (const std::string& str : input_node_names) {
            cstr_vec.push_back(str.c_str());
        }
    }
    std::vector<int64_t> get_input_node_info() {
        Ort::AllocatorWithDefaultOptions allocator;
        size_t num_input_nodes = session.GetInputCount();
        std::vector<int64_t> input_dims;

        for (size_t i = 0; i < num_input_nodes; i++) {
            auto var_ptr = session.GetInputNameAllocated(i, allocator);
            input_node_names.push_back(var_ptr.get());

            Ort::TypeInfo type_info = session.GetInputTypeInfo(i);
            auto tensor_info = type_info.GetTensorTypeAndShapeInfo();
            input_dims = tensor_info.GetShape();
        }

        return input_dims;
    }

    void load_scaler_params(const std::string& file_path) {
        std::ifstream file(file_path);
        if (!file.is_open()) {
            throw std::runtime_error("Error opening scaler params file.");
        }
        nlohmann::json scaler_json;
        file >> scaler_json;

        mean = scaler_json["mean"].get<std::vector<float>>();
        scale = scaler_json["scale"].get<std::vector<float>>();
    }

    std::vector<float> run_inference(std::vector<float>& input_tensor_values) {
        Ort::MemoryInfo memory_info = Ort::MemoryInfo::CreateCpu(OrtArenaAllocator, OrtMemTypeDefault);
        normalize_data(input_tensor_values);
        Ort::Value input_tensor = Ort::Value::CreateTensor<float>(
            memory_info, const_cast<float*>(input_tensor_values.data()), input_tensor_values.size(),
            input_node_dims.data(), input_node_dims.size()
        );

        const char* output_node_names[] = {"output"};
        auto output_tensors = session.Run(Ort::RunOptions{nullptr}, cstr_vec.data(), &input_tensor, 1, output_node_names, 1);

        float* output_data = output_tensors[0].GetTensorMutableData<float>();
        auto output_tensor_info = output_tensors[0].GetTensorTypeAndShapeInfo();
        std::vector<int64_t> output_shape = output_tensor_info.GetShape();
        int num_classes = output_shape[1]; // Assuming output shape is [batch_size, num_classes]

        return std::vector<float>(output_data, output_data + num_classes);
    }

    void normalize_data(std::vector<float>& data) const {
        for (size_t i = 0; i < data.size(); ++i) {
            data[i] = (data[i] - mean[i]) / scale[i];
        }
    }

private:
    Ort::SessionOptions session_options;
    Ort::Session session{nullptr};
    std::vector<int64_t> input_node_dims;
    std::vector<std::string> input_node_names;
    std::vector<const char*> cstr_vec;
    std::vector<float> mean, scale;
};