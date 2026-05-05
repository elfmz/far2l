#ifndef MTPLOG_H
#define MTPLOG_H

#include <stdio.h>
#include <stdarg.h>
#include <wchar.h>

#ifdef __cplusplus
extern "C" {
#endif

// Set plugin directory from module path (call once from SetStartupInfoW).
void MTPLog_Init(const wchar_t *module_path);
// Returns plugin directory set by MTPLog_Init, or "" if not yet called.
const char *MTPLog_GetPluginDir();

#if defined(DEBUG) || defined(_DEBUG)
void DebugLog(const char *format, ...);
#endif

#ifdef __cplusplus
}
#endif

// Debug macros - completely absent from release builds; enable with -DDEBUG
#if defined(DEBUG) || defined(_DEBUG)
#define DBG(fmt, ...) DebugLog("[%s] " fmt, __FUNCTION__, ##__VA_ARGS__)
#else
#define DBG(fmt, ...) ((void)0)
#endif

#endif // MTPLOG_H
