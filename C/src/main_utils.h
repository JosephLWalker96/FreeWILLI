#pragma once

// forward declarations
class RuntimeConfig;
class FirmwareConfig;
class SocketManager;

void printMode();

void parseJsonConfig(SocketManager &socketManager, RuntimeConfig &runtimeConfig, char *argv[]);

void initializeRuntimeObjects(RuntimeConfig &runtimeConfig, const FirmwareConfig &firmwareConfig);
