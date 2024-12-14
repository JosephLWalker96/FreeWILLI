# Datalogger Simulator

## Overview

This subdirectory contains a Python-based simulator (datalogger_simulator.py) that streams simulated data packets over UDP. The simulator mimics the behavior of firmware-based data logging systems. It can read .npy files, process their contents (e.g., apply channel offsets for TDOA testing, scale the data, etc.), and send out structured UDP packets for testing HarpListener program.

## File Structure
```bash
.
├── datalogger_simulator.py   # The main simulation script
├── firmware_config           # Firmware-specific configuration files
│   ├── firmware_1240.py
│   └── firmware_1550.py
├── simulator_data            # Directory containing .npy data files
├── utils.py                  # Shared utility functions (e.g., data manipulation, concurrency helpers)
└── requirements.txt          # Python dependencies needed to run the simulator
```

- **datalogger_simulator.py**
The main entry point for the simulator. It handles:
1. Command-line argument parsing (to customize data streaming IP, port, etc.).
2. Loading data from .npy files.
3. (Optionally) applying channel duplication, shifting, stretching, glitches, and more.
4. Sending the processed data over UDP.

- **firmware_config/**
1. Contains firmware-specific configurations for different firmware versions (e.g., firmware_1240.py, firmware_1550.py).
2. These Python files define constants used by the simulator (packet size, sample rate, etc.), so the simulator can mimic each firmware version’s data format.

- **simulator_data/**
Holds .npy files to be streamed by the simulator. You can store multiple .npy data files here to simulate longer runs or multiple scenarios.

- **utils.py**
Contains helper functions (e.g., reading binary data, applying channel shifts, data scaling, sleeping, etc.) that are shared by the simulator.

- **requirements.txt**
Defines the Python dependencies needed to run this simulator (e.g., NumPy, psutil, argparse, etc.).

## Program Description
The simulator is designed to emulate a real-time data capture and logging system:

1. **Firmware Emulation:**
Selects firmware configuration (e.g., firmware_1240.py) based on user arguments to determine packet structure and timing intervals.

2. **Data Streaming:**:
Reads .npy data files (which contain raw sample data).
Preprocesses the data (e.g., interleaving, scaling, time-stamping).
Sends packets over UDP to a specified IP and port.

3. **Optional Features:**

- **Looping**: Repeatedly stream the same data files in a loop.
- **TDOA Simulation**: Applies channel offsets to simulate time-difference-of-arrival.
- **Glitches**: Intentionally modifies timestamps or data packets to test receiver robustness.
- **IMU Data**: Appends IMU packets if enabled, to simulate additional sensor data.

## How to Run
1. **Install Dependencies**
From the same directory, install required dependencies:
```bash
pip install -r requirements.txt
```

2. **Prepare Data Files**

Place your .npy data files in the simulator_data/ directory (or wherever you prefer).
If using IMU functionality, ensure you have the correct IMU data files referenced in the script (adjust path in the arguments or within utils.py if needed).

3. **Run the Simulator**
Execute datalogger_simulator.py with your desired arguments.
This example will load data from simulator_data/track132_5minchunks/ and organize the data according to the firmware version 1240. The data is sent to to port 1045 at IP address 192.168.100.235 

```bash
python datalogger_simulator.py \
    --ip 192.168.100.235 \
    --port 1045 \
    --data_dir simulator_data/track132_5minchunks/ \
    --fw 1240 \
```
## Notes
**High Priority**: The simulator can set high priority for the process (using SetHighPriority in utils.py) if your system supports it.
**Multiprocessing**: Uses multiple processes to preload the next .npy file while the current one is streaming. This helps achieve smoother, more real-time data streaming.