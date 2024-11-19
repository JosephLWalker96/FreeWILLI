
/*

This file contains all function prototypes for processor_thread.cpp

*/

#pragma once

class Session;
class ExperimentConfig;
class ExperimentRuntime;

void DataProcessor(Session &sess, ExperimentConfig &expConfig, ExperimentRuntime &expRuntime);