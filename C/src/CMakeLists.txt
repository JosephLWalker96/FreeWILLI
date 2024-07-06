add_library(MainLib STATIC filters.cpp process_data.cpp TDOA_estimation.cpp utils.cpp)

add_executable(HarpListen main.cpp)

# Set the output directory for the executable
set_target_properties(HarpListen PROPERTIES
  RUNTIME_OUTPUT_DIRECTORY ${PROJECT_SOURCE_DIR}/bin
)

#######target_link_libraries(MainLib PUBLIC SigPack)
target_link_libraries(HarpListen PRIVATE MainLib)

# Linking external libraries

# Armadillo
find_package(Armadillo REQUIRED PATH_SUFFIXES /opt/homebrew/Cellar/)
message(STATUS "Armadillo libraries: ${ARMADILLO_LIBRARIES}")
message(STATUS "Armadillo include dirs: ${ARMADILLO_INCLUDE_DIRS}")
include_directories(${ARMADILLO_INCLUDE_DIRS})
target_link_libraries(HarpListen PRIVATE ${ARMADILLO_LIBRARIES})

# LAPACK
find_package(LAPACK REQUIRED)
message(STATUS "LAPACK libraries: ${LAPACK_LIBRARIES}")
message(STATUS "LAPACK include dirs: ${LAPACK_INCLUDE_DIRS}")
target_link_libraries(HarpListen PRIVATE ${LAPACK_LIBRARIES})

# Liquid
find_library(LIQUID_LIBRARIES NAMES liquid REQUIRED)
message(STATUS "LIQUID libraries: ${LIQUID_LIBRARIES}")
target_link_libraries(HarpListen PRIVATE ${LIQUID_LIBRARIES})

find_path(LIQUID_INCLUDE_DIRS NAMES liquid REQUIRED PATH_SUFFIXES /opt/homebrew/Cellar/)
message(STATUS "LIQUID include dirs: ${LIQUID_INCLUDE_DIRS}")
include_directories(HarpListen PRIVATE ${LIQUID_INCLUDE_DIRS})

# FFTW3
find_library(FFTW_LIBRARIES NAMES fftw3 REQUIRED)
message(STATUS "FFTW libraries: ${FFTW_LIBRARIES}")
target_link_libraries(HarpListen PRIVATE ${FFTW_LIBRARIES})

find_path(FFTW_INCLUDE_DIRS NAMES fftw3.h REQUIRED PATH_SUFFIXES /opt/homebrew/Cellar/)
message(STATUS "FFTW include dirs: ${FFTW_INCLUDE_DIRS}")
include_directories(HarpListen PRIVATE ${FFTW_INCLUDE_DIRS})

# BLAS
find_package(BLAS REQUIRED)
message(STATUS "BLAS libraries: ${BLAS_LIBRARIES}")
message(STATUS "BLAS include dirs: ${BLAS_INCLUDE_DIRS}")
target_link_libraries(HarpListen PRIVATE ${BLAS_LIBRARIES})

# Eigen3
find_package (Eigen3 REQUIRED NO_MODULE)
message(STATUS "Eigen3 libraries: ${EIGEN3_LIBRARIES}")
message(STATUS "Eigen3 include dir: ${EIGEN3_INCLUDE_DIR}")
target_link_libraries(HarpListen PRIVATE Eigen3::Eigen)
