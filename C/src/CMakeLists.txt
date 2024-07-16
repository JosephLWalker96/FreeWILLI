add_library(MainLib STATIC process_data.cpp TDOA_estimation.cpp utils.cpp)

add_executable(HarpListen main.cpp)

# Set the output directory for the executable
set_target_properties(HarpListen PROPERTIES
  RUNTIME_OUTPUT_DIRECTORY ${PROJECT_SOURCE_DIR}/bin
)

set(THIRD_PARTY_LIBRARIES)
set(THIRD_PARTY_INCLUDE_DIRS)

# LAPACK
find_package(LAPACK REQUIRED)
if (LAPACK_LIBRARIES)
    list(APPEND THIRD_PARTY_LIBRARIES ${LAPACK_LIBRARIES})
    message(STATUS "LAPACK libraries: ${LAPACK_LIBRARIES}")
endif ()
if (LAPACK_INCLUDE_DIRS)
    list(APPEND THIRD_PARTY_INCLUDE_DIRS ${LAPACK_INCLUDE_DIRS})
    message(STATUS "LAPACK include dirs: ${LAPACK_INCLUDE_DIRS}")
endif()


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

target_precompile_headers(HarpListen PRIVATE
    pch.h
)
# Debug output to ensure CMake is processing precompile headers command
get_target_property(PCH_SOURCES HarpListen PRECOMPILE_HEADERS)
message(STATUS "Precompiled headers: ${PCH_SOURCES}")

include_directories(HarpListen PRIVATE
        ${THIRD_PARTY_INCLUDE_DIRS}
)
target_link_libraries(HarpListen PRIVATE
        MainLib
        ${THIRD_PARTY_LIBRARIES}
)
