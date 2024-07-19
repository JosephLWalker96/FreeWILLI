# Create the test executable
add_executable(UnitTests process_data_test.cpp TDOA_estimation_test.cpp utils_test.cpp)

# Include directories properly set
target_include_directories(UnitTests PRIVATE
        ${THIRD_PARTY_INCLUDE_DIRS}
#        ${gtest_SOURCE_DIR}/include  # Only needed if GoogleTest is built from source
)
target_link_libraries(UnitTests PRIVATE
        gtest_main
        MainLib
        ${THIRD_PARTY_LIBRARIES}
)

# Add tests to the testing framework
include(GoogleTest)
gtest_discover_tests(UnitTests)

set_target_properties(UnitTests PROPERTIES
  RUNTIME_OUTPUT_DIRECTORY ${PROJECT_SOURCE_DIR}/bin
)

if (ENABLE_AUTO_TEST)
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
endif ()