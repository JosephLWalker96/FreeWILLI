message(STATUS "Cross-compiling for ARM (Raspberry Pi Zero 2W) with static linking")

# Ensure CMake searches the correct paths within the sysroot
set(CMAKE_PREFIX_PATH ${CMAKE_SYSROOT}/usr ${CMAKE_SYSROOT}/usr/lib/aarch64-linux-gnu ${CMAKE_SYSROOT}/usr/include)
set(CMAKE_LIBRARY_PATH ${CMAKE_SYSROOT}/usr/lib/aarch64-linux-gnu ${CMAKE_SYSROOT}/lib/aarch64-linux-gnu)

# Enforce static linking
set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -static")
set(CMAKE_FIND_LIBRARY_SUFFIXES ".a")
set(BUILD_SHARED_LIBS OFF)

# Find FFTW3 (Static)
find_library(FFTW_LIBRARIES NAMES fftw3 REQUIRED PATHS ${CMAKE_SYSROOT}/usr/lib/aarch64-linux-gnu/ NO_DEFAULT_PATH)
find_path(FFTW_INCLUDE_DIRS NAMES fftw3.h PATHS ${CMAKE_SYSROOT}/usr/include NO_DEFAULT_PATH)
if (FFTW_LIBRARIES)
    list(APPEND THIRD_PARTY_LIBRARIES ${FFTW_LIBRARIES})
    message(STATUS "FFTW libraries (static cross-compiling): ${FFTW_LIBRARIES}")
endif ()
if (FFTW_INCLUDE_DIRS)
    list(APPEND THIRD_PARTY_INCLUDE_DIRS ${FFTW_INCLUDE_DIRS})
    message(STATUS "FFTW include dirs (cross-compiling): ${FFTW_INCLUDE_DIRS}")
endif()

# Find ONNX Runtime library and include directory (Static)
find_library(ONNXRUNTIME_LIBRARIES 
            NAMES onnxruntime 
            REQUIRED 
            PATHS ${CMAKE_SYSROOT}/usr/lib/aarch64-linux-gnu/ 
                ${CMAKE_SYSROOT}/usr/local/lib 
            NO_DEFAULT_PATH)

find_path(ONNXRUNTIME_INCLUDE_DIRS 
        NAMES onnxruntime_cxx_api.h 
        PATHS ${CMAKE_SYSROOT}/usr/include 
                ${CMAKE_SYSROOT}/usr/local/include 
        NO_DEFAULT_PATH)

if (ONNXRUNTIME_LIBRARIES AND ONNXRUNTIME_INCLUDE_DIRS)
    list(APPEND THIRD_PARTY_LIBRARIES ${ONNXRUNTIME_LIBRARIES})
    list(APPEND THIRD_PARTY_INCLUDE_DIRS ${ONNXRUNTIME_INCLUDE_DIRS})
    
    message(STATUS "ONNX Runtime libraries (static cross-compiling): ${ONNXRUNTIME_LIBRARIES}")
    message(STATUS "ONNX Runtime include dirs (cross-compiling): ${ONNXRUNTIME_INCLUDE_DIRS}")
else()
    message(FATAL_ERROR "ONNX Runtime libraries or include directories not found!")
endif()

# Find FFTW3f for single precision (Static)
find_library(FFTWF_LIBRARIES NAMES fftw3f REQUIRED PATHS ${CMAKE_SYSROOT}/usr/lib/aarch64-linux-gnu/ NO_DEFAULT_PATH)
if (FFTWF_LIBRARIES)
    list(APPEND THIRD_PARTY_LIBRARIES ${FFTWF_LIBRARIES})
    message(STATUS "FFTWf libraries (static cross-compiling): ${FFTWF_LIBRARIES}")
endif ()
if (FFTWF_INCLUDE_DIRS)
    list(APPEND THIRD_PARTY_INCLUDE_DIRS ${FFTWF_INCLUDE_DIRS})
    message(STATUS "FFTWf include dirs (cross-compiling): ${FFTWF_INCLUDE_DIRS}")
endif()

