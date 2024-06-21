cmake_minimum_required(VERSION 3.29.6)

if (${CMAKE_SYSTEM_NAME} MATCHES "Darwin")
    set(MACOSX TRUE)
endif()

project(HarpListen VERSION 1.0)

# Set the C++ standard
set(CMAKE_CXX_STANDARD 20)

if(MACOSX)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -mcpu=apple-m1 -O3 -flto=auto")
else()
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -mcpu=native -O3 -flto=auto")
endif()

set(SRC_DIR ${CMAKE_SOURCE_DIR}/src)
set(OBJ_DIR ${CMAKE_SOURCE_DIR}/src)

file(GLOB SRCS "${SRC_DIR}/*.cpp")

# Add executable
add_executable(${PROJECT_NAME} ${SRCS})

# Include directories
add_subdirectory(${PROJECT_SOURCE_DIR}/sigpack)
target_link_libraries(HarpListen PRIVATE SigPack)
target_include_directories(HarpListen PRIVATE
                          "${PROJECT_BINARY_DIR}"
                          "${PROJECT_SOURCE_DIR}/sigpack"
                          )

# Armadillo
find_package(Armadillo REQUIRED PATH_SUFFIXES /opt/homebrew/Cellar/)
message(STATUS "Armadillo libraries: ${ARMADILLO_LIBRARIES}")
message(STATUS "Armadillo include dirs: ${ARMADILLO_INCLUDE_DIRS}")
include_directories(${ARMADILLO_INCLUDE_DIRS})
target_link_libraries(HarpListen PRIVATE ${ARMADILLO_LIBRARIES})

# LAPACK
find_package(LAPACK REQUIRED)
message(STATUS "LAPACK libraries: ${LAPACK_LIBRARIES}")
message(STATUS "LAPACK include dirs: ${LAPACK_INCLUDE_DIRS}")
target_link_libraries(HarpListen PRIVATE ${LAPACK_LIBRARIES})

# Liquid
find_library(LIQUID_LIBRARIES NAMES liquid REQUIRED)
message(STATUS "LIQUID libraries: ${LIQUID_LIBRARIES}")
target_link_libraries(HarpListen PRIVATE ${LIQUID_LIBRARIES})

find_path(LIQUID_INCLUDE_DIRS NAMES liquid.h REQUIRED PATH_SUFFIXES /opt/homebrew/Cellar/)
message(STATUS "LIQUID include dirs: ${LIQUID_INCLUDE_DIRS}")
include_directories(HarpListen PRIVATE ${LIQUID_INCLUDE_DIRS})

# FFTW3
find_library(FFTW_LIBRARIES NAMES fftw3 REQUIRED)
message(STATUS "FFTW libraries: ${FFTW_LIBRARIES}")
target_link_libraries(HarpListen PRIVATE ${FFTW_LIBRARIES})

find_path(FFTW_INCLUDE_DIRS NAMES fftw3.h REQUIRED PATH_SUFFIXES /opt/homebrew/Cellar/)
message(STATUS "FFTW include dirs: ${FFTW_INCLUDE_DIRS}")
include_directories(HarpListen PRIVATE ${FFTW_INCLUDE_DIRS})


# BLAS
find_package(BLAS REQUIRED)
message(STATUS "BLAS libraries: ${BLAS_LIBRARIES}")
message(STATUS "BLAS include dirs: ${BLAS_INCLUDE_DIRS}")
target_link_libraries(HarpListen PRIVATE ${BLAS_LIBRARIES})

# SET(OpenBLAS_LIBRARIES /opt/homebrew/Cellar/openblas/0.3.27/lib/libopenblas.dylib)
# message(STATUS "OPENBLAS libraries: ${OPENBLAS_LIBRARIES}")
# target_link_libraries(HarpListen PRIVATE ${OpenBLAS_LIBRARIES})

# SET(OpenBLAS_INCLUDE_DIRS /opt/homebrew/Cellar/openblas/0.3.27/include)
# message(STATUS "OPENBLAS include dirs: ${OPENBLAS_INCLUDE_DIRS}")
# include_directories(HarpListen PRIVATE ${OpenBLAS_INCLUDE_DIRS})

# Eigen3
find_package (Eigen3 REQUIRED NO_MODULE)
message(STATUS "Eigen3 libraries: ")
target_link_libraries(HarpListen PRIVATE Eigen3::Eigen)
# set(EIGEN_ROOT_DIR "/opt/homebrew/Cellar/eigen/3.4.0_1/include/eigen3/")
# include_directories(
#     ${EIGEN_ROOT_DIR}
# )


# The following is for googletest unit test
# include(FetchContent)
# FetchContent_Declare(
#   googletest
#   URL https://github.com/google/googletest/archive/03597a01ee50ed33e9dfd640b249b4be3799d395.zip
# )

# # For Windows: Prevent overriding the parent project's compiler/linker settings
# set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
# FetchContent_MakeAvailable(googletest)


# add_executable(filters_test ${CMAKE_SOURCE_DIR}/unit_tests/filters_test.cpp)
# target_link_libraries(filters_test gtest_main)
# add_test(NAME FilterTest COMMAND filters_test)