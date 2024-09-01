# cmake .. -G Ninja -DCMAKE_C_COMPILER=arm-none-eabi-gcc -DCMAKE_TOOLCHAIN_FILE:PATH="pico_toolchain.cmake"

set(CMAKE_SYSTEM_NAME Generic)
set(CMAKE_SYSTEM_PROCESSOR arm)

#set(CMAKE_EXE_LINKER_FLAGS "--specs=rdimon.specs" CACHE INTERNAL "")

set(CMAKE_C_COMPILER arm-none-eabi-gcc)
set(CMAKE_CXX_COMPILER arm-none-eabi-g++)
set(CMAKE_ASM_COMPILER arm-none-eabi-gcc)
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -mcpu=cortex-m0plus")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -mcpu=corteplus")

add_compile_options(-Os -O3 -DNDEBUG)

set(CMAKE_FIND_ROOT_PATH_MODE_PROGRAM NEVER)
set(CMAKE_FIND_ROOT_PATH_MODE_LIBRARY ONLY)
set(CMAKE_FIND_ROOT_PATH_MODE_INCLUDE ONLY)
set(CMAKE_FIND_ROOT_PATH_MODE_PACKAGE ONLY)

#function(download_and_setup_fftw)
#    if(NOT EXISTS "${CMAKE_SOURCE_DIR}/fftw-3.3.10")
#        execute_process(
#                COMMAND wget http://www.fftw.org/fftw-3.3.10.tar.gz
#                WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
#                RESULT_VARIABLE result
#        )
#        if(NOT result EQUAL "0")
#            message(FATAL_ERROR "Download of FFTW failed: ${result}")
#        endif()
#
#        execute_process(
#                COMMAND tar -xvzf fftw-3.3.10.tar.gz
#                WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
#                RESULT_VARIABLE result
#        )
#        execute_process(
#                COMMAND rm fftw-3.3.10.tar.gz
#                WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
#        )
#        if(NOT result EQUAL "0")
#            message(FATAL_ERROR "Unpacking of FFTW failed: ${result}")
#        endif()
#    endif()
#    execute_process(
#            COMMAND cp ${CMAKE_SOURCE_DIR}/fftw3_CMakeLists.txt ${CMAKE_SOURCE_DIR}/fftw-3.3.10/CMakeLists.txt
#            WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
#    )
#endfunction()
#
