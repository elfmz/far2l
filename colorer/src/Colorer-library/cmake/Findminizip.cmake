# - Find minizip
# Find the native MINIZIP includes and library
#
#  MINIZIP_INCLUDE_DIRS - where to find minizip.h, etc.
#  MINIZIP_LIBRARIES   - List of libraries when using minizip.
#  MINIZIP_FOUND       - True if minizip found.
#
#  minizip::minizip - library, if found


find_path(minizip_INCLUDE_DIR NAMES "minizip/zip.h")
mark_as_advanced(minizip_INCLUDE_DIR)

find_library(minizip_LIBRARY NAMES minizip)
mark_as_advanced(minizip_LIBRARY)

include(FindPackageHandleStandardArgs)
  find_package_handle_standard_args(minizip
    FOUND_VAR minizip_FOUND
    REQUIRED_VARS minizip_LIBRARY minizip_INCLUDE_DIR
    FAIL_MESSAGE "Failed to find minizip")

if(minizip_FOUND)
  set(minizip_LIBRARIES ${minizip_LIBRARY})
  set(minizip_INCLUDE_DIRS ${minizip_INCLUDE_DIR})

  # For header-only libraries
  if(NOT TARGET minizip::minizip)
    add_library(minizip::minizip UNKNOWN IMPORTED)
    if(minizip_INCLUDE_DIRS)
      set_target_properties(minizip::minizip PROPERTIES
          INTERFACE_INCLUDE_DIRECTORIES "${minizip_INCLUDE_DIRS}")
    endif()
    if(EXISTS "${minizip_LIBRARY}")
      set_target_properties(minizip::minizip PROPERTIES
          IMPORTED_LINK_INTERFACE_LANGUAGES "C"
          IMPORTED_LOCATION "${minizip_LIBRARY}")
    endif()
  endif()
endif()


