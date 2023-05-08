cmake_minimum_required(VERSION 3.17)
project(DSML)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_BUILD_TYPE Debug)

include_directories(include/)

add_library(dsml src/dsml.cpp)

add_executable(test test/test.cpp)
target_link_libraries(test dsml)

# Demo Executables

find_package(OpenCV)

add_executable(video_demo demo/video.cpp)
target_include_directories(video_demo PRIVATE ${OpenCV_INCLUDE_DIRS})
target_link_libraries(video_demo dsml ${OpenCV_LIBS})

add_executable(process_demo demo/process.cpp)
target_include_directories(process_demo PRIVATE /usr/local/include/apriltag)
target_link_libraries(process_demo dsml /usr/local/lib/libapriltag.so)
