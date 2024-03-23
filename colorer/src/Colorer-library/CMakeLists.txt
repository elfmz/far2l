cmake_minimum_required(VERSION 3.15)

#====================================================
# Policy
#====================================================
# enable CMAKE_MSVC_RUNTIME_LIBRARY
cmake_policy(SET CMP0091 NEW)
# enable <LIB>_ROOT
cmake_policy(SET CMP0074 NEW)
cmake_policy(SET CMP0115 OLD)
#====================================================
# Extract version
#====================================================
include(cmake/colorer_extract_version.cmake)
colorer_extract_version()

#====================================================
# main project
#====================================================
project(colorer VERSION ${COLORER_VERSION} LANGUAGES CXX)
message(STATUS "Build Colorer library: ${COLORER_VERSION}")

#====================================================
# Set default build type
#====================================================
if (NOT CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE Release CACHE STRING "Build type, one of: Release, Debug" FORCE)
endif ()
message("Build type: ${CMAKE_BUILD_TYPE}")

#====================================================
# global library settings
#====================================================
# build options
option(COLORER_INTERNAL_BUILD "This CMakeLists.txt called from other" OFF)
option(COLORER_USE_DEPENDS_FROM_SYSTEM "Use dependencies installed in system" OFF)
option(COLORER_WIN64_BUILD "Build for x64, else x86" ON)
option(COLORER_TOOLS_BUILD "build colorer tools" ON)

# library features
option(COLORER_USE_JARINPUTSOURCE "Use jar inputsource for schemes" ON)
option(COLORER_USE_DUMMY_LOGGER "Use dummy logging" ON)

#====================================================
# global compilation settings
#====================================================
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
if(COLORER_USE_DEPENDS_FROM_SYSTEM)
  # this line need for find_package
  list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_LIST_DIR}/cmake")
endif()

include(cmake/ide.cmake)

#====================================================
# xercesc
#====================================================
if (COLORER_USE_DEPENDS_FROM_SYSTEM)
  find_package(XercesC REQUIRED)
else ()
  if (WIN32)
    set(XMLCH_TYPE "wchar_t")
  else ()
    set(XMLCH_TYPE "uint16_t")
  endif ()
  set(xmlch-type ${XMLCH_TYPE} CACHE STRING "XMLCh type wchar_t" FORCE)
  set(BUILD_SHARED_LIBS OFF CACHE BOOL "Build shared libraries")
  set(network OFF CACHE BOOL "Network support" FORCE)
  add_subdirectory(./external/xercesc EXCLUDE_FROM_ALL)
endif ()

#====================================================
# zlib
#====================================================
if (COLORER_USE_JARINPUTSOURCE)
  if (COLORER_USE_DEPENDS_FROM_SYSTEM)
    find_package(ZLIB REQUIRED)
    find_package(minizip REQUIRED)
  else ()
    set(ZLIB_EXAMPLES OFF CACHE BOOL "Build zlib example programs")
    set(ZLIB_MINIZIP ON CACHE BOOL "Build contrib/minizip alongside zlib")
    set(ZLIB_BUILD_SHARED_LIBS OFF CACHE BOOL "Build shared (dynamic) libraries")
    set(ZLIB_BUILD_SHARED_RUNTIME OFF CACHE BOOL "Build with shared linked runtine library (MSVC)")

    add_subdirectory(./external/zlib EXCLUDE_FROM_ALL)
  endif ()
endif ()


#====================================================
# spdlog library
#====================================================
if (NOT COLORER_USE_DUMMY_LOGGER)
  if (COLORER_USE_DEPENDS_FROM_SYSTEM)
    find_package(spdlog REQUIRED)
    find_package(fmt REQUIRED)
  else()
    add_subdirectory(./external/spdlog EXCLUDE_FROM_ALL)
  endif ()
endif ()
#====================================================
# colorer library
#====================================================
add_subdirectory(./src)

#====================================================
# colorer tool
#====================================================
if (COLORER_TOOLS_BUILD)
  add_subdirectory(./tools/colorer)
endif ()
