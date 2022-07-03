cmake_minimum_required(VERSION 3.4)

set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} ${CMAKE_CURRENT_SOURCE_DIR}/cmake-modules)

find_package(OpenCV REQUIRED)

project(TargetToOrigin)

include_directories( ${OpenCV_INCLUDE_DIRS} )

set(TargetToOrigin_SOURCES
        TargetToOrigin.cpp)
add_executable(TargetToOrigin ${TargetToOrigin_SOURCES} PoseEstimation.cpp)
target_link_libraries (TargetToOrigin ${OpenCV_LIBS})
