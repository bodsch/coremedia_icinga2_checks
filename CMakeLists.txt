
cmake_minimum_required (VERSION 3.6)

project (CoreMedia_Icinga2_checks)

if(NOT CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE Release)
endif()

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11 -fPIC -Wall -Wextra")
set(CMAKE_CXX_FLAGS_RELEASE "-O2")
set(CMAKE_MODULE_PATH ${PROJECT_SOURCE_DIR}/cmake)
set(CMAKE_C_FLAGS_RELEASE "${CMAKE_C_FLAGS_RELEASE} -s")
set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} -s")

message(STATUS "Build type: ${CMAKE_BUILD_TYPE}")

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

find_library(LIB_REDOX libredox.so ${INCLUDE_PATH} REQUIRED)
find_library(LIB_EV libev.so ${INCLUDE_PATH} REQUIRED)

include_directories(${LIB_INCLUDES})


file(GLOB CHECK_RUNLEVEL check_runlevel/*.cpp)
add_executable(check_runlevel ${CHECK_RUNLEVEL})
target_link_libraries(check_runlevel corelib ${LIB_REDOX} ${LIB_EV})


file(GLOB CHECK_TOMCAT_MEMORY check_tomcat_memory/*.cpp)
add_executable(check_tomcat_memory ${CHECK_TOMCAT_MEMORY})
target_link_libraries(check_tomcat_memory corelib ${LIB_REDOX} ${LIB_EV})


file(GLOB CHECK_APP_CACHE check_app_cache/*.cpp)
add_executable(check_app_cache ${CHECK_APP_CACHE})
target_link_libraries(check_app_cache corelib ${LIB_REDOX} ${LIB_EV})


file(GLOB CHECK_CAP_CONNECTION check_capconnection/*.cpp)
add_executable(check_capconnection ${CHECK_CAP_CONNECTION})
target_link_libraries(check_capconnection corelib ${LIB_REDOX} ${LIB_EV})


file(GLOB CHECK_FEEDER_STATUS check_feeder_status/*.cpp)
add_executable(check_feeder_status ${CHECK_FEEDER_STATUS})
target_link_libraries(check_feeder_status corelib ${LIB_REDOX} ${LIB_EV})


file(GLOB CHECK_LICENSE check_license/*.cpp)
add_executable(check_license ${CHECK_LICENSE})
target_link_libraries(check_license corelib ${LIB_REDOX} ${LIB_EV})


file(GLOB CHECK_SEQUENCENUMBERS check_sequencenumbers/*.cpp)
add_executable(check_sequencenumbers ${CHECK_SEQUENCENUMBERS})
target_link_libraries(check_sequencenumbers corelib ${LIB_REDOX} ${LIB_EV})


file(GLOB CHECK_NE_LOAD check_nodeexporter_load/*.cpp)
add_executable(check_nodeexporter_load ${CHECK_NE_LOAD})
target_link_libraries(check_nodeexporter_load corelib ${LIB_REDOX} ${LIB_EV})


file(GLOB CHECK_NE_MEMORY check_nodeexporter_memory/*.cpp)
add_executable(check_nodeexporter_memory ${CHECK_NE_MEMORY})
target_link_libraries(check_nodeexporter_memory corelib ${LIB_REDOX} ${LIB_EV})


file(GLOB CHECK_NE_DISK check_nodeexporter_disk/*.cpp)
add_executable(check_nodeexporter_disk ${CHECK_NE_DISK})
target_link_libraries(check_nodeexporter_disk corelib ${LIB_REDOX} ${LIB_EV})


file(GLOB CHECK_PUBLISHER check_publisher/*.cpp)
add_executable(check_publisher ${CHECK_PUBLISHER})
target_link_libraries(check_publisher corelib ${LIB_REDOX} ${LIB_EV})






