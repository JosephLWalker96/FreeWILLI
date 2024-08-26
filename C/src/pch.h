#ifndef PCH_H // Precompiled Headers
#define PCH_H

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

#ifdef PICO
#include <Eigen/Dense>
#include <fftw3.h>
#else
#include <eigen3/Eigen/Dense>
#include <fftw3.h>
//#include <liquid/liquid.h>
#endif


// System Headers
#ifdef PICO
#include "pico/stdlib.h"
#include "lwip/sockets.h"
#include <unistd.h>
#else
#include <netinet/in.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <Eigen/Dense>
#endif

// Third-Party Libraries
//#define EIGEN_VECTORIZE
//#define EIGEN_VECTORIZE_AVX
//#define EIGEN_VECTORIZE_AVX2

#endif //PCH_H
