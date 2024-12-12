#pragma once
#include "pch.h"
// forward declarations
class RuntimeConfig;
class FirmwareConfig;
class SocketManager;

void printMode();

void parseJsonConfig(FirmwareConfig &firmwareConfig, RuntimeConfig &runtimeConfig, const std::string &jsonFilePath);