project(SigPack)

add_library(SigPack INTERFACE)

# Specify the include directories for this target
target_include_directories(SigPack INTERFACE ${CMAKE_CURRENT_SOURCE_DIR})

# Armadillo
find_package(Armadillo REQUIRED PATH_SUFFIXES /opt/homebrew/Cellar/)
# message(STATUS "Armadillo libraries: ${ARMADILLO_LIBRARIES}")
# message(STATUS "Armadillo include dirs: ${ARMADILLO_INCLUDE_DIRS}")
include_directories(${ARMADILLO_INCLUDE_DIRS})
target_link_libraries(SigPack INTERFACE ${ARMADILLO_LIBRARIES})

# LAPACK
find_package(LAPACK REQUIRED)
# message(STATUS "LAPACK libraries: ${LAPACK_LIBRARIES}")
# message(STATUS "LAPACK include dirs: ${LAPACK_INCLUDE_DIRS}")
target_link_libraries(SigPack INTERFACE ${LAPACK_LIBRARIES})

# Liquid
find_library(LIQUID_LIBRARIES NAMES liquid REQUIRED)
# message(STATUS "LIQUID libraries: ${LIQUID_LIBRARIES}")
target_link_libraries(SigPack INTERFACE ${LIQUID_LIBRARIES})

find_path(LIQUID_INCLUDE_DIRS NAMES liquid REQUIRED PATH_SUFFIXES /opt/homebrew/Cellar/)
# message(STATUS "LIQUID include dirs: ${LIQUID_INCLUDE_DIRS}")
include_directories(SigPack INTERFACE ${LIQUID_INCLUDE_DIRS})

# FFTW3
find_library(FFTW_LIBRARIES NAMES fftw3 REQUIRED)
# message(STATUS "FFTW libraries: ${FFTW_LIBRARIES}")
target_link_libraries(SigPack INTERFACE ${FFTW_LIBRARIES})

find_path(FFTW_INCLUDE_DIRS NAMES fftw3.h REQUIRED PATH_SUFFIXES /opt/homebrew/Cellar/)
# message(STATUS "FFTW include dirs: ${FFTW_INCLUDE_DIRS}")
include_directories(SigPack INTERFACE ${FFTW_INCLUDE_DIRS})


# BLAS
find_package(BLAS REQUIRED)
# message(STATUS "BLAS libraries: ${BLAS_LIBRARIES}")
# message(STATUS "BLAS include dirs: ${BLAS_INCLUDE_DIRS}")
target_link_libraries(SigPack INTERFACE ${BLAS_LIBRARIES})

# Eigen3
find_package (Eigen3 REQUIRED NO_MODULE)
target_link_libraries(SigPack INTERFACE Eigen3::Eigen)
