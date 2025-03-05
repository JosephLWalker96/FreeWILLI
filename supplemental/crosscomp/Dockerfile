# Use Ubuntu as the base image
FROM ubuntu:22.04

# Set the working directory inside the container
WORKDIR /app

# Install necessary dependencies
RUN apt-get update && apt-get install -y \
    wget \
    ninja-build \
    make \
    gcc-aarch64-linux-gnu \
    g++-aarch64-linux-gnu \
    rsync \
    && rm -rf /var/lib/apt/lists/*

# Install CMake 3.29.7
RUN wget https://github.com/Kitware/CMake/releases/download/v3.29.7/cmake-3.29.7-linux-x86_64.sh && \
    chmod +x cmake-3.29.7-linux-x86_64.sh && \
    ./cmake-3.29.7-linux-x86_64.sh --skip-license --prefix=/usr/local && \
    rm cmake-3.29.7-linux-x86_64.sh

# Copy the sysroot into the container
COPY rpi-sysroot /rpi-sysroot

# Set environment variables for toolchain
ENV CMAKE_SYSROOT=/rpi-sysroot

# Default command: Start an interactive shell
CMD ["/bin/bash"]

