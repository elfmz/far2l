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

// Debug macros - completely absent from release builds; enable with -DDEBUG.
#if defined(DEBUG) || defined(_DEBUG)
#define DBG(fmt, ...) DebugLog("[%s] " fmt, __FUNCTION__, ##__VA_ARGS__)
#else
template <typename... Args>
constexpr void DebugLogDummy(const char *, Args &&...) noexcept {}
#define DBG(fmt, ...) (false ? (void)DebugLogDummy((fmt), ##__VA_ARGS__) : (void)0)
#endif

#endif // ADBLOG_H
