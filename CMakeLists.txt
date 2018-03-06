
cmake_minimum_required (VERSION 3.6)

project (CoreMedia_Icinga2_checks)

if(NOT CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE Release)
endif()

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11 -fPIC -Wall -Wextra")
set(CMAKE_CXX_FLAGS_RELEASE "-O2")

option(test "Build test check." OFF)

find_package(redox REQUIRED)
find_package(libev REQUIRED)

# ---------------------------------------------------------
# Library generation
# ---------------------------------------------------------
file(GLOB SRC_CORE_ALL lib/src/*.cpp)
add_library(corelib STATIC ${SRC_CORE_ALL})

# test check
if (test)
  file(GLOB CHECK_TEST check_test/*.cpp)
  include_directories(lib/include)
  add_executable(check_test ${CHECK_TEST})
  target_link_libraries(check_test corelib)
endif()


# set includes
set(LIB_INCLUDES
  lib/include
  /usr/local/include
  3rdParty/redox/include
  3rdParty/json/include
)

# set path to look for shared librarys
set(INCLUDE_PATH
  /usr/local/lib
  /usr/local/lib64
  /usr/lib
  /usr/lib64
)

find_library(LIB_REDOX libredox.so ${INCLUDE_PATH})
find_library(LIB_EV libev.so ${INCLUDE_PATH})

file(GLOB CHECK_RUNLEVEL check_runlevel/*.cpp)
file(GLOB CHECK_TOMCAT_MEMORY check_tomcat_memory/*.cpp)

add_executable(check_runlevel ${CHECK_RUNLEVEL})
add_executable(check_tomcat_memory ${CHECK_TOMCAT_MEMORY})

include_directories(${LIB_INCLUDES})

target_link_libraries(check_runlevel corelib ${LIB_REDOX} ${LIB_EV})
target_link_libraries(check_tomcat_memory corelib ${LIB_REDOX} ${LIB_EV})
