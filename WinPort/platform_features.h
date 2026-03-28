
// platform_features.h
#pragma once

#if !defined(__HAIKU__) && (defined(__linux__) || defined(__FreeBSD__) || \
    defined(__OpenBSD__) || defined(__NetBSD__) || defined(__DragonFly__) || \
    defined(__APPLE__) || defined(__sun) || defined(_AIX))
#define USE_FSTATAT	1
#else
#define USE_FSTATAT 0
#endif

#if defined(__linux__)
#define USE_STATX 1
#else
#define USE_STATX 0
#endif

#if defined(__HAIKU__)
#define USE_HAIKU_NATIVE_API 1
#else
#define USE_HAIKU_NATIVE_API 0
#endif

#if defined(__FreeBSD__) || defined(__OpenBSD__) || defined(__NetBSD__) || \
	defined(__DragonFly__) || defined(__APPLE__)
#define HAVE_STAT_BIRTHTIME 1
#else
#define HAVE_STAT_BIRTHTIME 0
#endif
