/* config.h for libusb on desktop Linux (glibc, no libudev — netlink hotplug).
 *
 * Hand-curated to match what `./configure --disable-udev` produces on a recent
 * glibc-based distro (Debian 12 / Ubuntu 22.04+ baseline). The full set of
 * HAVE_* macros relevant to our included sources is enumerated in VENDORING.md.
 *
 * Notably absent: HAVE_LIBUDEV (we use linux_netlink.c instead),
 * USE_SYSTEM_LOGGING_FACILITY (no syslog redirection on desktop).
 */

/* Define to the attribute for default visibility. */
#define DEFAULT_VISIBILITY __attribute__ ((visibility ("default")))

/* Define to 1 to enable message logging. */
#define ENABLE_LOGGING 1

/* Define to 1 if you have the <asm/types.h> header file. */
#define HAVE_ASM_TYPES_H 1

/* Define to 1 if you have the `clock_gettime' function. */
#define HAVE_CLOCK_GETTIME 1

/* Define to 1 if you have the <sys/eventfd.h> header / `eventfd' syscall. */
#define HAVE_EVENTFD 1

/* Define to 1 if the system has the type `nfds_t'. */
#define HAVE_NFDS_T 1

/* Define to 1 if you have the `pipe2' function. */
#define HAVE_PIPE2 1

/* Define to 1 if you have the `pthread_condattr_setclock' function. */
#define HAVE_PTHREAD_CONDATTR_SETCLOCK 1

/* Define to 1 if you have the `pthread_setname_np' function. */
#define HAVE_PTHREAD_SETNAME_NP 1

/* Define to 1 if you have the <sys/time.h> header file. */
#define HAVE_SYS_TIME_H 1

/* Define to 1 if you have the `timerfd_create' function (Linux >= 2.6.25). */
#define HAVE_TIMERFD 1

/* Define to 1 if compiling for a POSIX platform. */
#define PLATFORM_POSIX 1

/* Define to the attribute for enabling parameter checks on printf-like
   functions. */
#define PRINTF_FORMAT(a, b) __attribute__ ((__format__ (__printf__, a, b)))

/* Enable GNU extensions. */
#define _GNU_SOURCE 1
