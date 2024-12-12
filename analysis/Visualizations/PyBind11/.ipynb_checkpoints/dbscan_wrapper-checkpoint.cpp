#include <pybind11/pybind11.h>
#include <pybind11/stl.h>
#include <pybind11/numpy.h>
#include <vector>
#include <span>

// Assuming the definition of point2 is as follows:
struct point2 {
    float x;
    float y;
};

// Assuming dbscan function has been defined as follows:
std::vector<std::vector<size_t>> dbscan(const std::span<const point2>& data, float eps, int min_pts);

// Helper function to convert NumPy array to std::vector<point2>
std::vector<point2> numpy_to_point2_vector(pybind11::array_t<float> numpy_array) {
    if (numpy_array.ndim() != 2 || numpy_array.shape(1) != 2) {
        throw std::runtime_error("Input array must be a 2D array with shape (N, 2).");
    }

    std::vector<point2> points;
    auto r = numpy_array.unchecked<2>();  // Get access to raw array data

    for (ssize_t i = 0; i < r.shape(0); ++i) {
        points.push_back(point2{r(i, 0), r(i, 1)});
    }

    return points;
}

namespace py = pybind11;

PYBIND11_MODULE(dbscan_module, m) {
    m.def("dbscan", [](py::array_t<float> data, float eps, int min_pts) {
        // Convert the numpy array to std::vector<point2>
        std::vector<point2> points = numpy_to_point2_vector(data);
        
        // Convert std::vector<point2> to std::span<const point2>
        std::span<const point2> data_span(points);
        
        // Call the dbscan function
        std::vector<std::vector<size_t>> result = dbscan(data_span, eps, min_pts);
        
        return result;
    }, py::arg("data"), py::arg("eps"), py::arg("min_pts"));
}
