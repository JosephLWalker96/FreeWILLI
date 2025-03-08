#pragma once
// Standard C++ Library Headers
#include <atomic>
#include <chrono>
#include <cmath>
#include <cstdlib>
#include <cstring>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <mutex>
#include <optional>
#include <queue>
#include <random>
#include <set>
#include <span>
#include <sstream>
#include <stdexcept>
#include <cstdint>
#include <string>
#include <thread>
#include <tuple>
#include <vector>

// System Headers
// #include <arpa/inet.h>   // Not used in MCU env
// #include <fftw3.h>       // YJ// Ignoring for now
// #include <netinet/in.h>  // Not used in MCU env
// #include <onnxruntime_cxx_api.h>
// #include <sys/socket.h>  // Not used in MCU env
#include <unistd.h>

// #include "Eigen/Dense"  // YJ// Ignoring for now
#include <ArduinoEigen.h>
#include <ArduinoEigenDense.h>
// #include <nlohmann/json.hpp>  // Use a JSON library to load JSON data

using TimePoint = std::chrono::system_clock::time_point;

// #ifdef __ARM_NEON
// #include <arm_neon.h>  // Include NEON intrinsics
// #endif