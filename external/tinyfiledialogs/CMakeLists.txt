cmake_minimum_required(VERSION 3.1)
project(tinyfiledialogs CXX)

add_library(${PROJECT_NAME} STATIC tinyfiledialogs.cpp)
target_include_directories(${PROJECT_NAME} PUBLIC include)