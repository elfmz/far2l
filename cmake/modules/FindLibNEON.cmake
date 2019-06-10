#  LIBNEON_FOUND     - System has libneon
#  LIBNEON_INCLUDE_DIR - The libneon include directories
#  LIBNEON_LIBRARIES    - The libraries needed to use libneon

find_path(LIBNEON_INCLUDE_DIR
    NAMES
      neon/ne_basic.h
      neon/ne_session.h
      neon/ne_props.h
    PATHS
      /usr/include
      /usr/local/include
      /opt/local/include
      /sw/include
    PATH_SUFFIXES
      neon
  )

find_library(LIBNEON_LIBRARY
    NAMES
      libneon
      neon
    PATHS
      /usr/lib
      /usr/local/lib
      /opt/local/lib
      /sw/lib
  )

# handle the QUIETLY and REQUIRED arguments and set LIBNEON_FOUND to TRUE
# if all listed variables are TRUE
include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(LibNEON
	REQUIRED_VARS LIBNEON_LIBRARY LIBNEON_INCLUDE_DIR
)

set(LIBNEON_LIBRARIES ${LIBNEON_LIBRARY})
set(LIBNEON_INCLUDE_DIRS ${LIBNEON_INCLUDE_DIR})

mark_as_advanced(LIBNEON_INCLUDE_DIR LIBNEON_LIBRARY)
