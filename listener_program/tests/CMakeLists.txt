# Create the test executable
#add_executable(UnitTests process_data_test.cpp TDOA_estimation_test.cpp utils_test.cpp)



# Recursively find all .cpp files in the tests directory
file(GLOB_RECURSE TEST_SOURCES "${CMAKE_CURRENT_SOURCE_DIR}/*.cpp")

# Add the UnitTests target with all found test sources
add_executable(UnitTests ${TEST_SOURCES})





# Include directories properly set
target_include_directories(UnitTests PRIVATE
        ${THIRD_PARTY_INCLUDE_DIRS}
        "${gtest_SOURCE_DIR}/include"
)
target_link_libraries(UnitTests PRIVATE
        gtest
        gtest_main
        MainLib
        ${THIRD_PARTY_LIBRARIES}
)

# Add tests to the testing framework
include(GoogleTest)

set_target_properties(UnitTests PROPERTIES
  RUNTIME_OUTPUT_DIRECTORY ${PROJECT_SOURCE_DIR}/bin
)

message(STATUS "ENABLE_AUTO_TEST: ${ENABLE_AUTO_TEST}")
if (ENABLE_AUTO_TEST)
    enable_testing()
    gtest_discover_tests(UnitTests)
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