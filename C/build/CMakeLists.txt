cmake_minimum_required(VERSION 3.22)

project(myproject)

add_executable(${PROJECT_NAME} main.cpp)

add_library(mylib SHARED lib.cpp)

target_link_libraries(${PROJECT_NAME} my_lib)

