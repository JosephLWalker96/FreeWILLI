cmake_minimum_required(VERSION 3.22.1)

project(HarpListen VERSION 1.0)

# Set the C++ standard
set(CMAKE_CXX_STANDARD 20)

add_subdirectory(${PROJECT_SOURCE_DIR}/src)
set(TEST_ON_RASPBERRY_PI_ZERO FALSE)

# Add a check for empty CMAKE_SYSTEM_NAME
if (NOT CMAKE_SYSTEM_NAME)
    message(FATAL_ERROR "CMAKE_SYSTEM_NAME is not set. Please check your CMake configuration.")
endif()

message(STATUS "CMAKE_SYSTEM_NAME: ${CMAKE_SYSTEM_NAME}")

# Check for a build type
if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Release)
endif()

# Check for OS
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

if(MACOSX)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -mcpu=apple-m1 -Og")
    add_subdirectory(${PROJECT_SOURCE_DIR}/tests)
elseif(UBUNTU)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -march=native -Og")
    #add_subdirectory(${PROJECT_SOURCE_DIR}/tests)
elseif(RASPBERRY_PI_4)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -mcpu=native -Og")
    add_subdirectory(${PROJECT_SOURCE_DIR}/tests)
elseif(RASPBERRY_PI_ZERO)
    #set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -mcpu=cortex-a53 -O3 -flto=auto")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -mcpu=native -Og")
    if(TEST_ON_RASPBERRY_PI_ZERO)
        add_subdirectory(${PROJECT_SOURCE_DIR}/tests)
    endif ()
else()
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -mcpu=native -Ogbu")
    add_subdirectory(${PROJECT_SOURCE_DIR}/tests)
endif()

if(CMAKE_BUILD_TYPE STREQUAL "Debug")
    message(STATUS "Building tests in Debug mode.")

    # the following line sets a variable `DEBUG` that
    # can be accessed in all cpp files in the project
    target_compile_definitions(HarpListen PRIVATE DEBUG)
    target_compile_definitions(UnitTests PRIVATE DEBUG)

    # Set the output directory for the executable
    set_property(TARGET HarpListen PROPERTY OUTPUT_NAME HarpListenDebug)
    set_target_properties(HarpListen PROPERTIES
            RUNTIME_OUTPUT_DIRECTORY ${PROJECT_SOURCE_DIR}/debug
    )
    set_property(TARGET UnitTests PROPERTY OUTPUT_NAME UnitTestsDebug)
    set_target_properties(UnitTests PROPERTIES
            RUNTIME_OUTPUT_DIRECTORY ${PROJECT_SOURCE_DIR}/debug
    )
    add_compile_options(-g -Wall -Wextra -fsanitize=address -fno-omit-frame-pointer)
else ()
    message(STATUS "Building tests in Release mode.")
    # Set the output directory for the executable
    set_target_properties(HarpListen PROPERTIES
            RUNTIME_OUTPUT_DIRECTORY ${PROJECT_SOURCE_DIR}/bin
    )

    # Enable Optimization flags
    add_compile_options(-O3 -flto=auto)
    # Vectorization
    if(CMAKE_CXX_COMPILER_ID MATCHES "GNU" OR CMAKE_CXX_COMPILER_ID MATCHES "Clang")
        if(CMAKE_SYSTEM_PROCESSOR MATCHES "arm|aarch64")
            message(STATUS "ARM processor does not support AVX vectorization.")
            if(MACOSX)
                message(STATUS "Apple M-series chip detected")
                add_compile_options(-march=arm64)
                # add_compile_options(--fast-math) # may mess up the results
            else ()
                message(STATUS "Non-M-series ARM chip detected: enabling neon vectorization")
                add_compile_options(-mfpu=neon)
            endif ()

        else()
            message(STATUS "x86 chip detected: enabling AVX vectorization.")
            add_compile_options(-mavx -mavx2 -mfma -O3)
        endif()
    elseif(CMAKE_CXX_COMPILER_ID MATCHES "MSVC")
        add_compile_options(/arch:AVX2 /O2)
    endif()
endif ()

# Print the compile flags
message(STATUS "CMAKE_CXX_FLAGS: ${CMAKE_CXX_FLAGS}")