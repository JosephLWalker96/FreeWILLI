#include <pybind11/pybind11.h>
#include <pybind11/stl.h>
#include <pybind11/eigen.h> // Add support for Eigen/NumPy conversion
#include <pybind11/numpy.h>
#include <vector>
#include <span>
#include <eigen3/Eigen/Dense>
#include "kalman_filter.h"
#include "tracker.h"
// Assuming the definition of point2 is as follows:
/*
struct point2
{
    float x;
    float y;
};
*/

// Assuming dbscan function has been defined as follows:
std::vector<std::vector<size_t>> dbscan(const std::span<const point3> &data, float eps, int min_pts);

// Helper function to convert NumPy array to std::vector<point2>
std::vector<point3> numpy_to_point2_vector(pybind11::array_t<float> numpy_array)
{
    if (numpy_array.ndim() != 2 || numpy_array.shape(1) != 3)
    {
        throw std::runtime_error("Input array must be a 2D array with shape (N, 2).");
    }

    std::vector<point3> points;
    auto r = numpy_array.unchecked<2>(); // Get access to raw array data

    for (ssize_t i = 0; i < r.shape(0); ++i)
    {
        points.push_back(point3{r(i, 0), r(i, 1), r(i, 2)});
    }

    return points;
}

// Define the label function
std::vector<size_t> label(const std::vector<std::vector<size_t>> &clusters, size_t n)
{
    std::vector<size_t> flat_clusters(n);

    for (size_t i = 0; i < clusters.size(); i++)
    {
        for (auto p : clusters[i])
        {
            flat_clusters[p] = i + 1; // Assign cluster labels (1-indexed)
        }
    }

    return flat_clusters;
}

namespace py = pybind11;

PYBIND11_MODULE(dbscan_module, m)
{
    m.def("dbscan", [](py::array_t<float> data, float eps, int min_pts)
          {
        // Convert the numpy array to std::vector<point2>
        std::vector<point3> points = numpy_to_point2_vector(data);
        
        // Convert std::vector<point2> to std::span<const point2>
        std::span<const point3> data_span(points);
        
        // Call the dbscan function
        std::vector<std::vector<size_t>> result = dbscan(data_span, eps, min_pts);
        
        return result; }, py::arg("data"), py::arg("eps"), py::arg("min_pts"));

    // Expose the label function to Python
    m.def("label", &label, py::arg("clusters"), py::arg("n"));

    py::class_<KalmanFilter>(m, "KalmanFilter")
        .def(pybind11::init<const Eigen::Vector3d &>())
        .def("predict", &KalmanFilter::predict)
        .def("update", &KalmanFilter::update)
        .def("filter_update", &KalmanFilter::filter_update)
        .def_property_readonly("H", &KalmanFilter::getH)            // Expose H
        .def_property_readonly("x_prior", &KalmanFilter::getXPrior) // Expose x_prior
        .def_property_readonly("x", &KalmanFilter::getX);           // Expose x (state estimate)

    // Expose LogEntry struct
    py::class_<LogEntry>(m, "LogEntry")
        .def(py::init<double, int, double, double>())
        .def_readwrite("time", &LogEntry::time)
        .def_readwrite("filter_id", &LogEntry::filter_id)
        .def_readwrite("updated_x", &LogEntry::updated_x)
        .def_readwrite("updated_y", &LogEntry::updated_y);

    py::class_<Tracker>(m, "Tracker")
        .def(py::init<double, int, int>())
        .def("update_kalman_filters", &Tracker::update_kalman_filters)
        .def("process_batch", &Tracker::process_batch)
        .def("update_kalman_filters_continuous", &Tracker::update_kalman_filters_continuous)
        .def_readwrite("kalman_log", &Tracker::kalman_log); // Expose kalman_log
}
