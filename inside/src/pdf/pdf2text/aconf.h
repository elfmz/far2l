/*
 * aconf.h
 *
 * This file is modified by cmake.
 *
 * Copyright 2002-2015 Glyph & Cog, LLC
 */

#ifndef ACONF_H
#define ACONF_H

#include <aconf2.h>

/*
 * Use A4 paper size instead of Letter for PostScript output.
 */
//#cmakedefine A4_PAPER

/*
 * Do not allow text selection.
 */
//#cmakedefine NO_TEXT_SELECT

/*
 * Include support for OPI comments.
 */
//#cmakedefine01 OPI_SUPPORT

/*
 * Enable multithreading support.
 */
//#cmakedefine01 MULTITHREADED

/*
 * Enable C++ exceptions.
 */
//#cmakedefine01 USE_EXCEPTIONS

/*
 * Use fixed point (instead of floating point) arithmetic.
 */
//#cmakedefine01 USE_FIXEDPOINT

/*
 * Enable support for CMYK output.
 */
//#cmakedefine01 SPLASH_CMYK

/*
 * Enable support for DeviceN output.
 */
//#cmakedefine01 SPLASH_DEVICEN

/*
 * Enable support for highlighted regions.
 */
//#cmakedefine HIGHLIGHTED_REGIONS

/*
 * Full path for the system-wide xpdfrc file.
 */

/*
 * Various include files and functions.
 */
//#cmakedefine01 HAVE_MKSTEMP
//#cmakedefine01 HAVE_MKSTEMPS
//#cmakedefine HAVE_POPEN
//#cmakedefine01 HAVE_STD_SORT
//#cmakedefine01 HAVE_FSEEKO
//#cmakedefine01 HAVE_FSEEK64
//#cmakedefine01 HAVE_FSEEKI64
#define _FILE_OFFSET_BITS 64
#define _LARGE_FILES 1
#define _LARGEFILE_SOURCE 1

/*
 * This is defined if using FreeType 2.
 */
//#cmakedefine01 HAVE_FREETYPE_H

/*
 * This is defined if using D-Type 4.
 */
//#cmakedefine01 HAVE_DTYPE4_H

/*
 * This is defined if using libpaper.
 */
//#cmakedefine01 HAVE_PAPER_H

/*
 * Defined if the Splash library is avaiable.
 */
//#cmakedefine01 HAVE_SPLASH

/*
 * Defined if using lcms2.
 */
//#cmakedefine01 HAVE_LCMS

/*
 * Defined for evaluation mode.
 */
//#cmakedefine01 EVAL_MODE

/*
 * Defined when building the closed source XpdfReader binary.
 */
//#cmakedefine01 BUILDING_XPDFREADER

#endif
