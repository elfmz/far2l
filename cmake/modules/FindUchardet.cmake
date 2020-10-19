# Copyright (C) 2007-2009 LuaDist.
# Created by Peter Kapec <kapecp@gmail.com>
# Redistribution and use of this file is allowed according to the terms of the MIT license.
# For details see the COPYRIGHT file distributed with LuaDist.
#	Note:
#		Searching headers and libraries is very simple and is NOT as powerful as scripts
#		distributed with CMake, because LuaDist defines directories to search for.
#		Everyone is encouraged to contact the author with improvements. Maybe this file
#		becomes part of CMake distribution sometimes.

# - Find uchardet
# Find the native uchardet headers and libraries.
#
# UCHARDET_INCLUDE_DIRS	- where to find UCHARDET.h, etc.
# UCHARDET_LIBRARIES	- List of libraries when using UCHARDET.
# UCHARDET_FOUND	- True if UCHARDET found.

# Look for the header file.
FIND_PATH(UCHARDET_INCLUDE_DIR NAMES uchardet.h PATH_SUFFIXES uchardet)

# Look for the library.
FIND_LIBRARY(UCHARDET_LIBRARY NAMES uchardet)

# Handle the QUIETLY and REQUIRED arguments and set UCHARDET_FOUND to TRUE if all listed variables are TRUE.
INCLUDE(FindPackageHandleStandardArgs)
FIND_PACKAGE_HANDLE_STANDARD_ARGS(UCHARDET DEFAULT_MSG UCHARDET_LIBRARY UCHARDET_INCLUDE_DIR)

# Copy the results to the output variables.
IF(UCHARDET_FOUND)
	SET(UCHARDET_LIBRARIES ${UCHARDET_LIBRARY})
	SET(UCHARDET_INCLUDE_DIRS ${UCHARDET_INCLUDE_DIR})
ELSE(UCHARDET_FOUND)
	SET(UCHARDET_LIBRARIES)
	SET(UCHARDET_INCLUDE_DIRS)
ENDIF(UCHARDET_FOUND)

MARK_AS_ADVANCED(UCHARDET_INCLUDE_DIRS UCHARDET_LIBRARIES)
