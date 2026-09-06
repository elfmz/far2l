/* config.h — hand-written for CMake build (replaces autotools-generated) */
#ifndef _LIBMTP_CONFIG_H
#define _LIBMTP_CONFIG_H 1

#define HAVE_ARPA_INET_H 1
#define HAVE_BASENAME 1
#define HAVE_CTYPE_H 1
#define HAVE_DLFCN_H 1
#define HAVE_ERRNO_H 1
#define HAVE_FCNTL_H 1
#define HAVE_GETOPT_H 1
#define HAVE_ICONV 1
#define HAVE_ICONV_H 1
#define HAVE_INTTYPES_H 1
#define HAVE_LIBGEN_H 1
#define HAVE_LIBUSB1 /**/
#define HAVE_LIMITS_H 1
#define HAVE_LOCALE_H 1
#define HAVE_MEMSET 1
#define HAVE_MKSTEMP 1
#define HAVE_SELECT 1
#define HAVE_STDINT_H 1
#define HAVE_STDIO_H 1
#define HAVE_STDLIB_H 1
#define HAVE_STRDUP 1
#define HAVE_STRERROR 1
#define HAVE_STRINGS_H 1
#define HAVE_STRING_H 1
#define HAVE_STRNDUP 1
#define HAVE_STRRCHR 1
#define HAVE_STRTOUL 1
#define HAVE_STRUCT_STAT_ST_BLKSIZE 1
#define HAVE_ST_BLKSIZE 1
#define HAVE_SYS_STAT_H 1
#define HAVE_SYS_TIME_H 1
#define HAVE_SYS_TYPES_H 1
#define HAVE_UNISTD_H 1
#define HAVE_USLEEP 1
#define ICONV_CONST
#define LSTAT_FOLLOWS_SLASHED_SYMLINK 1
#define LT_OBJDIR ".libs/"
#define PACKAGE "libmtp"
#define PACKAGE_BUGREPORT "libmtp-discuss@lists.sourceforge.net"
#define PACKAGE_NAME "libmtp"
#define PACKAGE_STRING "libmtp 1.1.23"
#define PACKAGE_TARNAME "libmtp"
#define PACKAGE_URL ""
#define PACKAGE_VERSION "1.1.23"
#define RETSIGTYPE void
#define STDC_HEADERS 1
#define TIME_WITH_SYS_TIME 1
#define UDEV_DIR "/usr/lib/udev/"
/* #undef USE_MTPZ */
#define VERSION "1.1.23"

/* langinfo.h and sys/uio.h are POSIX — present on macOS, glibc, musl.
 * HAVE_LANGINFO_H gates iconv-based UCS2↔UTF-8 in ptp-pack.c; without it
 * non-ASCII filenames silently degrade to '?'. */
#define HAVE_LANGINFO_H 1
#define HAVE_SYS_UIO_H 1
/* byteswap.h is glibc-only — macOS / musl don't have it. */
#if defined(__linux__) && defined(__GLIBC__)
#  define HAVE_BYTESWAP_H 1
#endif

#if defined AC_APPLE_UNIVERSAL_BUILD
#  if defined __BIG_ENDIAN__
#    define WORDS_BIGENDIAN 1
#  endif
#else
#  ifndef WORDS_BIGENDIAN
/* #  undef WORDS_BIGENDIAN */
#  endif
#endif

#endif /* _LIBMTP_CONFIG_H */
