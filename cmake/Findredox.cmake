# Try to find redox
# Once done, this will define
#
# REDOX_FOUND        - system has hiredis
# REDOX_INCLUDE_DIRS - hiredis include directories
# REDOX_LIBRARIES    - libraries need to use hiredis

if(REDOX_INCLUDE_DIRS AND REDOX_LIBRARIES)
  set(REDOX_FIND_QUIETLY TRUE)
else()
  find_path(
    REDOX_INCLUDE_DIR
    NAMES redox/redox.hpp
    HINTS ${REDOX_ROOT_DIR}
    PATH_SUFFIXES include)

  find_library(
    REDOX_LIBRARY
    NAMES redox
    HINTS ${REDOX_ROOT_DIR}
    PATH_SUFFIXES ${CMAKE_INSTALL_LIBDIR})

  set(REDOX_INCLUDE_DIRS ${REDOX_INCLUDE_DIR})
  set(REDOX_LIBRARIES ${REDOX_LIBRARY})

  include(FindPackageHandleStandardArgs)
  find_package_handle_standard_args(
    redox DEFAULT_MSG REDOX_LIBRARY REDOX_INCLUDE_DIR)

  mark_as_advanced(REDOX_LIBRARY REDOX_INCLUDE_DIR)
endif()

