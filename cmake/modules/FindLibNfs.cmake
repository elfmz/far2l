#  LIBNFS_FOUND     - System has libnf
#  LIBNFS_INCLUDE_DIR - The libnf include directories
#  LIBNFS_LIBRARIES    - The libraries needed to use libnf

find_path(LIBNFS_INCLUDE_DIR
    NAMES
      nfsc/libnfs.h
    PATHS
      /usr/include
      /usr/local/include
      /opt/local/include
      /sw/include
    PATH_SUFFIXES
      nfsc
  )

find_library(LIBNFS_LIBRARY
    NAMES
      nfs
      libnfs
    PATHS
      /usr/lib
      /usr/local/lib
      /opt/local/lib
      /sw/lib
  )

# handle the QUIETLY and REQUIRED arguments and set NF_FOUND to TRUE
# if all listed variables are TRUE
include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(LibNfs
	REQUIRED_VARS LIBNFS_LIBRARY LIBNFS_INCLUDE_DIR
)

set(LIBNFS_LIBRARIES ${LIBNFS_LIBRARY})
set(LIBNFS_INCLUDE_DIRS ${LIBNFS_INCLUDE_DIR})
mark_as_advanced(LIBNFS_INCLUDE_DIR LIBNFS_LIBRARY)
