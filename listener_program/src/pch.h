#pragma once
// Standard C++ Library Headers
#include <atomic>
#include <chrono>
#include <cmath>
#include <cstdlib>
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
#include <string>
#include <thread>
#include <tuple>
#include <vector>

// System Headers
#include <arpa/inet.h>
#include <fftw3.h>
#include <netinet/in.h>
#include <onnxruntime_cxx_api.h>
#include <sys/socket.h>
#include <unistd.h>

#include <eigen3/Eigen/Dense>
#include <nlohmann/json.hpp>  // Use a JSON library to load JSON data

#ifdef __ARM_NEON
#include <arm_neon.h>  // Include NEON intrinsics
#endif