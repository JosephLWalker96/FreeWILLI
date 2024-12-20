# Overview
### Free software for Whale Identification and Localization with Low-power Implementation:

The Free WILLI project aims to deliver high-performance C++ software for soft real-time, low-power underwater passive acoustic array data processing. It supports automated multi-target tracking from small volumetric four-channel hydrophone arrays and offers integrated functionality for neural network inference. Designed for efficiency and flexibility, the system enables advanced acoustic analyses on resource-constrained devices.

This repo consists of two main programs:

1. **HarpListen**: A multi-target tracking application for receiving and processing acoustic data.

2. **DataLogger Simulator**: A utility for simulating a four-channel data logger by sending pre-recorded data as UDP packets for testing and development purposes

## Repository Structure
The repository is organized as follows:

- .github/workflows/: Contains GitHub Actions workflows for continuous integration and deployment. 
- .vscode/: Configuration files for Visual Studio Code, including workspace settings and extensions.
- analysis/: Scripts and tools for analyzing and visualizing data outputs from HarpListen.
- listener_program/: Source code and resources for the HarpListen program.
- simulator_program/: Source code and resources for the DataLogger Simulator program. 
- supplemental/: Additional files, documentation, or supporting scripts.
- .gitignore: Specifies files and directories to be ignored by Git.
- .gitmodules: Configuration for managing Git submodules.
- CONTRIBUTING.md: Guidelines for contributing to the project.
- README.md: The main README file, providing an overview of the project and setup instructions.


## Installation and build for HarpListen

### Prerequisites

- **CMake**: A build system generator used to configure and build the project across multiple platforms.
- **FFTW3**: Required for performing fast Fourier transforms.
- **Eigen**: Used for advanced linear algebra operations, such as matrix manipulations and decompositions.
- **nlohmann-json**: A JSON library for parsing and managing configuration files.
- **ONNX Runtime**: Used for running machine learning models in the program.

### Installing Dependencies on Ubuntu/Debian
1. Example Installing CMake 3.29.7 on Linux x86
Use wget to download the precompiled binary from the official CMake website:
```bash
wget https://github.com/Kitware/CMake/releases/download/v3.29.7/cmake-3.29.7-linux-x86_64.sh
```

Make the downloaded file executable and run it to install CMake:
```bash
chmod +x cmake-3.29.7-linux-x86_64.sh
sudo ./cmake-3.29.7-linux-x86_64.sh --skip-license --prefix=/usr/local
```

Confirm the installed version of CMake:
```bash
cmake --version
```

2. Install the required libraries using apt:
```bash
sudo apt-get update
sudo apt-get install -y build-essential libfftw3-dev libeigen3-dev nlohmann-json3-dev
```

3. Download and install ONNX Runtime
```bash
wget https://github.com/microsoft/onnxruntime/releases/download/v1.14.1/onnxruntime-linux-x64-1.14.1.tgz
tar -xzf onnxruntime-linux-x64-1.14.1.tgz
sudo cp -r onnxruntime-linux-x64-1.14.1/include/* /usr/local/include/
sudo cp -r onnxruntime-linux-x64-1.14.1/lib/* /usr/local/lib/
```

### Installing Dependencies on macOS
1. Example Installing CMake 3.29.7 on macOS
Use curl to download the precompiled binary from the official CMake website:
```bash
curl -L -o cmake-3.29.7-macos-universal.dmg https://github.com/Kitware/CMake/releases/download/v3.29.7/cmake-3.29.7-macos-universal.dmg
```

Mount the .dmg file to access the installer:
```bash
hdiutil attach cmake-3.29.7-macos-universal.dmg
```

Open the mounted volume and drag the CMake application to your preferred location (e.g., /Applications

Add CMake to PATH
```bash
sudo "/Applications/CMake.app/Contents/bin/cmake" /usr/local/bin/cmake
```

Confirm the installed version of CMake:
```bash
cmake --version
```


1. Install the required libraries using brew:
```bash
brew install fftw eigen nlohmann-json
```

2. Download and install ONNX Runtime
```bash
curl -L -o onnxruntime-osx-arm64-1.19.2.tgz https://github.com/microsoft/onnxruntime/releases/download/v1.19.2/onnxruntime-osx-arm64-1.19.2.tgz
tar -xzf onnxruntime-osx-arm64-1.19.2.tgz
sudo mkdir -p /usr/local/include/onnxruntime /usr/local/lib
sudo cp -r onnxruntime-osx-arm64-1.19.2/include/* /usr/local/include/onnxruntime/
sudo cp -r onnxruntime-osx-arm64-1.19.2/lib/* /usr/local/lib/
```

### Build program (Ubuntu/Debian & macOS)

1. Clone the repository:

``` bash
git clone https://github.com/JosephLWalker96/Embedded_miniHarp.git
cd Embedded_miniHarp
```

2. Initialize and Update Submodules:
Run the following commands from the root of the repository:

```bash
git submodule init
git submodule update --init --recursive
```

3. Build the program:
```bash
cd listener_program/
mkdir out/ && cd out
cmake ..
make -j$(nproc)
```

4. Run the binary:
```bash
cd ..
./bin/HarpListen config_files/volumetric.json 50000
```

### Installation and build for DataLogger Simulator
1. Install dependencies:
```bash
cd simulator_program/
conda create --name freewilli python=3.9
conda activate freewilli
pip install -r requirements.txt
```

2. Download data:
By default, the program reads in data from folder **simulator_data/track132_5minchunks/**

[Link Download](https://drive.google.com/drive/folders/1v8sgYyQATcsUkzAI6AcUaiMpq5Wi37y1?usp=sharing) the track132_5minchunks/ folder and place it in the simulator_data/ directory.

3. Run example:
```bash
cd simulator_program/
```
Assuming you are running the simulator on the same machine as the listener program, run the following:
```bash
python datalogger_simulator.py --ip self --fw 1240 --port 1045
```

Otherwise, specify the correct IP address (--ip) and port (--port).