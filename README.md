## Description
The aim of the Embedded mimiHARP project is to provide highly optimized C++ software that supports a broad range of real-time data processing functionalities for underwater autonomous vehicle. This repo consists of two main programs:

# 1) HarpListen 
is a the executable designed to receive and process UDP packets efficiently. The program sets up a UDP listener that captures packets from a specified IP address and port, storing the received data in a shared buffer for subsequent processing. 

# 2) DataLogger simulator
is a program which simulates the operation of a four channel data logger by reading prerecorded four channel data from a specified file, formatting it according to a chosen firmware version, and sending it as UDP packets to a specified IP address and port.

### Features

- Dynamic configuration based on specified firmware versions, with adjustments to processing parameters as needed.
- Logging of processed data to an output file for further analysis.
- Automatic error recovery by restarting the listener and processing threads to maintain continuous operation.

### Requirements

- **FFTW3**: Required for performing fast Fourier transforms (FFTs).
- **Eigen**: Used for linear algebra operations, such as matrix manipulations and decompositions.

## Example Usage

rm -r out/ && mkdir out/ && cd out
cmake ..
make
cd ..
./bin/HarpListen config_files/volumetric.json 50000
