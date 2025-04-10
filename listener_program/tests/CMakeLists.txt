# Create the test executable
#add_executable(UnitTests process_data_test.cpp TDOA_estimation_test.cpp utils_test.cpp)

# Create the test executable
#add_executable(UnitTests process_data_test.cpp TDOA_estimation_test.cpp utils_test.cpp)


# Recursively find all .cpp files in the tests directory
file(GLOB_RECURSE TEST_SOURCES "${CMAKE_CURRENT_SOURCE_DIR}/*.cpp")

# Add the UnitTests target with all found test sources
add_executable(UnitTests ${TEST_SOURCES})

# Ensure GoogleTest is included only once
include(FetchContent)
if (NOT TARGET gtest)
    FetchContent_Declare(
        googletest
        SOURCE_DIR "${CMAKE_SOURCE_DIR}/libs/googletest"
    )
    FetchContent_MakeAvailable(googletest)
endif()

# Include directories for GoogleTest & GoogleMock
target_include_directories(UnitTests PRIVATE
        ${THIRD_PARTY_INCLUDE_DIRS}
        "${gtest_SOURCE_DIR}/include"
        "${gmock_SOURCE_DIR}/include"
)

# Link against GoogleTest & GoogleMock
target_link_libraries(UnitTests PRIVATE
        gtest
        gtest_main
        gmock
        gmock_main
        MainLib
        ${THIRD_PARTY_LIBRARIES}
)

# Enable GoogleTest Framework
# Enable GoogleTest Framework only if auto test is enabled
if (ENABLE_AUTO_TEST)
    include(GoogleTest)
    gtest_discover_tests(UnitTests)
endif()

set_target_properties(UnitTests PROPERTIES
  RUNTIME_OUTPUT_DIRECTORY ${PROJECT_SOURCE_DIR}/bin
)

# Enable automatic test execution
message(STATUS "ENABLE_AUTO_TEST: ${ENABLE_AUTO_TEST}")
if (ENABLE_AUTO_TEST)
    enable_testing()
    add_test(NAME UnitTests COMMAND UnitTests)

    # Custom target to run tests automatically after building
    add_custom_target(
      run_tests ALL                      # Runs every time the project builds
      COMMAND ${CMAKE_COMMAND} --build . --target UnitTests
      COMMAND UnitTests                  # Execute the tests after building
      DEPENDS UnitTests                  # Ensures UnitTests is built before running
      WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
      COMMENT "Running unit tests..."
    )

    add_dependencies(run_tests UnitTests)
endif ()
