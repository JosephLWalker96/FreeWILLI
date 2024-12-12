#pragma once

void normalizeDoa(Eigen::VectorXf &doaVector, int solutionRank);
auto computeDoaFromTdoa(const Eigen::MatrixXf &precomputedP, 
                                            const Eigen::MatrixXf &basisMatrixU, 
                                            const float speedOfSound, 
                                            const Eigen::VectorXf &tdoa, 
                                            int rank) -> Eigen::VectorXf;

auto convertDoaToElAz(const Eigen::VectorXf &doa) -> Eigen::VectorXf;
