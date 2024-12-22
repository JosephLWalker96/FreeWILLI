# Embedded_miniHarp for Raspberry Pi Pico W
## Implementation
### Set Up

### CMake
#### 3rd-party library and tools
The project on pico w is using the following library
- **eigen**: this library is clone from the gitlab, and can be set up with 
  `git submodule init;git submodule update` \
  It is included with the following command in cmake
  `add_subdirectory(${CMAKE_SOURCE_DIR}/libs/eigen)` 
  `include_directories(${CMAKE_SOURCE_DIR}/libs/eigen)`
- **fftw**: this library is a bit complicated. Since it is not able to directly included via add_subdirectory and include_directory, we need a custom cmake file. \
            the custom cmake file is located in [fftw3_CMakeLists.txt](C%2Ffftw3_CMakeLists.txt). This cmake file restrict the library to only work in single precision. \
            Please note that the fftw is downloaded from http://www.fftw.org/download.html instead of being cloned from github. The library is saved at [fftw-3.3.10](C%2Flibs%2Ffftw-3.3.10). \
            Also note that [fftw3_CMakeLists.txt](C%2Ffftw3_CMakeLists.txt) will be automatically moved to the correct position inside [fftw-3.3.10](C%2Flibs%2Ffftw-3.3.10) once cmake command is run. \
            The command to include this library is in cmake:
            `add_subdirectory(${CMAKE_SOURCE_DIR}/libs/fftw-3.3.10)`
            `include_directories(${CMAKE_SOURCE_DIR}/libs/fftw-3.3.10/api)`
- **pico-sdk**: this library comes from the github repo, which can be set up via `git submodule init;git submodule update`. Please refer to cmake to check out how it is included as it is quite complicated.
- **FreeRTOS-Kernel**: this library is in charge of handling kernel command. Please refer to cmake to check out how it is included as it is quite complicated.
- **C/port/FreeRTOS-Kernel**: [FreeRTOSConfig.h](C%2Fport%2FFreeRTOS-Kernel%2FFreeRTOSConfig.h) specifies the setting of the kernel: heap size (note: taken by kernel, not the system), number of cores and so on. Currently, the kernel application does not take over the whole heap space of the system. \
                              Note: It can be, however, set up to take over the whole heap space, and kernel can be used to manipulate the heap space of the system. This can be experimented further in the future.
- **C/port/lwip**: [lwipopts.h](C%2Fport%2Flwip%2Flwipopts.h) is the config file for lwip setting: mainly for TCP/UDP transmission setting. This file can be experimented further.

#### cmake files
- **[C/CMakeLists.txt](C%2FCMakeLists.txt)**: main cmake file for including and set up, functioning the same as the other branch.
- **[C/src/CMakeLists.txt](C%2Fsrc%2FCMakeLists.txt)**: cmake file for generating src executable, functioning the same as the other branch.
- **[C/pico_sdk_import.cmake](C%2Fpico_sdk_import.cmake)**: set up for the pico-sdk include.
- **[C/pico_toolchain.cmake](C%2Fpico_toolchain.cmake)**: toolchain file to set up the c and cpp compiler for pico w executables. \
                                                          To install the compiler, on mac, use `brew install gcc-arm-embedded`\
                                                          On ubuntu, download from https://developer.arm.com/downloads/-/arm-gnu-toolchain-downloads \
                                                          if `arm-none-eabi-gcc` is not set up correctly in ubuntu, modify [C/pico_toolchain.cmake](C%2Fpico_toolchain.cmake) to have the absolute path to the executable.
### Usage
Follow the following commands\
`cd C` \
`mkdir build; cd build` \
`cmake -S .. -B . -DCMAKE_BUILD_TYPE=PicoW` \
`cmake --build . --target HarpListen `\
After the build is complete, the executable can be found as C/build/src/HarpListen.uf2 \
Drag this file into pico w over a usb connection, and the program will start working. \
To observe the log message from pico, you need to connect over usb with pico w and have\
`minicom -D /dev/tty.usbmodem1301 -b 115200 -o`\
Device name is subject to change, and you can find that via `ls /dev/tty.*` to find a device name similar to the above.\
Note: pico w is transmiting data over wifi only, so make sure to change the wifi ssid and password in [C/src/main.cpp](C%2Fsrc%2Fmain.cpp)

### Understanding the code
To read the code, it might be difficult to directly read [C/src/main.cpp](C%2Fsrc%2Fmain.cpp). So instead, you can read the following files:
- [C/fftw_test.cpp](C%2Ffftw_test.cpp): This file shows simple print program using pico-sdk
- [C/simple_server_wifi.cpp](C%2Fsimple_server_wifi.cpp): This is an example of using the wifi connection on pico-w. It also showcases how to work with multi-processing in the context of pico-sdk + freertos-kernel
- [C/multicore.cpp](C%2Fmulticore.cpp): This is an example for multicore program, and it demonstrates how to assign a specific process to a specific core.
- [C/src/main.cpp](C%2Fsrc%2Fmain.cpp): This file is similar to all above, and it wraps the original functions to make it workable for pico-sdk + freertos.

## Known Issues

### Issue 1: Memory Management
- **Problem**:
    - When using `TIME_WINDOW >= 0.01`, the system runs out of memory even with only 10 packets. GCC_PHAT also needs a lot of memory for `TIME_WINDOW >= 0.01`
    - With `TIME_WINDOW = 0.002`, the system can handle more packets, but it cannot process continuous inputs in real-time because the data buffer will be quickly overflow or out of memory.
    - The data processor becomes too slow to keep up with the listener's rate of adding packets to the buffer, leading to a memory overflow.

- **Root Cause**:
    - The buffer fills up faster than the system can process the packets, causing memory exhaustion.
    - GCC_PHAT needs more memory with bigger `TIME_WINDOW`

- **Potential Solution**:
    - Adjust the minimum allocated heap size in `memmap_default.ld`. However, the compiler can only allocate up to 125KB for heap memory, which limits the available space.
    - Reducing `TIME_WINDOW` in `custom_types.h`
    - Make listener also decode the packet before appending into a queue

---

### Issue 2: Lack of Persistent Storage
- **Problem**:
    - The Raspberry Pi Pico W lacks onboard storage to save output files.
    - The current setup requires an SD card and SD card adaptor to write files, which adds complexity.

- **Current Behavior**:
    - Instead of writing output to a file, the system currently only prints messages.

- **Potential Solution**:
  - Writing to SD card. Need to refer to pico w documentation.



---

### Issue 3: No Error Handling (No Try-Catch Mechanism)
- **Problem**:
    - The Raspberry Pi Pico W does not have a built-in `try-catch` mechanism for error handling.
    - When an exception occurs, the system hangs and stops functioning.

- **Future Work**:
    - Explore alternative methods for handling errors on the Pico W to prevent the system from hanging when an exception is encountered.
