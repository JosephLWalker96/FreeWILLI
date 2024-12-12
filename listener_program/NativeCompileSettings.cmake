# SET THIRD PARTY LIBRARIES AND INCLUDE DIRECTORIES
#set(THIRD_PARTY_LIBRARIES)
#set(THIRD_PARTY_INCLUDE_DIRS)

# LAPACK
find_package(LAPACK REQUIRED)
if (LAPACK_LIBRARIES)
    list(APPEND THIRD_PARTY_LIBRARIES ${LAPACK_LIBRARIES})
    message(STATUS "LAPACK libraries: ${LAPACK_LIBRARIES}")
endif ()
if (LAPACK_INCLUDE_DIRS)
    list(APPEND THIRD_PARTY_INCLUDE_DIRS ${LAPACK_INCLUDE_DIRS})
    message(STATUS "LAPACK include dirs: ${LAPACK_INCLUDE_DIRS}")
endif()


# FFTW3
find_library(FFTW_LIBRARIES NAMES fftw3 REQUIRED)
message(STATUS "FFTW libraries: ${FFTW_LIBRARIES}")
find_path(FFTW_INCLUDE_DIRS NAMES fftw3.h REQUIRED PATH_SUFFIXES /opt/homebrew/Cellar/)
message(STATUS "FFTW include dirs: ${FFTW_INCLUDE_DIRS}")
if (FFTW_LIBRARIES)
    list(APPEND THIRD_PARTY_LIBRARIES ${FFTW_LIBRARIES})
endif ()
if (FFTW_INCLUDE_DIRS)
    list(APPEND THIRD_PARTY_INCLUDE_DIRS ${FFTW_INCLUDE_DIRS})
endif()


# Set the path to the ONNX Runtime library and include directories
#set(ONNXRUNTIME_ROOT_DIR "/usr/local/onnxruntime")
#set(ONNXRUNTIME_INCLUDE_DIRS "${ONNXRUNTIME_ROOT_DIR}/include")
#set(ONNXRUNTIME_LIBRARY_DIR "${ONNXRUNTIME_ROOT_DIR}/lib")

# Find the ONNX Runtime library using the specified path
find_library(ONNXRUNTIME_LIBRARIES NAMES onnxruntime PATHS ${ONNXRUNTIME_LIBRARY_DIR} REQUIRED)

message(STATUS "ONNX Runtime libraries: ${ONNXRUNTIME_LIBRARIES}")

# Find ONNX Runtime library
find_library(ONNXRUNTIME_LIBRARIES NAMES onnxruntime REQUIRED)
message(STATUS "ONNX Runtime libraries: ${ONNXRUNTIME_LIBRARIES}")

# Add ONNX Runtime to third-party libraries and include directories
if (ONNXRUNTIME_LIBRARIES)
    list(APPEND THIRD_PARTY_LIBRARIES ${ONNXRUNTIME_LIBRARIES})
endif ()
if (ONNXRUNTIME_INCLUDE_DIRS)
    list(APPEND THIRD_PARTY_INCLUDE_DIRS ${ONNXRUNTIME_INCLUDE_DIRS})
endif()

# FFTW3f for single precision
find_library(FFTWF_LIBRARIES NAMES fftw3f REQUIRED)
if (FFTWF_LIBRARIES)
    list(APPEND THIRD_PARTY_LIBRARIES ${FFTWF_LIBRARIES})
    message(STATUS "FFTWf libraries: ${FFTWF_LIBRARIES}")
endif ()
if (FFTWF_INCLUDE_DIRS)
    list(APPEND THIRD_PARTY_INCLUDE_DIRS ${FFTWF_INCLUDE_DIRS})
    message(STATUS "FFTWf include dirs: ${FFTWF_INCLUDE_DIRS}")
endif()

# BLAS
find_package(BLAS REQUIRED)
if (BLAS_LIBRARIES)
    list(APPEND THIRD_PARTY_LIBRARIES ${BLAS_LIBRARIES})
    message(STATUS "BLAS libraries: ${BLAS_LIBRARIES}")
endif ()
if (BLAS_INCLUDE_DIRS)
    list(APPEND THIRD_PARTY_INCLUDE_DIRS ${BLAS_INCLUDE_DIRS})
    message(STATUS "BLAS include dirs: ${BLAS_INCLUDE_DIRS}")
endif()



# Get system information
if (${CMAKE_SYSTEM_NAME} MATCHES "Darwin")
    set(MACOSX TRUE)
elseif (${CMAKE_SYSTEM_NAME} MATCHES "Linux")
    execute_process(
        COMMAND lsb_release -is
        OUTPUT_VARIABLE LSB_RELEASE_ID
        OUTPUT_STRIP_TRAILING_WHITESPACE
    )
    message(STATUS "LSB_RELEASE_ID: ${LSB_RELEASE_ID}")
    if (LSB_RELEASE_ID STREQUAL "Ubuntu")
        set(UBUNTU TRUE)
    elseif (LSB_RELEASE_ID STREQUAL "Raspbian" OR LSB_RELEASE_ID STREQUAL "Debian")
        set(RASPBIAN_DEBIAN TRUE)

        # Check for specific Raspberry Pi models
        file(READ "/proc/cpuinfo" CPUINFO_CONTENTS)
        if (CPUINFO_CONTENTS MATCHES "Raspberry Pi 4")
            set(RASPBERRY_PI_4 TRUE)
        elseif (CPUINFO_CONTENTS MATCHES "Raspberry Pi Zero")
            set(RASPBERRY_PI_ZERO TRUE)
        endif()
    endif()
else()
    message(FATAL_ERROR "Unsupported operating system or Raspberry Pi model.")
endif()
