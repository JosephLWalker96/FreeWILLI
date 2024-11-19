cmake_minimum_required(VERSION 3.29)
project(HarpListen VERSION 1.0)

# Set the C++ standard
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Set default build type to Release if not specified
if (NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Release)
endif()
# Find and append third-party libraries
if(CMAKE_CROSSCOMPILING) # assuming we are building on Ubuntu x86 and targeting R Pi Zero 2W
    set(ENABLE_AUTO_TEST FALSE)
    include("${CMAKE_SOURCE_DIR}/../CrossCompileSettings.cmake")
else ()
    include("${CMAKE_SOURCE_DIR}/NativeCompileSettings.cmake")
endif()
# Specify the directory for vectorization reports
set(VECTOR_REPORT_DIR "${CMAKE_BINARY_DIR}/vector_reports")
file(MAKE_DIRECTORY ${VECTOR_REPORT_DIR})

# Define common compile options, including -mcpu=cortex-a53 for cross-compiling
set(COMMON_COMPILE_OPTIONS
    -mcpu=cortex-a53
    -O3
)

# Find all .cpp files in src/ located one directory above
set(SRC_FOLDER_PATH "${CMAKE_SOURCE_DIR}/../src/*.cpp")
file(GLOB SRC_FILES "${SRC_FOLDER_PATH}")

# Print out the folder path
message(STATUS "Looking for source files in: ${SRC_FOLDER_PATH}")

# Compile each source file with vectorization report output
foreach(src_file ${SRC_FILES})
    get_filename_component(src_name ${src_file} NAME_WE)

    # Set vectorization report file path
    set(VECTOR_REPORT_FILE "${VECTOR_REPORT_DIR}/vector_report_${src_name}.txt")

    # Apply compile options, including vectorization report flag with the full path for each file
    set_source_files_properties(${src_file} PROPERTIES COMPILE_OPTIONS
        "${COMMON_COMPILE_OPTIONS};-fopt-info-vec-optimized=${VECTOR_REPORT_FILE}"
    )

    # Add an executable target for each file
    add_executable(${src_name} ${src_file})
endforeach()
