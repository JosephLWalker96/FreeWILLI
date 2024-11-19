# VectorCompileSettings.cmake


# Ensure CMake searches the correct paths within the sysroot
set(CMAKE_PREFIX_PATH ${CMAKE_SYSROOT}/usr ${CMAKE_SYSROOT}/usr/lib/aarch64-linux-gnu ${CMAKE_SYSROOT}/usr/include)
set(CMAKE_LIBRARY_PATH ${CMAKE_SYSROOT}/usr/lib/aarch64-linux-gnu ${CMAKE_SYSROOT}/lib/aarch64-linux-gnu)

# Find OpenBLAS
find_library(OPENBLAS_LIBRARIES 
NAMES openblas openblasp-r0.3.21
PATHS ${CMAKE_SYSROOT}/usr/lib/aarch64-linux-gnu/ 
    ${CMAKE_SYSROOT}/usr/lib/aarch64-linux-gnu/openblas-pthread/
NO_DEFAULT_PATH)

if (OPENBLAS_LIBRARIES)
list(APPEND THIRD_PARTY_LIBRARIES ${OPENBLAS_LIBRARIES})
message(STATUS "OpenBLAS libraries (cross-compiling): ${OPENBLAS_LIBRARIES}")
else()
message(FATAL_ERROR "OpenBLAS libraries not found!")
endif()

# Find FFTW3
find_library(FFTW_LIBRARIES NAMES fftw3 REQUIRED PATHS ${CMAKE_SYSROOT}/usr/lib/aarch64-linux-gnu/ NO_DEFAULT_PATH)
find_path(FFTW_INCLUDE_DIRS NAMES fftw3.h PATHS ${CMAKE_SYSROOT}/usr/include NO_DEFAULT_PATH)
if (FFTW_LIBRARIES)
    list(APPEND THIRD_PARTY_LIBRARIES ${FFTW_LIBRARIES})
    message(STATUS "FFTW libraries (cross-compiling): ${FFTW_LIBRARIES}")
endif ()
if (FFTW_INCLUDE_DIRS)
    list(APPEND THIRD_PARTY_INCLUDE_DIRS ${FFTW_INCLUDE_DIRS})
    message(STATUS "FFTW include dirs (cross-compiling): ${FFTW_INCLUDE_DIRS}")
endif()

# Find ONNX Runtime library and include directory
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
    
    message(STATUS "ONNX Runtime libraries (cross-compiling): ${ONNXRUNTIME_LIBRARIES}")
    message(STATUS "ONNX Runtime include dirs (cross-compiling): ${ONNXRUNTIME_INCLUDE_DIRS}")
else()
    message(FATAL_ERROR "ONNX Runtime libraries or include directories not found!")
endif()

# Find FFTW3f for single precision
find_library(FFTWF_LIBRARIES NAMES fftw3f REQUIRED PATHS ${CMAKE_SYSROOT}/usr/lib/aarch64-linux-gnu/ NO_DEFAULT_PATH)
if (FFTWF_LIBRARIES)
    list(APPEND THIRD_PARTY_LIBRARIES ${FFTWF_LIBRARIES})
    message(STATUS "FFTWf libraries (cross-compiling): ${FFTWF_LIBRARIES}")
endif ()
if (FFTWF_INCLUDE_DIRS)
    list(APPEND THIRD_PARTY_INCLUDE_DIRS ${FFTWF_INCLUDE_DIRS})
    message(STATUS "FFTWf include dirs (cross-compiling): ${FFTWF_INCLUDE_DIRS}")
endif()



















# Set the directory for vectorization reports
set(VECTOR_REPORT_DIR "${CMAKE_BINARY_DIR}/vector_reports")
file(MAKE_DIRECTORY "${VECTOR_REPORT_DIR}")

# Define compile options specifically for vectorization report generation
set(VECTOR_COMPILE_OPTIONS
    "-mcpu=cortex-a53"
    "-O3"
    "-flto=auto"
)

# Find all .cpp files in src/ for vectorization reporting
file(GLOB_RECURSE VECTOR_SOURCE_FILES "${PROJECT_SOURCE_DIR}/src/*.cpp")

# Create a custom target for generating vectorization reports
add_custom_target(vector_reports ALL)

foreach(src_file ${VECTOR_SOURCE_FILES})
    get_filename_component(src_name ${src_file} NAME_WE)

    # Define the output vector report file for each source file
    set(VECTOR_REPORT_FILE "${VECTOR_REPORT_DIR}/vec_report_${src_name}.txt")

    # Add a custom command to compile each file with vectorization report enabled
    add_custom_command(
        OUTPUT "${VECTOR_REPORT_FILE}"
        COMMAND ${CMAKE_CXX_COMPILER} ${VECTOR_COMPILE_OPTIONS}
            -fopt-info-vec-optimized=${VECTOR_REPORT_FILE}
            -c ${src_file} -o "${CMAKE_BINARY_DIR}/CMakeFiles/vector_reports.dir/${src_name}.o"
        DEPENDS ${src_file}
        COMMENT "Generating vector report for ${src_name}"
        VERBATIM
    )

    # Add each report to the vector_reports target to ensure they're all generated
    add_custom_target(vector_report_${src_name} DEPENDS "${VECTOR_REPORT_FILE}")
    add_dependencies(vector_reports vector_report_${src_name})
endforeach()