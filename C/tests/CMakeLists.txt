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
add_executable(UnitTests filters_test.cpp process_data_test.cpp TDOA_estimation_test.cpp utils_test.cpp)

# Finding and linking external libraries
find_package(LAPACK REQUIRED)

find_library(LIQUID_LIBRARIES NAMES liquid REQUIRED)
find_path(LIQUID_INCLUDE_DIRS NAMES liquid.h REQUIRED PATH_SUFFIXES /opt/homebrew/Cellar/)

find_library(FFTW_LIBRARIES NAMES fftw3 REQUIRED)
find_path(FFTW_INCLUDE_DIRS NAMES fftw3.h REQUIRED PATH_SUFFIXES /opt/homebrew/Cellar/)

find_package(BLAS REQUIRED)

find_package(Eigen3 REQUIRED NO_MODULE)

# Include directories properly set
target_include_directories(UnitTests PRIVATE
  ${ARMADILLO_INCLUDE_DIRS}
  ${LIQUID_INCLUDE_DIRS}
  ${FFTW_INCLUDE_DIRS}
  ${gtest_SOURCE_DIR}/include  # Only needed if GoogleTest is built from source
)

# Linking all libraries
target_link_libraries(UnitTests PRIVATE
  gtest_main
  MainLib
  ${ARMADILLO_LIBRARIES}
  ${LAPACK_LIBRARIES}
  ${LIQUID_LIBRARIES}
  ${FFTW_LIBRARIES}
  ${BLAS_LIBRARIES}
  Eigen3::Eigen
)

# Add tests to the testing framework
include(GoogleTest)
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
