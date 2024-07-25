add_executable(Benchmark
#        HelloBenchmark.cpp
        process_data_benchmark.cpp
        TDOA_estimation_test.cpp
        ftt_filter_benchmark.cpp
        simulation_benchmark.cpp
        threashold_benchmark.cpp
)
# Include directories properly set
target_include_directories(Benchmark PRIVATE
        ${THIRD_PARTY_INCLUDE_DIRS}
        "${benchmark_SOURCE_DIR}/include"
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