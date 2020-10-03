#====================================================
# IDE support
#====================================================

#====================================================
# Set output path for ide.
#====================================================
if (NOT COLORER_INTERNAL_BUILD)
  set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR}/bin)
  set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR}/bin)

  if ("${CMAKE_GENERATOR}" MATCHES "Visual Studio")
    set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR}/bin/vc)
    set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR}/bin/vc)
  elseif (DEFINED ENV{CLION_IDE} AND MSVC)
    set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR}/bin/clion_vc_${CMAKE_BUILD_TYPE})
    set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR}/bin/clion_vc_${CMAKE_BUILD_TYPE})
  elseif (DEFINED ENV{CLION_IDE})
    set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR}/bin/clion_gcc_${CMAKE_BUILD_TYPE})
    set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR}/bin/clion_gcc_${CMAKE_BUILD_TYPE})
  endif ()
endif ()
message("Output directory: ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}")

#====================================================
# Set configuration types
#====================================================
if (NOT MSVC_IDE)
  set(CMAKE_CONFIGURATION_TYPES "Debug;Release" CACHE STRING "" FORCE)
else ()
  #target_compile_options cannot set parameters for all configurations in MSVC
  set(CMAKE_CONFIGURATION_TYPES "${CMAKE_BUILD_TYPE}" CACHE STRING "" FORCE)
endif ()
message("Configurations for IDE: ${CMAKE_CONFIGURATION_TYPES}")