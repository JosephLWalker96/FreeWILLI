cmake_minimum_required(VERSION 3.22.1)

project(HarpListen VERSION 1.0)

# Set the C++ standard
set(CMAKE_CXX_STANDARD 20)

# Ensure AVX vectorization is enabled globally
if(CMAKE_CXX_COMPILER_ID MATCHES "GNU" OR CMAKE_CXX_COMPILER_ID MATCHES "Clang")
    add_compile_options(-mavx -mavx2 -mfma)
elseif(CMAKE_CXX_COMPILER_ID MATCHES "MSVC")
    add_compile_options(/arch:AVX2)
endif()

add_library(MainLib STATIC process_data.cpp TDOA_estimation.cpp utils.cpp)

add_executable(HarpListen main.cpp)

# Set the output directory for the executable
set_target_properties(HarpListen PROPERTIES
  RUNTIME_OUTPUT_DIRECTORY ${PROJECT_SOURCE_DIR}/bin
)

# Link MainLib to HarpListen
target_link_libraries(HarpListen PRIVATE MainLib)

# LAPACK
find_package(LAPACK REQUIRED)
message(STATUS "LAPACK libraries: ${LAPACK_LIBRARIES}")
target_link_libraries(HarpListen PRIVATE ${LAPACK_LIBRARIES})

# Liquid
find_library(LIQUID_LIBRARIES NAMES liquid REQUIRED)
message(STATUS "LIQUID libraries: ${LIQUID_LIBRARIES}")
target_link_libraries(HarpListen PRIVATE ${LIQUID_LIBRARIES})

find_path(LIQUID_INCLUDE_DIRS NAMES liquid REQUIRED PATH_SUFFIXES /opt/homebrew/Cellar/)
message(STATUS "LIQUID include dirs: ${LIQUID_INCLUDE_DIRS}")
include_directories(${LIQUID_INCLUDE_DIRS})

# FFTW3
find_library(FFTW_LIBRARIES NAMES fftw3 REQUIRED)
message(STATUS "FFTW libraries: ${FFTW_LIBRARIES}")
target_link_libraries(HarpListen PRIVATE ${FFTW_LIBRARIES})

# FFTW3f for single precision
find_library(FFTWF_LIBRARIES NAMES fftw3f REQUIRED)
message(STATUS "FFTWf libraries: ${FFTWF_LIBRARIES}")
target_link_libraries(HarpListen PRIVATE ${FFTWF_LIBRARIES})

find_path(FFTW_INCLUDE_DIRS NAMES fftw3.h REQUIRED PATH_SUFFIXES /opt/homebrew/Cellar/)
message(STATUS "FFTW include dirs: ${FFTW_INCLUDE_DIRS}")
include_directories(${FFTW_INCLUDE_DIRS})

# BLAS
find_package(BLAS REQUIRED)
message(STATUS "BLAS libraries: ${BLAS_LIBRARIES}")
target_link_libraries(HarpListen PRIVATE ${BLAS_LIBRARIES})

# Eigen3
find_package(Eigen3 REQUIRED NO_MODULE)
message(STATUS "Eigen3 libraries: ${EIGEN3_LIBRARIES}")
message(STATUS "Eigen3 include dir: ${EIGEN3_INCLUDE_DIR}")
target_link_libraries(HarpListen PRIVATE Eigen3::Eigen)
include_directories(${EIGEN3_INCLUDE_DIR})

# Precompiled headers
target_precompile_headers(HarpListen PRIVATE pch.h)
get_target_property(PCH_SOURCES HarpListen PRECOMPILE_HEADERS)
message(STATUS "Precompiled headers: ${PCH_SOURCES}")

# Include directories and linking additional libraries
include_directories(HarpListen PRIVATE ${THIRD_PARTY_INCLUDE_DIRS})
target_link_libraries(HarpListen PRIVATE ${THIRD_PARTY_LIBRARIES})

# Print the compile flags
message(STATUS "CMAKE_CXX_FLAGS: ${CMAKE_CXX_FLAGS}")
