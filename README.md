# Description
The aim of the Embedded mimiHARP project is to provide highly optimized C++ software that supports a broad range of data processing functionalities for underwater passive acoustics. This repo consists of two main programs:

## 1. HarpListen 
The program for receiving and processing acoustic data. 

### Features

- Dynamic configuration. The path to a JSON configuration file is provided by the user
- Logging of processed data to an output file.
- Automatic error recovery by restarting the listener and processing threads to maintain continuous operation.

## 2. DataLogger simulator
This program simulates the operation of a four channel data logger by reading prerecorded four channel data from a specified file, formatting it according to a chosen firmware version, and sending it as UDP packets to a specified IP address and port in which the HarpListen program is listening on. The purpose of this program is to provide ease of development and testing of HarpListen.

## Installation

### 1. Initialize and Update Submodules
Run the following commands from the root of the repository:

```bash
git submodule init
git submodule update
```

### 2. Ensure Recursive Submodule Initialization

```bash
git submodule update --init --recursive
```

### Requirements

- **FFTW3**: Required for performing fast Fourier transforms (FFTs).
- **Eigen**: Used for linear algebra operations, such as matrix manipulations and decompositions.

## Example Usage

rm -r out/ && mkdir out/ && cd out
cmake ..
make
cd ..
./bin/HarpListen config_files/volumetric.json 50000
