#ifndef ADBLOG_H
#define ADBLOG_H

#include <stdio.h>
#include <stdarg.h>

#ifdef __cplusplus
extern "C" {
#endif

#if defined(DEBUG) || defined(_DEBUG)
void DebugLog(const char *format, ...);
#endif

#ifdef __cplusplus
}
#endif

// In release DBG(...) discards its arguments at the preprocessor level, so
// DBG-only variables can be guarded with `#if defined(DEBUG) || defined(_DEBUG)`
// at the call site without wrapping the DBG call itself.
#if defined(DEBUG) || defined(_DEBUG)
#define DBG(fmt, ...) DebugLog("[%s] " fmt, __FUNCTION__, ##__VA_ARGS__)
#else
#define DBG(fmt, ...) ((void)0)
#endif

#endif // ADBLOG_H
