find_path(fmt_INCLUDE_DIR NAMES fmt/format.h)

if(fmt_INCLUDE_DIR)
  set(_fmt_version_file "${fmt_INCLUDE_DIR}/fmt/core.h")
  if(NOT EXISTS "${_fmt_version_file}")
    set(_fmt_version_file "${fmt_INCLUDE_DIR}/fmt/format.h")
  endif()
  if(EXISTS "${_fmt_version_file}")
    # parse "#define FMT_VERSION 40100" to 4.1.0
    file(STRINGS "${_fmt_version_file}" fmt_VERSION_LINE
      REGEX "^#define[ \t]+FMT_VERSION[ \t]+[0-9]+$")
    string(REGEX REPLACE "^#define[ \t]+FMT_VERSION[ \t]+([0-9]+)$"
      "\\1" fmt_VERSION "${fmt_VERSION_LINE}")
    foreach(ver "fmt_VERSION_PATCH" "fmt_VERSION_MINOR" "fmt_VERSION_MAJOR")
      math(EXPR ${ver} "${fmt_VERSION} % 100")
      math(EXPR fmt_VERSION "(${fmt_VERSION} - ${${ver}}) / 100")
    endforeach()
    set(fmt_VERSION
      "${fmt_VERSION_MAJOR}.${fmt_VERSION_MINOR}.${fmt_VERSION_PATCH}")
  endif()
endif()

find_library(fmt_LIBRARY NAMES fmt)

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(fmt
  REQUIRED_VARS fmt_INCLUDE_DIR fmt_LIBRARY
  VERSION_VAR fmt_VERSION)
mark_as_advanced(
  fmt_INCLUDE_DIR
  fmt_LIBRARY
  fmt_VERSION_MAJOR
  fmt_VERSION_MINOR
  fmt_VERSION_PATCH
  fmt_VERSION_STRING)

if(fmt_FOUND AND NOT (TARGET fmt::fmt))
  add_library(fmt::fmt UNKNOWN IMPORTED)
  set_target_properties(fmt::fmt PROPERTIES
    INTERFACE_INCLUDE_DIRECTORIES "${fmt_INCLUDE_DIR}"
    IMPORTED_LINK_INTERFACE_LANGUAGES "CXX"
    IMPORTED_LOCATION "${fmt_LIBRARY}")
endif()
