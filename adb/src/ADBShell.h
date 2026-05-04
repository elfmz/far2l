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
    static std::string adbExec(const std::vector<std::string>& args, const std::function<void(const std::string&)>& on_chunk);
    static std::string adbExecWithProgress(const std::vector<std::string>& args, const std::function<void(const std::string&)>& on_chunk, const std::function<bool()>& abort_check = {});
    // Stop the shell process
    void stop();

    // Exit code from most recent shellCommand's END marker; -1 if unparseable (timeout / broken session).
    int lastExitCode() const { return _last_exit_code.load(); }
    // Exit code of the most recent adbExecWithProgress() child process; -1 if unset / signal.
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
    static std::string runAdbProcess(const std::vector<std::string>& args, const std::function<void(const std::string&)>* on_chunk = nullptr);
    static std::string runAdbProcessWithPty(const std::vector<std::string>& args, const std::function<void(const std::string&)>& on_chunk, const std::function<bool()>& abort_check = {});
    std::string generateMarker();
    bool writeCommand(const std::string& command, const std::string& marker);
    std::string readResponse(const std::string& marker);
    void setError(const std::string& error);
};
