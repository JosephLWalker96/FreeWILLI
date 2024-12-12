# toolchain.cmake

# Specify the target system name and architecture
set(CMAKE_SYSTEM_NAME Linux)
set(CMAKE_SYSTEM_PROCESSOR aarch64)

# Specify the compilers for cross-compilation
set(CMAKE_C_COMPILER /usr/bin/aarch64-linux-gnu-gcc)
set(CMAKE_CXX_COMPILER /usr/bin/aarch64-linux-gnu-g++)

# Specify the sysroot
set(CMAKE_SYSROOT /home/harp/rpi-sysroot)

# Set the sysroot and root path
set(CMAKE_FIND_ROOT_PATH ${CMAKE_SYSROOT})

# Ensure that CMake searches within the sysroot for libraries and include files
set(CMAKE_FIND_ROOT_PATH_MODE_PROGRAM NEVER)
set(CMAKE_FIND_ROOT_PATH_MODE_LIBRARY ONLY)
set(CMAKE_FIND_ROOT_PATH_MODE_INCLUDE ONLY)
set(CMAKE_FIND_ROOT_PATH_MODE_PACKAGE ONLY)


# Enable static linking by adding static link flags
#set(CMAKE_EXE_LINKER_FLAGS "-static")
#set(CMAKE_SHARED_LIBRARY_LINK_C_FLAGS "-static")

# Specify that only static libraries should be searched for
#set(CMAKE_FIND_LIBRARY_SUFFIXES ".a")

# Set compiler flags to optimize for static linking
#set(CMAKE_C_FLAGS "-static")
#set(CMAKE_CXX_FLAGS "-static")

# Optional: Force all libraries (including C++ standard libraries) to be statically linked
# This ensures that the C++ runtime libraries are also statically linked
#set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -static-libgcc -static-libstdc++")

# Additional flags for cross-compilation (optional, adjust as necessary)
