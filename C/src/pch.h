#pragma once
// Standard C++ Library Headers
#include <vector>
#include <iostream>
#include <string>
#include <chrono>
#include <queue>
#include <mutex>
#include <atomic>
#include <stdexcept>
#include <cstdlib>
#include <thread>
#include <cmath>
#include <random>
#include <sstream>
#include <fstream>
#include <iomanip>
#include <span>
#include <tuple>
#include <set>
#include <optional>

// System Headers
#include <netinet/in.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>

#include <eigen3/Eigen/Dense>
#include <fftw3.h>
// #include <liquid/liquid.h>

#include <onnxruntime_cxx_api.h>
#include <nlohmann/json.hpp> // Use a JSON library to load JSON data

#ifdef __ARM_NEON
#include <arm_neon.h> // Include NEON intrinsics
#endif