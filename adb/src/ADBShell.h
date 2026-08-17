#pragma once

// Standard library includes
#include <string>
#include <memory>
#include <cstdint>
#include <atomic>
#include <mutex>
#include <vector>
#include <functional>


class ADBShell {
public:
    ADBShell(const std::string& device_serial = "");
    ~ADBShell();
    
    // Disable copy constructor and assignment
    ADBShell(const ADBShell&) = delete;
    ADBShell& operator=(const ADBShell&) = delete;
    
    // Start the ADB shell process
    bool start();
    // Execute a command and return the output
    std::string shellCommand(const std::string& command);
    // Execute a device-specific ADB command with -s <device_serial> - instance version
    std::string adbCommand(const std::string& command) const;
    // Execute a global ADB command (e.g., "adb devices -l") - static version
    static std::string adbExec(const std::string& command);
    static std::string adbExec(const std::vector<std::string>& args);
    // idle_timeout_ms: kill the child if it emits nothing for that long. Only for commands that
    // are guaranteed to keep talking (transfers pass -p); leave 0 for silent `adb shell` work.
    static std::string adbExecWithProgress(const std::vector<std::string>& args, const std::function<void(const std::string&)>& on_chunk, const std::function<bool()>& abort_check = {}, int idle_timeout_ms = 0);
    // How long a transfer may stay completely silent before it is treated as dead. This is an
    // *idle* bound, not a total one - a large copy is legitimately slow, but `-p` prints progress
    // continuously (measured worst inter-emit gap 0.6 s pushing 768 MB to FUSE /sdcard), so 30 s
    // of nothing means the device went away.
    static constexpr int kTransferIdleTimeoutMs = 30000;
    // Stop the shell process
    void stop();

    // Exit code from most recent shellCommand's END marker; -1 if unparseable (timeout / broken session).
    int lastExitCode() const { return _last_exit_code.load(); }
    // Exit code of the most recent adbExecWithProgress() child process. kPtyExitKilled when we
    // killed it ourselves (Esc, or the idle bound); -1 when it died on a signal we did not send,
    // which is the only case where no status could be collected.
    static constexpr int kPtyExitKilled = -2;
    static int lastPtyExit() { return _last_pty_exit.load(); }

private:
    std::string _device_serial;
    FILE* _shell_pipe;
    int _shell_stdin;  // File descriptor for writing to shell stdin
    int _shell_pid;
    bool _is_running;
    std::string _last_error;
    std::atomic<int> _last_exit_code{-1};
    static std::atomic<int> _last_pty_exit;

    // Serializes shellCommand: write+read is one transaction, else callers cross-corrupt the pipe pair.
    std::mutex _shell_mutex;

    // Session management
    std::atomic<uint32_t> _command_counter;
    
    // Private methods
    static std::string findAdbExecutable();
    static std::vector<std::string> splitCommandArgs(const std::string& command);
    // Total wall-clock ceiling for one metadata `adb` invocation (device enumeration and the
    // like - transfers use the idle bound above instead, since they can be arbitrarily long).
    // 0 or negative disables it.
    static constexpr int kAdbProcessTimeoutMs = 30000;
    // Budget for the whole findAdbExecutable() search, shared by all candidate probes.
    static constexpr int kAdbSearchBudgetMs = 6000;
    static std::string runAdbProcess(const std::vector<std::string>& args, int timeout_ms = kAdbProcessTimeoutMs);
    static std::string runAdbProcessWithPty(const std::vector<std::string>& args, const std::function<void(const std::string&)>& on_chunk, const std::function<bool()>& abort_check = {}, int idle_timeout_ms = 0);
    std::string generateMarker();
    bool writeCommand(const std::string& command, const std::string& marker);
    std::string readResponse(const std::string& marker);
    void setError(const std::string& error);
};
