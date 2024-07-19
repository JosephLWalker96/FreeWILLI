project(Benchmark)
add_executable(Benchmark
        HelloBenchmark.cpp
        process_data_benchmark.cpp
        TDOA_estimation_test.cpp
)
# Include directories properly set
target_include_directories(Benchmark PRIVATE
        ${THIRD_PARTY_INCLUDE_DIRS}
)
target_link_libraries(Benchmark PRIVATE
        benchmark::benchmark
        benchmark::benchmark_main
        MainLib
        ${THIRD_PARTY_LIBRARIES}
)

set_target_properties(Benchmark PROPERTIES
        RUNTIME_OUTPUT_DIRECTORY ${PROJECT_SOURCE_DIR}/bin
)
