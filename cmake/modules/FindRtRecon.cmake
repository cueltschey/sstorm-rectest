# FindRtRecon.cmake - Find the RT Recon SDK
#
# This module finds the rt-recon-sdk library, which provides
# InfluxDB-based auto-configuration for sstorm-rectest.
#
# Variables:
#   RT_RECON_FOUND        - True if rt-recon-sdk was found
#   RT_RECON_INCLUDE_DIRS - Include directories
#   RT_RECON_LIBRARIES    - Libraries to link against
#
# It also creates an imported target rt_recon_sdk.

# Search priority:
# 1. RT_RECON_DIR environment variable
# 2. Adjacent rt-recon-sdk directory (sibling to sstorm-rectest)
# 3. Standard system paths

if(NOT RT_RECON_ROOT)
  if(DEFINED ENV{RT_RECON_DIR})
    set(RT_RECON_ROOT $ENV{RT_RECON_DIR})
  elseif(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/../rt-recon-sdk")
    set(RT_RECON_ROOT "${CMAKE_CURRENT_SOURCE_DIR}/../rt-recon-sdk")
  endif()
endif()

find_path(RT_RECON_INCLUDE_DIR
  NAMES rt_recon/recon_data.h
  PATHS
    ${RT_RECON_ROOT}/include
    /usr/local/include
    /usr/include
)

find_library(RT_RECON_LIBRARY
  NAMES rt_recon_sdk
  PATHS
    ${RT_RECON_ROOT}/build
    ${RT_RECON_ROOT}/build/lib
    /usr/local/lib
    /usr/lib
)

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(RtRecon
  REQUIRED_VARS RT_RECON_LIBRARY RT_RECON_INCLUDE_DIR
  FAIL_MESSAGE "rt-recon-sdk not found. Set RT_RECON_DIR or install rt-recon-sdk."
)

if(RT_RECON_FOUND)
  set(RT_RECON_INCLUDE_DIRS ${RT_RECON_INCLUDE_DIR})
  set(RT_RECON_LIBRARIES ${RT_RECON_LIBRARY})

  if(NOT TARGET rt_recon_sdk)
    add_library(rt_recon_sdk UNKNOWN IMPORTED)
    set_target_properties(rt_recon_sdk PROPERTIES
      IMPORTED_LOCATION "${RT_RECON_LIBRARY}"
      INTERFACE_INCLUDE_DIRECTORIES "${RT_RECON_INCLUDE_DIRS}"
    )
  endif()

  message(STATUS "RtRecon found: include=${RT_RECON_INCLUDE_DIRS}, libs=${RT_RECON_LIBRARIES}")
endif()

mark_as_advanced(RT_RECON_INCLUDE_DIR RT_RECON_LIBRARY RT_RECON_ROOT)
