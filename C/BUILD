'''

bazel test //:filter_test --cxxopt=-std=c++20

'''

load("@rules_cc//cc:defs.bzl", "cc_binary", "cc_test")


cc_library(
    name = "filters",
    srcs = ["filters.cpp", "my_globals.h", "filters.h", "utils.h"],
    # Add the linker flags here
    linkopts = ["-lopenblas", "-llapack","-lfftw3", "-flto"],
    copts = ["-I/usr/include/sigpack-1.2.7/sigpack"],
)

cc_library(
    name = "utils",
    srcs = ["filters.cpp", "my_globals.h", "filters.h", "utils.h", "utils.cpp", "multi_datalogger_reader.cpp", "process_data.h", "process_data.cpp", "TDOA_estimation.cpp",  "TDOA_estimation.h"],
    # Add the linker flags here
    linkopts = ["-lopenblas", "-llapack","-lfftw3", "-flto"],
    copts = ["-I/usr/include/sigpack-1.2.7/sigpack"],
)

cc_library(
    name = "process_data",
    srcs = ["my_globals.h", "process_data.h", "utils.h"],
    # Add the linker flags here
)

cc_test(
  name = "filters_test",
  srcs = ["Unit_tests/filters_test.cpp", "filters.cpp"],
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
  srcs = ["Unit_tests/utils_test.cpp", "utils.cpp"],
  deps = [
    "@com_google_googletest//:gtest_main",
    ":utils",
  ],
  linkopts = ["-larmadillo"],
)

cc_test(
  name = "process_data_test",
  srcs = ["Unit_tests/process_data_test.cpp", "process_data.cpp"],
  deps = [
    "@com_google_googletest//:gtest_main",
    ":utils",
  ],
  linkopts = ["-larmadillo"],
)
