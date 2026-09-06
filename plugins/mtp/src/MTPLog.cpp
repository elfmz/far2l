#include "MTPLog.h"
#include <time.h>
#include <stdlib.h>
#include <unistd.h>
#include <mutex>
#include <string>
#include <utils.h>
#include <WideMB.h>

// Plugin directory, set once by MTPLog_Init from SetStartupInfoW.
static std::string g_plugin_dir;

void MTPLog_Init(const wchar_t *module_path) {
    if (!module_path || !*module_path) return;
    g_plugin_dir = ExtractFilePath<char>(StrWide2MB(module_path));
}

const char *MTPLog_GetPluginDir() {
    return g_plugin_dir.c_str();
}

#if defined(DEBUG) || defined(_DEBUG)

static const char *DebugLogPath() {
    static std::string path = []() {
        if (!g_plugin_dir.empty())
            return g_plugin_dir + "/mtp.log";
        // Fallback before MTPLog_Init runs.
        const char *base = getenv("TMPDIR");
        if (!base || !*base) base = "/tmp";
        std::string p = base;
        if (!p.empty() && p.back() == '/') p.pop_back();
        return p + "/mtp.log";
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
