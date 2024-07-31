include(FetchContent)
FetchContent_Declare(
  googletest
  GIT_REPOSITORY https://github.com/google/googletest.git
  GIT_TAG release-1.10.0  # Specify the release you need
)
# Set the build options to skip Google Mock
set(BUILD_GMOCK OFF CACHE BOOL "" FORCE)
set(INSTALL_GMOCK OFF CACHE BOOL "" FORCE)
set(BUILD_GTEST ON CACHE BOOL "" FORCE)
set(INSTALL_GTEST ON CACHE BOOL "" FORCE)

FetchContent_MakeAvailable(googletest)

# Create the test executable
add_executable(UnitTests process_data_test.cpp TDOA_estimation_test.cpp utils_test.cpp)

set(THIRD_PARTY_LIBRARIES)
set(THIRD_PARTY_INCLUDE_DIRS)

# Finding and linking external libraries
# FFTW3
find_library(FFTW_LIBRARIES NAMES fftw3 REQUIRED)
message(STATUS "FFTW libraries: ${FFTW_LIBRARIES}")
find_path(FFTW_INCLUDE_DIRS NAMES fftw3.h REQUIRED PATH_SUFFIXES /opt/homebrew/Cellar/)
message(STATUS "FFTW include dirs: ${FFTW_INCLUDE_DIRS}")
if (FFTW_LIBRARIES)
    list(APPEND THIRD_PARTY_LIBRARIES ${FFTW_LIBRARIES})
endif ()
if (FFTW_INCLUDE_DIRS)
    list(APPEND THIRD_PARTY_INCLUDE_DIRS ${FFTW_INCLUDE_DIRS})
endif()

# FFTW3f for single precision
find_library(FFTWF_LIBRARIES NAMES fftw3f REQUIRED)
if (FFTWF_LIBRARIES)
    list(APPEND THIRD_PARTY_LIBRARIES ${FFTWF_LIBRARIES})
    message(STATUS "FFTWf libraries: ${FFTWF_LIBRARIES}")
endif ()
if (FFTWF_INCLUDE_DIRS)
    list(APPEND THIRD_PARTY_INCLUDE_DIRS ${FFTWF_INCLUDE_DIRS})
    message(STATUS "FFTWf include dirs: ${FFTWF_INCLUDE_DIRS}")
endif()

# BLAS
find_package(BLAS REQUIRED)
if (BLAS_LIBRARIES)
    list(APPEND THIRD_PARTY_LIBRARIES ${BLAS_LIBRARIES})
    message(STATUS "BLAS libraries: ${BLAS_LIBRARIES}")
endif ()
if (BLAS_INCLUDE_DIRS)
    list(APPEND THIRD_PARTY_INCLUDE_DIRS ${BLAS_INCLUDE_DIRS})
    message(STATUS "BLAS include dirs: ${BLAS_INCLUDE_DIRS}")
endif()

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


# Include directories properly set
target_include_directories(UnitTests PRIVATE
        ${THIRD_PARTY_INCLUDE_DIRS}
        ${gtest_SOURCE_DIR}/include  # Only needed if GoogleTest is built from source
)
target_link_libraries(UnitTests PRIVATE
        gtest_main
        MainLib
        ${THIRD_PARTY_LIBRARIES}
)

# Add tests to the testing framework
include(GoogleTest)
enable_testing()
gtest_discover_tests(UnitTests)

set_target_properties(UnitTests PROPERTIES
  RUNTIME_OUTPUT_DIRECTORY ${PROJECT_SOURCE_DIR}/bin
)

# Add a custom target that runs the tests every time the project is built
add_custom_target(
  run_tests ALL                      # ALL means the target is built every time
  COMMAND ${CMAKE_COMMAND} --build . --target UnitTests
  COMMAND UnitTests                  # Execute the tests after building
  DEPENDS UnitTests                  # Ensures UnitTests is built before running
  WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
  COMMENT "Running unit tests..."
)

# This ensures the UnitTests executable is up-to-date before running
add_dependencies(run_tests UnitTests)
