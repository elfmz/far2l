#ifndef ADBLOG_H
#define ADBLOG_H

#include <stdio.h>
#include <stdarg.h>

#ifdef __cplusplus
extern "C" {
#endif

#ifndef NDEBUG
void DebugLog(const char *format, ...);
#endif

#ifdef __cplusplus
}
#endif

// Debug macros - only active in debug builds (NDEBUG not defined)
#ifndef NDEBUG
#define DBG(fmt, ...) DebugLog("[%s] " fmt, __FUNCTION__, ##__VA_ARGS__)
#else
#define DBG(fmt, ...) ((void)0)
#endif

#endif // ADBLOG_H
