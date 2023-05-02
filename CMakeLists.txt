cmake_minimum_required(VERSION 3.17)
project(DSML)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_BUILD_TYPE Debug)

include_directories(include/)

add_library(dsml src/dsml.cpp)

add_executable(test test/test.cpp)
add_executable(demo demo/demo.cpp)
add_executable(demo2 demo/demo2.cpp)

target_link_libraries(test dsml)
target_link_libraries(demo dsml)
target_link_libraries(demo2 dsml)
