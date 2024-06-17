'''

Example test run:
bazel test //:TDOA_estimation_test  --cxxopt=-std=c++20

View the log file of the test:
vi bazel-testlogs/TDOA_estimation_test/test.log

'''

load("@rules_cc//cc:defs.bzl", "cc_binary", "cc_test")

cc_library(
    name = "filters",
    srcs = ["src/filters.cpp", "src/custom_types.h", "src/filters.h", "src/utils.h"],
    # Add the linker flags here
    linkopts = ["-lopenblas", "-llapack","-lfftw3", "-flto"],
    copts = ["-I/usr/include/sigpack-1.2.7/sigpack"],
)

cc_library(
    name = "utils",
    srcs = ["src/filters.cpp", "src/custom_types.h", "src/filters.h", "src/utils.h", "src/utils.cpp", "src/main.cpp", "src/process_data.h", "src/process_data.cpp", "src/TDOA_estimation.cpp",  "src/TDOA_estimation.h"],
    # Add the linker flags here
    linkopts = ["-lopenblas", "-llapack","-lfftw3", "-flto"],
    copts = ["-I/usr/include/sigpack-1.2.7/sigpack"],
)

cc_library(
    name = "process_data",
    srcs = ["src/custom_types.h", "src/process_data.h", "src/utils.h"],
    # Add the linker flags here
)

cc_test(
  name = "filters_test",
  srcs = ["unit_tests/filters_test.cpp", "src/filters.cpp"],
  deps = [
    "@com_google_googletest//:gtest_main",
    ":filters",  # Replace with your Armadillo dependency name
  ],
  linkopts = ["-larmadillo"],
  includes = ["/usr/include/sigpack-1.2.7/sigpack"],
  copts = ["-I/usr/include/sigpack-1.2.7/sigpack"],
)


cc_test(
  name = "utils_test",
  srcs = ["unit_tests/utils_test.cpp", "src/utils.cpp"],
  deps = [
    "@com_google_googletest//:gtest_main",
    ":utils",
  ],
  linkopts = ["-larmadillo"],
)

cc_test(
  name = "process_data_test",
  srcs = ["unit_tests/process_data_test.cpp", "src/process_data.cpp"],
  deps = [
    "@com_google_googletest//:gtest_main",
    ":utils",
  ],
  linkopts = ["-larmadillo"],
)

cc_test(
  name = "TDOA_estimation_test",
  srcs = ["unit_tests/TDOA_estimation_test.cpp", "src/TDOA_estimation.cpp"],
  deps = [
    "@com_google_googletest//:gtest_main",
    ":utils",
  ],
  linkopts = ["-larmadillo"],
  copts = ["-I/usr/include/sigpack-1.2.7/sigpack"],
)
