# Specify the minimum version for CMake

cmake_minimum_required(VERSION 3.8)

# Project's name

project(check_test VERSION 1.0.1 DESCRIPTION "check_test description")

set(CMAKE_BINARY_DIR ${CMAKE_SOURCE_DIR}/bin)
set(EXECUTABLE_OUTPUT_PATH ${CMAKE_BINARY_DIR})
set(LIBRARY_OUTPUT_PATH ${CMAKE_BINARY_DIR})

# The following folder will be included
include_directories("${PROJECT_SOURCE_DIR}")
include_directories("../library/resolver")
include_directories("../library/md5")

LINK_DIRECTORIES(${CMAKE_BINARY_DIR}/../lib)
#TARGET_LINK_LIBRARIES(${PROJECT_NAME} mylib)

#
#file(GLOB SOURCES src/*.cpp)
# Include header files
#include_directories(include)

find_library(MY_LIB mylib PATHS ${CMAKE_BINARY_DIR}/../lib)

target_link_libraries(mylib "${MY_LIB}")


# Your-external "mylib", add GLOBAL if the imported library is located in directories above the current.
# add_library(mylib SHARED IMPORTED GLOBAL)
#set_property(TARGET mylib PROPERTY IMPORTED_LOCATION "../lib/mylib.so")

# You can define two import-locations: one for debug and one for release.
#set_target_properties( mylib PROPERTIES IMPORTED_LOCATION ${CMAKE_BINARY_DIR}/../lib/mylib.so )

add_executable(${PROJECT_NAME} ${PROJECT_SOURCE_DIR}/${PROJECT_NAME}.cpp)

