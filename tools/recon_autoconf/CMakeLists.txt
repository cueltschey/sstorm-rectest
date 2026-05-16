cmake_minimum_required(VERSION 3.5)

list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake/modules")

find_package(RtRecon QUIET)

option(ENABLE_RECON_AUTOCONF "Enable auto-configuration from InfluxDB recon data" ${RT_RECON_FOUND})

if(ENABLE_RECON_AUTOCONF AND RT_RECON_FOUND AND TARGET rt_recon_sdk)
  add_executable(sstorm_autoconf
    sstorm_autoconf.cc
  )

  target_include_directories(sstorm_autoconf PRIVATE
    ${RT_RECON_INCLUDE_DIRS}
  )

  target_link_libraries(sstorm_autoconf PRIVATE
    rt_recon_sdk
  )

  target_compile_features(sstorm_autoconf PUBLIC cxx_std_14)

  install(TARGETS sstorm_autoconf DESTINATION bin)

  message(STATUS "Building sstorm_autoconf: auto-configuration from InfluxDB recon data")
else()
  message(STATUS "sstorm_autoconf disabled (RtRecon SDK library not available)")
endif()
