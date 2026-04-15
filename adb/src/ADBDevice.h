#pragma once

// Standard library includes
#include <string>
#include <vector>
#include <memory>
#include <time.h>
#include <functional>

// System includes
#include <sys/stat.h>

// FAR Manager includes
#include "farplug-wide.h"

// Forward declarations
class ADBShell;
struct PluginPanelItem;

// ADB Device implementation
class ADBDevice {
private:
    std::string _device_serial;
    std::string _current_path;
    std::unique_ptr<ADBShell> _adb_shell;
    bool _connected;

    void EnsureConnection();
    std::string ExtractPathFromPwd(const std::string &pwd_output);

    // Parse ls -la date/time format
    time_t ParseLsDateTime(const std::string &date, const std::string &time_str);

    // Helper to build args with device serial prefix
    std::vector<std::string> BuildArgs(const std::vector<std::string>& args) const;

    // Helper to create progress callback for parsing percentage
    std::function<void(const std::string&)> CreateProgressCallback(
        std::function<void(int)>& on_progress, int& last_percent, std::string& pending) const;

    // Helper to check if result indicates success
    bool IsSuccessResult(const std::string& result, bool is_push = false) const;

    // Unified transfer helper (DRY)
    int TransferItem(const std::string& src, const std::string& dst, bool is_push, bool recursive,
                    const std::function<void(int)>& on_progress = {},
                    const std::function<bool()>& abort_check = {});

public:
    // Public methods for command execution
    std::string RunAdbCommand(const std::string &command);
    std::string RunAdbCommand(const std::vector<std::string> &args);
    std::string RunAdbCommand(const std::vector<std::string> &args, const std::function<void(const std::string&)> &on_chunk);
    std::string RunAdbCommandWithProgress(const std::vector<std::string> &args, const std::function<void(const std::string&)> &on_chunk, const std::function<bool()> &abort_check = {});
    std::string RunShellCommand(const std::string &command);
    // Run shell command and stream output via callback (bypasses persistent session echo issues)
    void RunShellCommandStreaming(const std::string &command, const std::function<void(const std::string&)> &on_line);
    std::string GetCurrentWorkingDirectory();
    ADBDevice(const std::string &device_serial);
    virtual ~ADBDevice();

    // File operations
    std::string DirectoryEnum(const std::string &path, std::vector<PluginPanelItem> &files);
    bool SetDirectory(const std::string &path);

    // File transfer operations
    int PullFile(const std::string &devicePath, const std::string &localPath);
    int PullFile(const std::string &devicePath, const std::string &localPath, const std::function<void(int)> &on_progress, const std::function<bool()> &abort_check = {});
    int PushFile(const std::string &localPath, const std::string &devicePath);
    int PushFile(const std::string &localPath, const std::string &devicePath, const std::function<void(int)> &on_progress, const std::function<bool()> &abort_check = {});
    int PullDirectory(const std::string &devicePath, const std::string &localPath);
    int PullDirectory(const std::string &devicePath, const std::string &localPath, const std::function<void(int)> &on_progress, const std::function<bool()> &abort_check = {});
    int PushDirectory(const std::string &localPath, const std::string &devicePath);
    int PushDirectory(const std::string &localPath, const std::string &devicePath, const std::function<void(int)> &on_progress, const std::function<bool()> &abort_check = {});

    // File deletion operations
    int DeleteFile(const std::string &devicePath);
    int DeleteDirectory(const std::string &devicePath);

    // Directory creation operations
    int CreateDirectory(const std::string &devicePath);
    int CopyRemote(const std::string &srcDevicePath, const std::string &dstDeviceDir);
    int MoveRemote(const std::string &srcDevicePath, const std::string &dstDeviceDir);

    // File existence check
    bool FileExists(const std::string &devicePath);

    // Directory info (file count, total size in bytes)
    struct DirectoryInfo {
        uint64_t file_count;
        uint64_t total_size;
    };
    DirectoryInfo GetDirectoryInfo(const std::string &devicePath);

    // Connection management
    bool Connect();
    void Disconnect();
    bool IsConnected() const { return _connected; }
    std::string GetDeviceSerial() const { return _device_serial; }
    std::string GetCurrentPath() const { return _current_path; }
    void SyncPath();  // Query shell's actual cwd and update internal path

    // Utility functions
    std::string WStringToString(const std::wstring &wstr);

    // Error mapping
    static int Str2Errno(const std::string &adbError);

    // Static helper for PluginPanelItem memory management
    static wchar_t* AllocateItemString(const std::string& s);
};

// Path utility functions (shared across ADB classes)
namespace ADBUtils {
    // Join path components, ensuring proper separator
    std::string JoinPath(const std::string& base, const std::string& component);

    // Trims trailing newlines/carriage returns
    void TrimTrailingNewlines(std::string& s);

    // Shell quoting for paths with spaces
    std::string ShellQuote(const std::string& s);

    // Check if connection is available, returns EIO if not
    int CheckConnection(bool connected);
}

// Helper class for progress parsing (reduces code duplication)
class ProgressParser {
public:
    ProgressParser(std::function<void(int)> on_progress, bool debug_log = false);

    // Process a chunk of output, calling on_progress for each percentage found
    void operator()(const std::string &chunk);

    // Drain any remaining pending data (call after command completes)
    void drain();

    // Report completion (100%)
    void complete();

    // Report initial state (0%)
    void start();

private:
    std::function<void(int)> _on_progress;
    bool _debug_log;
    int _last_percent;
    std::string _pending;

    static int ExtractPercent(const std::string &s);
};
