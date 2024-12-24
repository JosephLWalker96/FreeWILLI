# cmake -DCMAKE_BUILD_TYPE=Debug ..

# cmake -DCMAKE_TOOLCHAIN_FILE=../toolchain.cmake ..

cmake_minimum_required(VERSION 3.29.7)
project(HarpListen VERSION 1.0)

# Set the C++ standard
set(CMAKE_CXX_STANDARD 20)

# Add a check for empty CMAKE_SYSTEM_NAME
if (NOT CMAKE_SYSTEM_NAME)
    message(FATAL_ERROR "CMAKE_SYSTEM_NAME is not set. Please check your CMake configuration.")
endif()
message(STATUS "CMAKE_SYSTEM_NAME: ${CMAKE_SYSTEM_NAME}")

# GLOBAL VARIABLES FOR SETTINGS
set(ENABLE_TEST TRUE)
set(ENABLE_AUTO_TEST TRUE)
set(ENABLE_BENCHMARK FALSE)

# Check for a build type
if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Release)
endif()

# Initialize third-party libraries and include directories
set(THIRD_PARTY_LIBRARIES "")
set(THIRD_PARTY_INCLUDE_DIRS "")

# Include and link directories
include_directories(${THIRD_PARTY_INCLUDE_DIRS})
link_directories(${CMAKE_LIBRARY_PATH})

# Find and append third-party libraries
if(CMAKE_CROSSCOMPILING) # assuming we are building on Ubuntu x86 and targeting R Pi Zero 2W
    set(ENABLE_AUTO_TEST FALSE)
    include("${CMAKE_SOURCE_DIR}/CrossCompileSettings.cmake")
    #include("${CMAKE_SOURCE_DIR}/VectorCompileSettings.cmake")
else ()
    include("${CMAKE_SOURCE_DIR}/NativeCompileSettings.cmake")
endif()

# Eigen3
find_package (Eigen3 REQUIRED NO_MODULE)
if (EIGEN3_LIBRARIES)
    list(APPEND THIRD_PARTY_LIBRARIES ${EIGEN3_LIBRARIES})
    message(STATUS "Eigen3 libraries: ${EIGEN3_LIBRARIES}")
endif ()
if (EIGEN3_INCLUDE_DIR)
    list(APPEND THIRD_PARTY_INCLUDE_DIRS ${EIGEN3_INCLUDE_DIRS})
    message(STATUS "Eigen3 include dir: ${EIGEN3_INCLUDE_DIR}")
endif()


# Define Optimization flags according to mode
if(CMAKE_BUILD_TYPE STREQUAL "Debug")
    if(CMAKE_CROSSCOMPILING) # assuming we are always cross-compiling for R Pi Zero 2W
        add_compile_options(-mcpu=cortex-a53 -O3 -g -Wall -fno-inline -flto=auto)
    elseif(MACOSX)
        add_compile_options(-O3 -g -Wall -fno-inline)
        add_link_options(-fsanitize=address)
    else ()
        #add_compile_options(-O3 -g -fanalyzer -Wall -Wextra -fno-inline)
        add_compile_options(-O3 -g -fanalyzer -fno-inline)
        #add_compile_options(-O0 -g -Wall -fno-inline -fsanitize=address)
    endif ()
elseif (CMAKE_BUILD_TYPE STREQUAL "Release")
    if(CMAKE_CROSSCOMPILING) # assuming we are always cross-compiling for R Pi Zero 2W
        # Define a variable for the vectorization report directory
        add_compile_options(-mcpu=cortex-a53 -O3 -fopt-info-vec-optimized-missed=vector_info.txt)
    elseif(CMAKE_CXX_COMPILER_ID MATCHES "GNU" OR CMAKE_CXX_COMPILER_ID MATCHES "Clang")
        if(CMAKE_SYSTEM_PROCESSOR MATCHES "arm|aarch64")
            message(STATUS "ARM processor does not support AVX vectorization.")
            if(MACOSX)
                message(STATUS "Apple M-series chip detected")
                add_compile_options(-mcpu=apple-m1)
            elseif(RASPBERRY_PI_ZERO)
                #message(STATUS "Non-M-series ARM chip detected: enabling neon vectorization")
                set(ENABLE_TEST FALSE)
                set(ENABLE_AUTO_TEST FALSE)
                set(ENABLE_BENCHMARK FALSE)
                add_compile_options(-mcpu=cortex-a53 -O3 -flto=auto)
            else()
                message(FATAL_ERROR "Could not determine compiler flags for native build")
            endif ()
        else()
            message(STATUS "x86 chip detected: enabling AVX vectorization.")
            add_compile_options(-march=native -flto=auto -mavx -mavx2 -mfma -O3)
        endif()
    elseif(CMAKE_CXX_COMPILER_ID MATCHES "MSVC")
        add_compile_options(/arch:AVX2 /O2)
    endif()
else()
    message(FATAL_ERROR "Specified compilation mode is not defined")
endif ()

# Add the source directory
add_subdirectory(${PROJECT_SOURCE_DIR}/src)

# Add the dbscan subdirectory
add_subdirectory(${PROJECT_SOURCE_DIR}/libs/dbscan)

# Add the test and benchmark directories
if (ENABLE_TEST)
    message(STATUS "Compiling Unit Tests")

    # Enable Google Mock and Google Test
    set(BUILD_GMOCK ON CACHE BOOL "" FORCE)
    set(INSTALL_GMOCK ON CACHE BOOL "" FORCE)
    set(BUILD_GTEST ON CACHE BOOL "" FORCE)
    set(INSTALL_GTEST ON CACHE BOOL "" FORCE)

    # Add Google Test and Google Mock to the project
    add_subdirectory(${PROJECT_SOURCE_DIR}/libs/googletest)
    
    # Add your tests directory
    add_subdirectory(${PROJECT_SOURCE_DIR}/tests)

    # Explicitly add include directories to UnitTests target
    target_include_directories(UnitTests PRIVATE
        ${PROJECT_SOURCE_DIR}/libs/googletest/googletest/include
        ${PROJECT_SOURCE_DIR}/libs/googletest/googlemock/include
    )

    # Link libraries for UnitTests
    target_link_libraries(UnitTests PRIVATE gmock gmock_main gtest gtest_main pthread)
endif()
if(ENABLE_BENCHMARK)
    message(STATUS "Compiling Benchmark Tests")
    set(BENCHMARK_ENABLE_TESTING off)
    add_subdirectory(${PROJECT_SOURCE_DIR}/libs/benchmark)
    add_subdirectory(${PROJECT_SOURCE_DIR}/benchmark)
endif()


# Set the output directory for the executable based on mode
if(CMAKE_BUILD_TYPE STREQUAL "Debug")
    message(STATUS "Building tests in Debug mode.")

    # the following line sets a variable `DEBUG` that
    # can be accessed in all cpp files in the project
    target_compile_definitions(HarpListen PRIVATE DEBUG)
    target_compile_definitions(UnitTests PRIVATE DEBUG)
    
    #BRING THIS BACK IN
    #target_compile_definitions(Benchmark PRIVATE DEBUG)

    # Set the output directory for the executable
    set_property(TARGET HarpListen PROPERTY OUTPUT_NAME HarpListenDebug)
    if(CMAKE_CROSSCOMPILING)
        set_property(TARGET HarpListen PROPERTY OUTPUT_NAME HarpListenDebugX)
    endif()
    set_target_properties(HarpListen PROPERTIES
            RUNTIME_OUTPUT_DIRECTORY ${PROJECT_SOURCE_DIR}/debug
    )
    if (ENABLE_TEST)
        set_property(TARGET UnitTests PROPERTY OUTPUT_NAME UnitTestsDebug)
        if(CMAKE_CROSSCOMPILING)
            set_property(TARGET UnitTests PROPERTY OUTPUT_NAME UnitTestsDebugX)
        endif()
        set_target_properties(UnitTests PROPERTIES
                RUNTIME_OUTPUT_DIRECTORY ${PROJECT_SOURCE_DIR}/debug
        )
    endif ()
    if (ENABLE_BENCHMARK)
        set_property(TARGET Benchmark PROPERTY OUTPUT_NAME BenchmarkDebug)
        if(CMAKE_CROSSCOMPILING)
            set_property(TARGET Benchmark PROPERTY OUTPUT_NAME BenchmarkDebugX)
        endif()
        set_target_properties(Benchmark PROPERTIES
                RUNTIME_OUTPUT_DIRECTORY ${PROJECT_SOURCE_DIR}/debug
        )
    endif ()
else ()
    message(STATUS "Building tests in Release mode.")
    if(CMAKE_CROSSCOMPILING)
        set_property(TARGET HarpListen PROPERTY OUTPUT_NAME HarpListenX)
        set_property(TARGET UnitTests PROPERTY OUTPUT_NAME UnitTestsX)
        #set_property(TARGET Benchmark PROPERTY OUTPUT_NAME BenchmarkX)
    endif()
    # Set the output directory for the executable
    set_target_properties(HarpListen PROPERTIES
            RUNTIME_OUTPUT_DIRECTORY ${PROJECT_SOURCE_DIR}/bin
    )
endif ()

# Print the compile flags
get_target_property(target_options HarpListen COMPILE_OPTIONS)
message(STATUS "Compile options for HarpListen: ${target_options}")

if (ENABLE_TEST)
    get_target_property(target_options UnitTests COMPILE_OPTIONS)
    message(STATUS "Compile options for UnitTests: ${target_options}")
endif ()

if(ENABLE_BENCHMARK)
    get_target_property(target_options Benchmark COMPILE_OPTIONS)
    message(STATUS "Compile options for Benchmark: ${target_options}")
endif ()
