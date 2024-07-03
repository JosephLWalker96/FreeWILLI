cmake_minimum_required(VERSION 3.22.1)

project(HarpListen VERSION 1.0)

# Set the C++ standard
set(CMAKE_CXX_STANDARD 20)

# Add a check for empty CMAKE_SYSTEM_NAME
if (NOT CMAKE_SYSTEM_NAME)
    message(FATAL_ERROR "CMAKE_SYSTEM_NAME is not set. Please check your CMake configuration.")
endif()

message(STATUS "CMAKE_SYSTEM_NAME: ${CMAKE_SYSTEM_NAME}")

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
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -mcpu=apple-m1 -O3 -flto=auto")
elseif(UBUNTU)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -march=native -O3 -flto=auto")
elseif(RASPBERRY_PI_4)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -mcpu=native -O3 -flto=auto")
elseif(RASPBERRY_PI_ZERO)
    #set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -mcpu=cortex-a53 -O3 -flto=auto")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -mcpu=native -O3 -flto=auto")
else()
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -mcpu=native -O3 -flto=auto")
endif()

# Print the compile flags
message(STATUS "CMAKE_CXX_FLAGS: ${CMAKE_CXX_FLAGS}")

add_subdirectory(${PROJECT_SOURCE_DIR}/libs/sigpack)
add_subdirectory(${PROJECT_SOURCE_DIR}/src)
add_subdirectory(${PROJECT_SOURCE_DIR}/tests)
