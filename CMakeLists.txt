
cmake_minimum_required (VERSION 3.6)

project (CoreMedia_Icinga2_checks)

if(NOT CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE Release)
endif()

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11 -fPIC -Wall -Wextra")
set(CMAKE_CXX_FLAGS_RELEASE "-O2")

option(test "Build test check." OFF)

# link_directories(${MyProj_BINARY_DIR}/lib)

file(GLOB corelibSrc lib/src/*.cpp)
add_library(corelib STATIC ${corelibSrc})

if (test)
  file(GLOB CHECK_TEST check_test/*.cpp)
  include_directories(lib/include)
  add_executable(check_test ${CHECK_TEST})
  target_link_libraries(check_test corelib)
endif()

set(LIB_INCLUDES
  lib/include
  3rdParty/redox/include
  3rdParty/json/include
)

file(GLOB CHECK_RUNLEVEL check_runlevel/*.cpp)
file(GLOB CHECK_TOMCAT_MEMORY check_tomcat_memory/*.cpp)

add_executable(check_runlevel ${CHECK_RUNLEVEL})
add_executable(check_tomcat_memory ${CHECK_TOMCAT_MEMORY})

include_directories(${LIB_INCLUDES})
find_library(REDOX libredox.so PATHS 3rdParty/redox/build/)


target_link_libraries(check_runlevel corelib ${REDOX} ${HIREDIS} ${EV})
target_link_libraries(check_tomcat_memory corelib ${REDOX} ${HIREDIS} ${EV})
