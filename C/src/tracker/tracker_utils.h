#pragma once
#include "../pch.h"
//#include "/home/harp/Documents/Embedded_miniHarp/C/libs/dbscan/dbscan.hpp"
#include "../../libs/dbscan/dbscan.hpp"
class Tracker;
class KalmanFilter;

auto printInfo(const std::vector<Eigen::Vector3f> &clusterCenters,
               const Eigen::MatrixXf &distanceMatrix,
               const std::vector<int> &associations,
               const std::vector<int> &unassignedClusters) -> void;

auto convertDoaToElAz(float x, float y, float z) -> std::pair<float, float>;

auto labelClusters(const std::vector<std::vector<size_t>> &clusters, size_t numPoints) -> std::vector<size_t>;

auto convertEigenToPointVector(const std::vector<Eigen::VectorXf> &eigenData) -> std::vector<point3>;

auto findOptimalAssociation(const Eigen::MatrixXf &distance_matrix, double threshold) -> std::pair<std::vector<int>, std::vector<int>>;

Eigen::MatrixXf calculateDistanceMatrix(const std::vector<Eigen::Vector3f> &clusterCenters, const std::vector<KalmanFilter> &kalmanFilters);
