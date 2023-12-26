# Embedded_miniHarp

|-- Docs : includes any relevant documentation
|-- HARP_UDP : includes all the code that was originally provided by Sean and Allen
|-- Python
| |-- clicks_data.txt : output file for logging detected impulses
| |-- datalogger_simulator.py : Runs the data logger simulator.
| |-- datalogger_reader.py : Reads and processes data from both the data logger simulator and the actual data logger.
| |-- multi_datalogger_reader.py : Multithreaded version of datalogger_reader.py.
| |-- process_data.py : Contains functions for processing data, such as classification and localization.
| |-- utils.py : Includes various utility functions utilized by datalogger_simulator.py.
| |-- Experimentation/
| | |-- sleep_function_experiments.ipynb : Scratch code experimenting with various sleep functions.
| | |-- sleep_script.py : Scratch code for further experimentation with different sleep functions.
| | |-- synthetic_data_experiments.py : Code for experimenting with methods to generate synthetic data.
| |-- Firmware_config/
| | |-- firmware_1240.py : defines all constants determined by firmware version 1250
| | |-- firmware_1550.py : defines all constants determined by firmware version 1550
