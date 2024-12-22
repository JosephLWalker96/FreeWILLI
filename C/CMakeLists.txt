cmake_minimum_required(VERSION 3.29)

#if (CMAKE_TOOLCHAIN_FILE)
if (${CMAKE_BUILD_TYPE} STREQUAL "PicoW")
    message("Compiling for PICO W")

    set(PICO_SDK_PATH "${CMAKE_SOURCE_DIR}/libs/pico-sdk")
    set(PICO_BOARD pico_w)
    include(${PICO_SDK_PATH}/pico_sdk_init.cmake)
    include(${CMAKE_SOURCE_DIR}/pico_sdk_import.cmake)

    project(HarpListen VERSION 1.0 LANGUAGES C CXX ASM)
    # optimize the code
#    add_compile_options(-O3 -Os)
    add_compile_options(-Os)
else ()
    project(HarpListen VERSION 1.0)
endif ()

# Set the C++ standard
set(CMAKE_CXX_STANDARD 20)

set(TEST_ON_RASPBERRY_PI_ZERO FALSE)

# Add a check for empty CMAKE_SYSTEM_NAME
if (NOT CMAKE_SYSTEM_NAME)
    message(FATAL_ERROR "CMAKE_SYSTEM_NAME is not set. Please check your CMake configuration.")
endif()
message(STATUS "CMAKE_SYSTEM_NAME: ${CMAKE_SYSTEM_NAME}")

# GLOBAL VARIABLES FOR SETTINGS
set(ENABLE_TEST FALSE)
set(ENABLE_AUTO_TEST FALSE)
set(ENABLE_BENCHMARK FALSE)
# Check for a build type
if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Release)
endif()

# SET THIRD PARTY LIBRARIES AND INCLUDE DIRECTORIES
set(THIRD_PARTY_LIBRARIES)
set(THIRD_PARTY_INCLUDE_DIRS)

if (${CMAKE_BUILD_TYPE} STREQUAL "PicoW")
    SET(FREERTOS_KERNEL_PATH ${CMAKE_SOURCE_DIR}/libs/FreeRTOS-Kernel)
    include(FreeRTOS_Kernel_import.cmake)
    include_directories(${FREERTOS_KERNEL_PATH}/portable/ThirdParty/GCC/RP2040)
    include_directories(${CMAKE_SOURCE_DIR}/port/FreeRTOS-Kernel)

    pico_sdk_init()
    # fftw3 3.3.10 branch does not work as expected, use webget instead
    if(NOT EXISTS "${CMAKE_SOURCE_DIR}/libs/fftw-3.3.10")
        execute_process(
                COMMAND wget http://www.fftw.org/fftw-3.3.10.tar.gz
                WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}/libs
        )
        execute_process(
                COMMAND tar -xvzf fftw-3.3.10.tar.gz
                WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}/libs
        )
        execute_process(
                COMMAND rm fftw-3.3.10.tar.gz
                WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}/libs
        )
    endif ()
    execute_process(
            COMMAND cp fftw3_CMakeLists.txt libs/fftw-3.3.10/CMakeLists.txt
            WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
    )

    add_subdirectory(${CMAKE_SOURCE_DIR}/libs/eigen)
    include_directories(${CMAKE_SOURCE_DIR}/libs/eigen)
    add_subdirectory(${CMAKE_SOURCE_DIR}/libs/fftw-3.3.10)
    include_directories(${CMAKE_SOURCE_DIR}/libs/fftw-3.3.10/api)

    add_subdirectory(${CMAKE_SOURCE_DIR}/libs/CH9121)
    include_directories(${CMAKE_SOURCE_DIR}/libs/CH9121)

    include(${CMAKE_SOURCE_DIR}/lwip_import.cmake)
    include_directories(${CMAKE_SOURCE_DIR}/port/lwip)

    list(APPEND THIRD_PARTY_LIBRARIES pico_cyw43_arch_lwip_sys_freertos)
    list(APPEND THIRD_PARTY_LIBRARIES pico_stdlib)
    list(APPEND THIRD_PARTY_LIBRARIES pico_lwip_freertos)
    list(APPEND THIRD_PARTY_LIBRARIES FreeRTOS-Kernel)
    list(APPEND THIRD_PARTY_LIBRARIES FreeRTOS-Kernel-Heap4) # FreeRTOS kernel and dynamic heap
#    list(APPEND THIRD_PARTY_LIBRARIES LWIP_PORT)

    list(APPEND THIRD_PARTY_INCLUDE_DIRS "${CMAKE_SOURCE_DIR}/port/FreeRTOS-Kernel")
    list(APPEND THIRD_PARTY_INCLUDE_DIRS "${CMAKE_SOURCE_DIR}/port/lwip")

#     Add the source directory
    add_subdirectory(${PROJECT_SOURCE_DIR}/src)

#   Temporary Test Files
    # Test fftw compilation on pico
    add_executable(hello_pico fftw_test.cpp)
    set(PICO "PICO")
    target_compile_definitions(hello_pico PUBLIC PICO)
    target_link_libraries(hello_pico
#            ${THIRD_PARTY_LIBRARIES}
            pico_stdlib
            fftw3f
    )
    include_directories(hello_pico
            PUBLIC
            ${THIRD_PARTY_INCLUDE_DIRS}
    )
    pico_enable_stdio_usb(hello_pico 1)
    pico_enable_stdio_uart(hello_pico 0)
    pico_add_extra_outputs(hello_pico)

    # Test usb on pico
#    add_executable(hello_usb usb_test.cpp)
#    target_link_libraries(hello_usb
#            pico_stdlib
#            pico_stdio
#    )
#    pico_enable_stdio_usb(hello_usb 1)
#    pico_add_extra_outputs(hello_usb)

    # Test uart on pico
#    add_executable(hello_uart uart_test.cpp)
#    target_link_libraries(hello_uart
#            pico_stdlib
#            pico_stdio
#            hardware_uart
#            hardware_gpio
#    )
#    pico_enable_stdio_uart(hello_uart 1)
#    pico_add_extra_outputs(hello_uart)

    # Test ethernet on pico
#    add_executable(hello_ethnet ethnet_test.cpp)
#    target_link_libraries(hello_ethnet
#            CH9121
#            pico_stdlib
#            hardware_spi
#    )
#    pico_enable_stdio_usb(hello_ethnet 1)
#    pico_enable_stdio_uart(hello_ethnet 1)
#    pico_add_extra_outputs(hello_ethnet)

    # Test udp on pico over FreeRTOS
#    add_executable(hello_udp udp_test.cpp)
#    target_link_libraries(hello_udp
#            pico_stdlib
#            FreeRTOS-Kernel-Heap4
#            pico_cyw43_arch_lwip_sys_freertos
#            LWIP_PORT
#    )
#    pico_enable_stdio_uart(hello_udp 1)
#    pico_add_extra_outputs(hello_udp)

    # Test simple_server over wifi
    add_executable(wifi_server simple_server_wifi.cpp)
    target_compile_definitions(wifi_server PRIVATE
            WIFI_SSID=\"LeoZ\"
            WIFI_PASSWORD=\"LZ19990114\"
            NO_SYS=0            # don't want NO_SYS (generally this would be in your lwipopts.h)
            LWIP_SOCKET=1       # we need the socket API (generally this would be in your lwipopts.h)
            PING_USE_SOCKETS=1
    )
    target_link_libraries(wifi_server PUBLIC
#            pico_cyw43_arch_lwip_sys_freertos
#            pico_stdlib
#            pico_lwip_freertos
#            FreeRTOS-Kernel-Heap4 # FreeRTOS kernel and dynamic heap
            ${THIRD_PARTY_LIBRARIES}
    )
    target_include_directories(wifi_server PUBLIC
            ${THIRD_PARTY_INCLUDE_DIRS}
    )
    pico_enable_stdio_usb(wifi_server 1)
    pico_enable_stdio_uart(wifi_server 0)
    pico_add_extra_outputs(wifi_server)

    # Test multicore
    add_executable(multicore multicore.cpp)
    target_link_libraries(multicore PUBLIC
            #            pico_cyw43_arch_lwip_sys_freertos
            #            pico_stdlib
            #            pico_lwip_freertos
            #            FreeRTOS-Kernel-Heap4 # FreeRTOS kernel and dynamic heap
            ${THIRD_PARTY_LIBRARIES}
    )
    target_include_directories(multicore PUBLIC
            ${THIRD_PARTY_INCLUDE_DIRS}
    )
    pico_enable_stdio_usb(multicore 1)
    pico_enable_stdio_uart(multicore 0)
    pico_add_extra_outputs(multicore)

else ()
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
#    find_package(BLAS REQUIRED)
#    if (BLAS_LIBRARIES)
#        list(APPEND THIRD_PARTY_LIBRARIES ${BLAS_LIBRARIES})
#        message(STATUS "BLAS libraries: ${BLAS_LIBRARIES}")
#    endif ()
#    if (BLAS_INCLUDE_DIRS)
#        list(APPEND THIRD_PARTY_INCLUDE_DIRS ${BLAS_INCLUDE_DIRS})
#        message(STATUS "BLAS include dirs: ${BLAS_INCLUDE_DIRS}")
#    endif()

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

    # Set compiler flags based on OS
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
        add_compile_options(-mcpu=apple-m1)
        set(ENABLE_TEST TRUE)
        set(ENABLE_BENCHMARK TRUE)
    elseif(UBUNTU)
        add_compile_options(-march=native)
        set(ENABLE_TEST TRUE)
        set(ENABLE_BENCHMARK TRUE)
    elseif(RASPBERRY_PI_4)
        add_compile_options(-march=native)
        set(ENABLE_TEST TRUE)
        set(ENABLE_BENCHMARK TRUE)
    elseif(RASPBERRY_PI_ZERO)
        #set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -mcpu=cortex-a53 -O3 -flto=auto")
        add_compile_options(-march=native)
        if(TEST_ON_RASPBERRY_PI_ZERO)
            set(ENABLE_TEST TRUE)
        endif ()
        set(ENABLE_BENCHMARK TRUE)
    else()
        add_compile_options(-march=native)
        set(ENABLE_TEST TRUE)
        set(ENABLE_BENCHMARK TRUE)
    endif()

    if(CMAKE_BUILD_TYPE STREQUAL "Debug")
        if (MACOSX)
            add_compile_options(-Og -g -Wall -Wextra)
            add_link_options(-fsanitize=address)
        else ()
            add_compile_options(-Og -g -Wall -Wextra -fsanitize=address)
            add_link_options(-fsanitize=address)
        endif ()
    else ()
        # Enable Optimization flags
        add_compile_options(-O3 -flto=auto)
        # Vectorization
        if(CMAKE_CXX_COMPILER_ID MATCHES "GNU" OR CMAKE_CXX_COMPILER_ID MATCHES "Clang")
            if(CMAKE_SYSTEM_PROCESSOR MATCHES "arm|aarch64")
                message(STATUS "ARM processor does not support AVX vectorization.")
                if(MACOSX)
                    message(STATUS "Apple M-series chip detected")
    #                add_compile_options(-march=arm64)
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

    # Add the source directory
    add_subdirectory(${PROJECT_SOURCE_DIR}/src)
    # Add the test and benchmark directories
    if (ENABLE_TEST)
        # Set the build options to skip Google Mock
        set(BUILD_GMOCK OFF CACHE BOOL "" FORCE)
        set(INSTALL_GMOCK OFF CACHE BOOL "" FORCE)
        set(BUILD_GTEST ON CACHE BOOL "" FORCE)
        set(INSTALL_GTEST ON CACHE BOOL "" FORCE)
        # current googletest version is on release-1.10.0
        add_subdirectory(${PROJECT_SOURCE_DIR}/libs/googletest)
        add_subdirectory(${PROJECT_SOURCE_DIR}/tests)
    endif()
    #set(ENABLE_BENCHMARK FALSE)
    if(ENABLE_BENCHMARK)
        set(BENCHMARK_ENABLE_TESTING off)
        # current benchmark version is on v1.8.5
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
        target_compile_definitions(Benchmark PRIVATE DEBUG)

        # Set the output directory for the executable
        set_property(TARGET HarpListen PROPERTY OUTPUT_NAME HarpListenDebug)
        set_target_properties(HarpListen PROPERTIES
                RUNTIME_OUTPUT_DIRECTORY ${PROJECT_SOURCE_DIR}/debug
        )
        if (ENABLE_TEST)
            set_property(TARGET UnitTests PROPERTY OUTPUT_NAME UnitTestsDebug)
            set_target_properties(UnitTests PROPERTIES
                    RUNTIME_OUTPUT_DIRECTORY ${PROJECT_SOURCE_DIR}/debug
            )
        endif ()
        if (ENABLE_BENCHMARK)
            set_property(TARGET Benchmark PROPERTY OUTPUT_NAME BenchmarkDebug)
            set_target_properties(Benchmark PROPERTIES
                    RUNTIME_OUTPUT_DIRECTORY ${PROJECT_SOURCE_DIR}/debug
            )
        endif ()
    #    add_compile_options(-Og -g -Wall -Wextra -fsanitize=address)
    else ()
        message(STATUS "Building tests in Release mode.")
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
endif ()
