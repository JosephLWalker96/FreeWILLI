/*

This file contains all function prototypes for utils.cpp

*/

#ifndef UTILS
#define UTILS

#include <iostream>
#include <string>
#include <random>
#include <armadillo>
#include "my_globals.h"

using std::string;
using std::vector;


arma::Col<double> ReadFIRFilterFile(const string& fileName);
void ProcessFile(const std::string& fileName); 
void RestartListener(Session& sess);
void ClearQueue(std::queue<std::vector<uint8_t>>& q);
bool withProbability(double probability);
#endif
