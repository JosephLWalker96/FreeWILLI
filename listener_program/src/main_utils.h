#pragma once
#include "pch.h"
// forward declarations
class RuntimeConfig;
class FirmwareConfig;
class SocketManager;

void printMode();

void parseJsonConfig(RuntimeConfig &runtimeConfig, const std::string& jsonFilePath);

void initializeRuntimeObjects(RuntimeConfig &runtimeConfig, const FirmwareConfig &firmwareConfig);
