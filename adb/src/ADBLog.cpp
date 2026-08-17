#include "ADBLog.h"
#include <time.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <dlfcn.h>
#include <mutex>
#include <string>

#if defined(DEBUG) || defined(_DEBUG)

// Log next to the plugin's own .so/.dylib; falls back to $TMPDIR if dladdr fails.
static const char *DebugLogPath() {
    static std::string path = []() {
        Dl_info info;
        if (dladdr((const void *)&DebugLogPath, &info) && info.dli_fname) {
            std::string p = info.dli_fname;
            auto slash = p.find_last_of('/');
            if (slash != std::string::npos) {
                return p.substr(0, slash) + "/adb_plugin.log";
            }
        }
        const char *base = getenv("TMPDIR");
        if (!base || !*base) base = "/tmp";
        std::string p = base;
        if (!p.empty() && p.back() == '/') p.pop_back();
        p += "/adb_plugin.log";
        return p;
    }();
    return path.c_str();
}

void DebugLog(const char *format, ...)
{
    static std::mutex mtx;
    static FILE *logFile = nullptr;
    static bool disabled = false;

    std::lock_guard<std::mutex> lock(mtx);

    if (disabled) return;

    if (!logFile) {
        logFile = fopen(DebugLogPath(), "a");
        if (!logFile) {
            // Couldn't open once — give up for this process to avoid spamming fopen errors.
            disabled = true;
            return;
        }
    }

    time_t now = time(nullptr);
    struct tm tm_buf;
    // localtime_r: reentrant variant; static-buffer localtime() would race between threads.
    struct tm *tm_info = localtime_r(&now, &tm_buf);
    char timestamp[64];
    if (tm_info) {
        strftime(timestamp, sizeof(timestamp), "%H:%M:%S", tm_info);
    } else {
        strncpy(timestamp, "??:??:??", sizeof(timestamp));
    }

    if (fprintf(logFile, "[%s] ", timestamp) < 0) {
        fclose(logFile);
        logFile = nullptr;
        disabled = true;
        return;
    }

    va_list args;
    va_start(args, format);
    int n = vfprintf(logFile, format, args);
    va_end(args);
    if (n < 0) {
        fclose(logFile);
        logFile = nullptr;
        disabled = true;
        return;
    }
    fflush(logFile);
}

#endif // defined(DEBUG) || defined(_DEBUG)
