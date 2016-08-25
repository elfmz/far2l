/* uxconfig.h.  Generated from uxconfig.in by configure.  */
/* uxconfig.in.  Generated from configure.ac by autoheader.  */

/* Define if clock_gettime() is available */
#define HAVE_CLOCK_GETTIME /**/

/* Define to 1 if you have the declaration of `CLOCK_MONOTONIC', and to 0 if
   you don't. */
#define HAVE_DECL_CLOCK_MONOTONIC 1

/* Define to 1 if you have the `getaddrinfo' function. */
#define HAVE_GETADDRINFO 1

/* Define to 1 if you have the <gssapi/gssapi.h> header file. */
/* #undef HAVE_GSSAPI_GSSAPI_H */

/* Define to 1 if you have the <inttypes.h> header file. */
#define HAVE_INTTYPES_H 1

/* Define if libX11.a is available */
#define HAVE_LIBX11 /**/

/* Define to 1 if you have the <memory.h> header file. */
#define HAVE_MEMORY_H 1

/* Define to 1 if you have the `pango_font_family_is_monospace' function. */
#define HAVE_PANGO_FONT_FAMILY_IS_MONOSPACE 1

/* Define to 1 if you have the `pango_font_map_list_families' function. */
#define HAVE_PANGO_FONT_MAP_LIST_FAMILIES 1

/* Define to 1 if you have the `posix_openpt' function. */
#define HAVE_POSIX_OPENPT 1

/* Define to 1 if you have the `ptsname' function. */
#define HAVE_PTSNAME 1

/* Define to 1 if you have the `setresuid' function. */
#define HAVE_SETRESUID 1

/* Define if SO_PEERCRED works in the Linux fashion. */
#define HAVE_SO_PEERCRED 1

/* Define to 1 if you have the <stdint.h> header file. */
#define HAVE_STDINT_H 1

/* Define to 1 if you have the <stdlib.h> header file. */
#define HAVE_STDLIB_H 1

/* Define to 1 if you have the <strings.h> header file. */
#define HAVE_STRINGS_H 1

/* Define to 1 if you have the <string.h> header file. */
#define HAVE_STRING_H 1

/* Define to 1 if you have the `strsignal' function. */
#define HAVE_STRSIGNAL 1

/* Define to 1 if you have the <sys/select.h> header file. */
#define HAVE_SYS_SELECT_H 1

/* Define to 1 if you have the <sys/stat.h> header file. */
#define HAVE_SYS_STAT_H 1

/* Define to 1 if you have the <sys/types.h> header file. */
#define HAVE_SYS_TYPES_H 1

/* Define to 1 if you have the <unistd.h> header file. */
#define HAVE_UNISTD_H 1

/* Define to 1 if you have the `updwtmpx' function. */
#define HAVE_UPDWTMPX 1

/* Define to 1 if you have the <utmpx.h> header file. */
#define HAVE_UTMPX_H 1

/* Define if we could not find a gssapi library */
/* #undef NO_GSSAPI_LIB */

/* Define if we could not find libdl. */
/* #undef NO_LIBDL */

/* Name of package */
#define PACKAGE "putty"

/* Define to the address where bug reports for this package should be sent. */
#define PACKAGE_BUGREPORT ""

/* Define to the full name of this package. */
#define PACKAGE_NAME "putty"

/* Define to the full name and version of this package. */
#define PACKAGE_STRING "putty 0.67"

/* Define to the one symbol short name of this package. */
#define PACKAGE_TARNAME "putty"

/* Define to the home page for this package. */
#define PACKAGE_URL ""

/* Define to the version of this package. */
#define PACKAGE_VERSION "0.67"

/* Define to 1 if you have the ANSI C header files. */
#define STDC_HEADERS 1

/* Version number of package */
#define VERSION "0.67"

/* Define if building with GSSAPI support. */
#define WITH_GSSAPI 1


/* Convert autoconf definitions to ones that PuTTY wants. */

#ifndef HAVE_GETADDRINFO
# define NO_IPV6
#endif
#ifndef HAVE_SETRESUID
# define HAVE_NO_SETRESUID
#endif
#ifndef HAVE_STRSIGNAL
# define HAVE_NO_STRSIGNAL
#endif
#if !defined(HAVE_UTMPX_H) || !defined(HAVE_UPDWTMPX)
# define OMIT_UTMP
#endif
#ifndef HAVE_PTSNAME
# define BSD_PTYS
#endif
#ifndef HAVE_SYS_SELECT_H
# define HAVE_NO_SYS_SELECT_H
#endif
#ifndef HAVE_PANGO_FONT_FAMILY_IS_MONOSPACE
# define PANGO_PRE_1POINT4
#endif
#ifndef HAVE_PANGO_FONT_MAP_LIST_FAMILIES
# define PANGO_PRE_1POINT6
#endif
#if !defined(WITH_GSSAPI)
# define NO_GSSAPI
#endif
#if !defined(NO_GSSAPI) && defined(NO_LIBDL)
# if !defined(HAVE_GSSAPI_GSSAPI_H) || defined(NO_GSSAPI_LIB)
#  define NO_GSSAPI
# endif
#endif

