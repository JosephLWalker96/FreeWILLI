#pragma once

// forward declarations
class RuntimeConfig;
class SocketManager;

/**
 * @brief prints the mode (Release or Debug) that the program was compiled with
 *
 */
void printMode();

/**
 * @brief Parses a JSON configuration file and initializes session and runtime parameters.
 *
 * This function reads a JSON configuration file, extracts parameters for the `SocketManager`
 * and `ExperimentRuntime` objects, and sets up runtime values from command-line arguments.
 * If the configuration file or required fields are missing, the function throws exceptions.
 *
 * @param socketManager A reference to a `SocketManager` object to configure network settings.
 * @param experimentRuntime A reference to an `ExperimentRuntime` object to initialize runtime settings.
 * @param argv Command-line arguments, where `argv[1]` is the JSON file path and `argv[2]` is the program runtime in seconds.
 * @throws std::runtime_error If the JSON file cannot be opened or required fields are missing.
 */
void parseJsonConfig(SocketManager &socketManager, RuntimeConfig &runtimeConfig, char *argv[]);
