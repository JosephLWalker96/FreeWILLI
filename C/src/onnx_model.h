#pragma once

#include <string>
#include <vector>
#include <onnxruntime_cxx_api.h>

// Class to encapsulate ONNX model loading and inference
class ONNXModel {
public:
    ONNXModel(const std::string& model_path, const std::string& scaler_params_path);
    std::vector<int64_t> get_input_node_info();
    void load_scaler_params(const std::string& file_path);
    std::vector<float> run_inference(std::vector<float>& input_tensor_values);
    void normalize_data(std::vector<float>& data) const;

private:
    Ort::SessionOptions session_options;
    Ort::Session session{nullptr};
    std::vector<int64_t> input_node_dims;
    std::vector<std::string> input_node_names;
    std::vector<const char*> cstr_vec;
    std::vector<float> mean, scale;
};