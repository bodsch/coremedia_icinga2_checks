
cmake_minimum_required (VERSION 3.6)

project (CoreMedia_Icinga2_checks)

if(NOT CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE Release)
endif()

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11 -fPIC -Wall -Wextra")
set(CMAKE_CXX_FLAGS_RELEASE "-O2")
set(CMAKE_MODULE_PATH ${PROJECT_SOURCE_DIR}/cmake)

option(test "Build test check." OFF)

# find_package(libev REQUIRED)
# find_package(redox REQUIRED)


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
  3rdParty/redox/build
)

find_library(LIB_REDOX libredox.so ${INCLUDE_PATH})
find_library(LIB_EV libev.so ${INCLUDE_PATH})

file(GLOB CHECK_RUNLEVEL check_runlevel/*.cpp)
file(GLOB CHECK_TOMCAT_MEMORY check_tomcat_memory/*.cpp)
file(GLOB CHECK_APP_CACHE check_app_cache/*.cpp)
file(GLOB CHECK_CAP_CONNECTION check_capconnection/*.cpp)

add_executable(check_runlevel ${CHECK_RUNLEVEL})
add_executable(check_tomcat_memory ${CHECK_TOMCAT_MEMORY})
add_executable(check_app_cache ${CHECK_APP_CACHE})
add_executable(check_capconnection ${CHECK_CAP_CONNECTION})

include_directories(${LIB_INCLUDES})

target_link_libraries(check_runlevel corelib ${LIB_REDOX} ${LIB_EV})
target_link_libraries(check_tomcat_memory corelib ${LIB_REDOX} ${LIB_EV})
target_link_libraries(check_app_cache corelib ${LIB_REDOX} ${LIB_EV})
target_link_libraries(check_capconnection corelib ${LIB_REDOX} ${LIB_EV})
