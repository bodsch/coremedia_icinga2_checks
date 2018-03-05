# Specify the minimum version for CMake

cmake_minimum_required(VERSION 3.8)

# Project's name

project(mylib VERSION 1.0.1 DESCRIPTION "mylib description")

#project(check_library)
# Set the output folder where your program will be created
set(CMAKE_BINARY_DIR ${CMAKE_SOURCE_DIR}/../lib)
set(EXECUTABLE_OUTPUT_PATH ${CMAKE_BINARY_DIR})
set(LIBRARY_OUTPUT_PATH ${CMAKE_BINARY_DIR})

# The following folder will be included
#include_directories("${PROJECT_SOURCE_DIR}")

add_library(${PROJECT_NAME} SHARED
    md5/md5.cpp
    resolver/resolver.cpp
)

