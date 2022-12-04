#pragma once
#include <sys/stat.h>
#include <sys/time.h>
#include <fcntl.h>

#undef utimens
#define utimens(PATH, TIMES) utimensat(AT_FDCWD, PATH, TIMES, 0)

#ifdef __APPLE__
# include "Availability.h"
# if !defined(MAC_OS_X_VERSION_10_13) || MAC_OS_X_VERSION_MAX_ALLOWED < MAC_OS_X_VERSION_10_13
static inline int utimens_compat(const char *path, const struct timespec times[2])
{
	struct timeval tvs[2] = {};
	tvs[0].tv_sec = times[0].tv_sec;
	tvs[0].tv_usec = suseconds_t(times[0].tv_nsec / 1000);
	tvs[1].tv_sec = times[1].tv_sec;
	tvs[1].tv_usec = suseconds_t(times[1].tv_nsec / 1000);
	return utimes(path, tvs);
}
#  undef utimens
#  define utimens utimens_compat

static inline int futimens_compat(int fd, const struct timespec times[2])
{
	struct timeval tvs[2] = {};
	tvs[0].tv_sec = times[0].tv_sec;
	tvs[0].tv_usec = suseconds_t(times[0].tv_nsec / 1000);
	tvs[1].tv_sec = times[1].tv_sec;
	tvs[1].tv_usec = suseconds_t(times[1].tv_nsec / 1000);
	return futimes(fd, tvs);
}

#  undef futimens
#  define futimens futimens_compat
# endif
#endif


