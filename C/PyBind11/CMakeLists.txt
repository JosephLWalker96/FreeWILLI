#cmake -DPython3_EXECUTABLE=$(which python) ..


cmake_minimum_required(VERSION 3.14)
project(DBSCANModule)

# Set the C++ standard
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Find Python3 and PyBind11
find_package(Python3 COMPONENTS Interpreter Development NumPy REQUIRED)
find_package(pybind11 REQUIRED)

# Include directories for Python and PyBind11
include_directories(${Python3_INCLUDE_DIRS})
include_directories(${pybind11_INCLUDE_DIRS})
include_directories(/home/harp/Documents/Embedded_miniHarp/C/libs/dbscan)  # Add the dbscan directory for nanoflann
include_directories(/home/harp/Documents/Embedded_miniHarp/C/libs/dbscan/vendor)
# Add the Eigen3 include directory
include_directories("/usr/include/eigen3")  # Add Eigen include path
# Specify the source files for your project
set(SOURCES
    /home/harp/Documents/Embedded_miniHarp/C/libs/dbscan/dbscan.cpp      # Your C++ DBSCAN implementation
    #/home/harp/Documents/Embedded_miniHarp/C/libs/dbscan/example.cpp      # Your C++ DBSCAN implementation
    dbscan_wrapper.cpp        # PyBind11 wrapper script
    kalman_filter.cpp        # PyBind11 wrapper script
    tracker.cpp
)

# Eigen3
find_package (Eigen3 REQUIRED NO_MODULE)
if (EIGEN3_LIBRARIES)
    list(APPEND THIRD_PARTY_LIBRARIES ${EIGEN3_LIBRARIES})
    message(STATUS "Eigen3 libraries: ${EIGEN3_LIBRARIES}")
endif ()
if (EIGEN3_INCLUDE_DIR)
    list(APPEND THIRD_PARTY_INCLUDE_DIRS ${EIGEN3_INCLUDE_DIRS})
    message(STATUS "Eigen3 include dir: ${EIGEN3_INCLUDE_DIR}")
endif()


# Create the Python module using pybind11
pybind11_add_module(dbscan_module ${SOURCES})

# Link libraries, if needed
target_link_libraries(dbscan_module PRIVATE ${Python3_LIBRARIES})

# Include NumPy headers
target_include_directories(dbscan_module PRIVATE ${Python3_NumPy_INCLUDE_DIRS})

# Set additional compile options if necessary (optional)
# For example, enabling optimization or warnings:
# target_compile_options(dbscan_module PRIVATE -O3 -Wall)