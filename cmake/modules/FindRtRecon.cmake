# FindRtRecon.cmake - Find the RT Recon SDK
#
# This module finds the rt-recon-sdk include directory for
# InfluxDB-based auto-configuration for sstorm-rectest.
#
# Variables:
#   RT_RECON_FOUND        - True if rt-recon-sdk was found
#   RT_RECON_INCLUDE_DIRS - Include directories
#   RT_RECON_LIBRARIES    - Libraries to link against (empty, header-only)
#
# The header-only library influxdb.hpp is used directly;
# no library linking is required.

if(NOT RT_RECON_ROOT)
  if(DEFINED ENV{RT_RECON_DIR})
    set(RT_RECON_ROOT $ENV{RT_RECON_DIR})
  elseif(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/../rt-recon-sdk")
    set(RT_RECON_ROOT "${CMAKE_CURRENT_SOURCE_DIR}/../rt-recon-sdk")
  endif()
endif()

find_path(RT_RECON_INCLUDE_DIR
  NAMES rt-recon-sdk/autoconfig/influxdb.hpp
  PATHS
    ${RT_RECON_ROOT}/include
    /usr/local/include
    /usr/include
)

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(RtRecon
  REQUIRED_VARS RT_RECON_INCLUDE_DIR
  FAIL_MESSAGE "rt-recon-sdk not found. Set RT_RECON_DIR or install rt-recon-sdk."
)

# CMake 4.x sets RtRecon_FOUND (case-sensitive), older versions use RT_RECON_FOUND
if(RtRecon_FOUND)
  set(RT_RECON_FOUND TRUE)
  set(RT_RECON_INCLUDE_DIRS ${RT_RECON_INCLUDE_DIR})
  set(RT_RECON_LIBRARIES "")

  message(STATUS "RtRecon found: include=${RT_RECON_INCLUDE_DIRS} (header-only)")
endif()

mark_as_advanced(RT_RECON_INCLUDE_DIR RT_RECON_ROOT)
