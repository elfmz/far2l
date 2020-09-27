#====================================================
# Enable policy
#====================================================
# enable CMAKE_MSVC_RUNTIME_LIBRARY
#cmake_policy(SET CMP0091 NEW)
# enable <LIB>_ROOT
#cmake_policy(SET CMP0074 NEW)

#====================================================
project(colorer CXX)

#====================================================
# Set default build to release
#====================================================
if (NOT CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE Release CACHE STRING "Build type, one of: Release, Debug" FORCE)
endif ()
message("Build type for Colorer: ${CMAKE_BUILD_TYPE}")

#====================================================
# global settings
#====================================================
#option(COLORER_INTERNAL_BUILD "This CMakeLists.txt called from other" OFF)
#option(COLORER_PVS_STUDIO_CHECK "Check project in PVS-Studio" OFF)
#option(COLORER_WIN64_BUILD "Build for x64, else x86" OFF)
#option(COLORER_TOOLS_BUILD "build colorer tools" OFF)

if (MSVC)
  # set global Visual C++ runtime
  if (CMAKE_BUILD_TYPE MATCHES Debug)
    set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreadedDebug")
  else ()
    set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded")
  endif ()
endif ()

#====================================================
# load modules
#====================================================
include(cmake/ide.cmake)
include(cmake/utils.cmake)

colorer_extract_version()
message(STATUS "Build Colorer: ${COLORER_VERSION}")

if (COLORER_PVS_STUDIO_CHECK)
  set(CMAKE_EXPORT_COMPILE_COMMANDS On)
  include(cmake/PVS-Studio)
endif ()

#====================================================
# xercesc
#====================================================
#if (WIN32)
#  set(XMLCH_TYPE "wchar_t")
#else ()
#  set(XMLCH_TYPE "uint16_t")
#endif ()
#set(xmlch-type ${XMLCH_TYPE} CACHE STRING "XMLCh type wchar_t" FORCE)
#set(BUILD_SHARED_LIBS OFF CACHE BOOL "Build shared libraries")
#set(network OFF CACHE BOOL "Network support" FORCE)
#add_subdirectory(./external/xercesc EXCLUDE_FROM_ALL)

#====================================================
# zlib
#====================================================
#set(ZLIB_EXAMPLES OFF CACHE BOOL "Build zlib example programs")
#set(ZLIB_MINIZIP ON CACHE BOOL "Build contrib/minizip alongside zlib")
#set(ZLIB_BUILD_SHARED_LIBS OFF CACHE BOOL "Build shared (dynamic) libraries")
#set(ZLIB_BUILD_SHARED_RUNTIME OFF CACHE BOOL "Build with shared linked runtine library (MSVC)")

#add_subdirectory(./external/zlib EXCLUDE_FROM_ALL)

#====================================================
# spdlog library
#====================================================
#add_subdirectory(./external/spdlog EXCLUDE_FROM_ALL)

#====================================================
# colorer library
#====================================================
add_subdirectory(./src)

#====================================================
# colorer tool
#====================================================
#if (COLORER_TOOLS_BUILD)
#  add_subdirectory(./tools/colorer)
#endif ()
add_definitions(-DSPDLOG_DISABLE_TID_CACHING)
