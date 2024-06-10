/*

This file contains all function prototypes for utils.cpp

*/

#pragma once

#include <iostream>
#include <string>
#include <random>
#include <vector>
#include <armadillo>
#include "custom_types.h"

using std::string;
using std::vector;


arma::Col<double> ReadFIRFilterFile(const string& fileName);
int ProcessFile(Experiment& exp, const string fileName); 
void RestartListener(Session& sess);
void ClearQueue(std::queue<vector<uint8_t>>& q);
bool WithProbability(double probability);
