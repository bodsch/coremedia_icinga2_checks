
cmake_minimum_required (VERSION 3.6)

project (CoreMedia_Icinga2_checks)

if(NOT CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE Release)
endif()

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11 -fPIC -Wall")
set(CMAKE_CXX_FLAGS_RELEASE "-O3")

option(test "Build test check." OFF)

include_directories(lib/include)

file(GLOB corelibSrc lib/src/*.cpp)
add_library(corelib STATIC ${corelibSrc})

if (test)
  file(GLOB CHECK_TEST check_test/*.cpp)
  add_executable(check_test ${CHECK_TEST})
  target_link_libraries(check_test corelib)
endif()

file(GLOB CHECK_RUNLEVEL check_runlevel/*.cpp)
add_executable(check_runlevel ${CHECK_RUNLEVEL})
include_directories(lib/include 3rdParty/redox/include 3rdParty/json/include)
find_library(REDOX libredox.so PATHS 3rdParty/redox/build/)
find_library(HIREDIS libhiredis.so PATHS /usr/lib64)
find_library(EV libev.so PATHS /usr/lib64)
# find_library(JSONX



link_directories(${MyProj_BINARY_DIR}/lib)

#include_directories("${PROJECT_SOURCE_DIR}")
#include_directories("../lib/resolver")
#include_directories("../lib/md5")


target_link_libraries(check_runlevel corelib ${REDOX} ${HIREDIS} ${EV})
