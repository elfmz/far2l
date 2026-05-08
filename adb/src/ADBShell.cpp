// Local includes
#include "ADBShell.h"
#include "ADBLog.h"

// Standard library includes
#include <cstring>
#include <sstream>
#include <chrono>
#include <cstdarg>
#include <algorithm>
#include <fstream>
#include <vector>
#include <mutex>

// System includes
#include <unistd.h>
#include <sys/wait.h>
#include <sys/ioctl.h>
#include <fcntl.h>
#include <poll.h>
#include <errno.h>
#include <signal.h>
#if defined(__APPLE__) || defined(__NetBSD__) || defined(__OpenBSD__)
#include <util.h>
#elif defined(__FreeBSD__) || defined(__DragonFly__)
#include <libutil.h>
#include <termios.h>
#else
#include <pty.h>
#endif


#include "ADBDevice.h"

static std::string _adbPath;
static std::mutex _adbPathMutex;

// Get current time in microseconds
inline uint64_t nowMicros() {
    using namespace std::chrono;
    return duration_cast<microseconds>(
        steady_clock::now().time_since_epoch()
    ).count();
}

ADBShell::ADBShell(const std::string& device_serial)
    : _device_serial(device_serial)
    , _shell_pipe(nullptr)
    , _shell_stdin(-1)
    , _shell_pid(-1)
    , _is_running(false)
    , _command_counter(0)
{
}

ADBShell::~ADBShell() {
    stop();
}

// Helper function to test if a path is a working ADB executable
static bool tryAdbPath(const std::string& path) {
    if (path.empty()) return false;
    DBG("Trying ADB path: %s\n", path.c_str());

    // Create temporary pipe for testing
    int pipefd[2] = {-1, -1};
    if (pipe(pipefd) < 0) {
        DBG("tryAdbPath: Failed to create pipe\n");
        return false;
    }

    pid_t pid = fork();
    if (pid < 0) {
        close(pipefd[0]);
        close(pipefd[1]);
        DBG("tryAdbPath: Failed to fork\n");
        return false;
    }

    if (pid == 0) {
        // Child process
        close(pipefd[0]);
        dup2(pipefd[1], STDOUT_FILENO);
        dup2(pipefd[1], STDERR_FILENO);
        close(pipefd[1]);
        execlp(path.c_str(), "adb", "version", (char*)nullptr);
        _exit(127);
    }

    // Parent process
    close(pipefd[1]);
    std::string output;
    char buffer[1024];
    while (true) {
        ssize_t n = read(pipefd[0], buffer, sizeof(buffer) - 1);
        if (n > 0) {
            buffer[n] = '\0';
            output += buffer;
        } else if (n == 0 || errno != EINTR) {
            break;
        }
    }
    close(pipefd[0]);

    int status = 0;
    waitpid(pid, &status, 0);

    DBG("Output from %s version: [%s]\n", path.c_str(), output.c_str());
    if (output.find("Android Debug Bridge") != std::string::npos ||
        output.find("Version") != std::string::npos ||
        output.find("version") != std::string::npos) {
        DBG("Found working ADB at: %s\n", path.c_str());
        return true;
    }
    return false;
}

std::string ADBShell::findAdbExecutable() {
    // Fast path: return cached result without expensive search
    {
        std::lock_guard<std::mutex> lock(_adbPathMutex);
        if (!_adbPath.empty()) return _adbPath;
    }

    // Search without holding the mutex: tryAdbPath() fork()s and holding a mutex across fork() is POSIX-unsafe.
    DBG("Starting ADB executable search...\n");

    std::string found;

    // Standard paths (includes macOS Homebrew + Linux system paths)
    const char* adb_paths[] = {
        "adb",
        "/opt/homebrew/bin/adb",
        "/usr/local/bin/adb",
        "/usr/bin/adb",
        nullptr
    };

    for (const char* path : adb_paths) {
        if (path && path[0] && tryAdbPath(path)) {
            found = path;
            break;
        }
    }

    if (found.empty()) {
        const char* android_home = getenv("ANDROID_HOME");
        DBG("ANDROID_HOME = %s\n", android_home ? android_home : "(not set)");
        if (android_home) {
            std::string sdk_path = std::string(android_home) + "/platform-tools/adb";
            if (tryAdbPath(sdk_path)) found = sdk_path;
        }
    }

    if (found.empty()) {
        const char* android_sdk_root = getenv("ANDROID_SDK_ROOT");
        DBG("ANDROID_SDK_ROOT = %s\n", android_sdk_root ? android_sdk_root : "(not set)");
        if (android_sdk_root) {
            std::string sdk_path = std::string(android_sdk_root) + "/platform-tools/adb";
            if (tryAdbPath(sdk_path)) found = sdk_path;
        }
    }

    if (found.empty()) {
        const char* home = getenv("HOME");
        DBG("HOME = %s\n", home ? home : "(not set)");
        if (home) {
            // macOS: ~/Library/Android/sdk/platform-tools/adb
            std::string mac_adb = std::string(home) + "/Library/Android/sdk/platform-tools/adb";
            if (tryAdbPath(mac_adb)) {
                found = mac_adb;
            } else {
                // Linux: ~/Android/sdk/platform-tools/adb
                std::string linux_adb = std::string(home) + "/Android/sdk/platform-tools/adb";
                if (tryAdbPath(linux_adb)) found = linux_adb;
            }
        }
    }

    if (found.empty()) {
        DBG("ERROR: Could not find ADB executable anywhere!\n");
        return "";
    }

    // Cache result under lock
    {
        std::lock_guard<std::mutex> lock(_adbPathMutex);
        _adbPath = found;
    }
    return found;
}

// Marker suffixes bracketing each command's response; shared so writeCommand emitter and readResponse parser can't drift.
static constexpr const char *kMarkerStartSuffix = "_S__";
static constexpr const char *kMarkerEndPrefix = "_E_";

std::string ADBShell::generateMarker() {
    uint64_t micros = nowMicros();
    uint32_t current_counter = _command_counter.fetch_add(1);
    return "__MARK_" + std::to_string(micros) + "_" + std::to_string(current_counter) + "__";
}

// Internal helper to prepare argv for exec
static std::vector<char*> PrepareArgv(const std::string& adbPath, const std::string& serial, const std::vector<std::string>& args, std::vector<std::string>& storage) {
    storage.clear();
    storage.push_back(adbPath);
    if (!serial.empty()) {
        storage.push_back("-s");
        storage.push_back(serial);
    }
    storage.insert(storage.end(), args.begin(), args.end());

    std::vector<char*> argv;
    for (auto& s : storage) {
        argv.push_back(const_cast<char*>(s.c_str()));
    }
    argv.push_back(nullptr);
    return argv;
}

static void SetupChildEnv() {
    setenv("TERM", "xterm-256color", 1);
    setenv("TERM_PROGRAM", "far2l", 1);
    setenv("LANG", "en_US.UTF-8", 1);
    setenv("LC_ALL", "en_US.UTF-8", 1);
}

bool ADBShell::start() {
    if (_is_running) {
        return true; // Already running
    }

    std::string adbPath = findAdbExecutable();
    if (adbPath.empty()) {
        setError("ADB executable not found");
        return false;
    }

    int pipe_stdin[2] = {-1, -1};
    int pipe_stdout[2] = {-1, -1};

    if (pipe(pipe_stdin) < 0) {
        setError("Failed to create stdin pipe");
        return false;
    }
    if (pipe(pipe_stdout) < 0) {
        setError("Failed to create stdout pipe");
        close(pipe_stdin[0]);
        close(pipe_stdin[1]);
        return false;
    }

    pid_t pid = fork();
    if (pid == 0) {
        // --- Child ---
        close(pipe_stdin[1]);
        close(pipe_stdout[0]);

        if (dup2(pipe_stdin[0], STDIN_FILENO) < 0 ||
            dup2(pipe_stdout[1], STDOUT_FILENO) < 0 ||
            dup2(pipe_stdout[1], STDERR_FILENO) < 0) {
            _exit(1);
        }

        // Close inherited FDs
        close(pipe_stdin[0]);
        close(pipe_stdout[1]);

        SetupChildEnv();
        std::vector<std::string> storage;
        // `-T` disables device-side PTY; without it adb echoes stdin into stdout and the marker matches its own echo (prev P3).
        std::vector<char*> argv = PrepareArgv(adbPath, _device_serial, {"shell", "-T"}, storage);
        execvp(adbPath.c_str(), argv.data());
        _exit(127);
    }
    else if (pid > 0) {
        // --- Parent ---
        close(pipe_stdin[0]);
        close(pipe_stdout[1]);

        _shell_pipe = fdopen(pipe_stdout[0], "r");
        _shell_stdin = pipe_stdin[1];
        _shell_pid = pid;

        if (!_shell_pipe) {
            setError("Failed to fdopen stdout pipe");
            close(pipe_stdout[0]);
            close(pipe_stdin[1]);
            // Kill orphaned child to prevent zombie
            kill(pid, SIGKILL);
            waitpid(pid, nullptr, 0);
            return false;
        }

        // Check if adb died instantly
        int status;
        if (waitpid(pid, &status, WNOHANG) > 0) {
            setError("ADB shell terminated immediately");
            fclose(_shell_pipe);
            close(_shell_stdin);
            _shell_pipe = nullptr;
            _shell_stdin = -1;
            return false;
        }

        _is_running = true;
        _last_error.clear();
        return true;
    }
    else {
        setError("Failed to fork process");
        close(pipe_stdin[0]); close(pipe_stdin[1]);
        close(pipe_stdout[0]); close(pipe_stdout[1]);
        return false;
    }
}

// Escape CR/LF/TAB/non-printables so debug logs show exactly what came across the pipe.
#if defined(DEBUG) || defined(_DEBUG)
static std::string EscapeForLog(const std::string& s, size_t limit = 200) {
    std::string out;
    out.reserve(s.size() + 16);
    size_t n = std::min(s.size(), limit);
    for (size_t i = 0; i < n; ++i) {
        unsigned char c = (unsigned char)s[i];
        switch (c) {
            case '\r': out += "\\r"; break;
            case '\n': out += "\\n"; break;
            case '\t': out += "\\t"; break;
            case '\\': out += "\\\\"; break;
            default:
                if (c < 0x20 || c == 0x7f) {
                    char buf[8];
                    snprintf(buf, sizeof(buf), "\\x%02x", c);
                    out += buf;
                } else {
                    out += (char)c;
                }
        }
    }
    if (s.size() > limit) out += "...";
    return out;
}
#endif

bool ADBShell::writeCommand(const std::string& command, const std::string& marker) {
    if (!_is_running || _shell_stdin == -1) {
        setError("Shell not running");
        return false;
    }

#if defined(DEBUG) || defined(_DEBUG)
    // Non-destructive peek via FIONREAD — reading would destroy stale bytes the protocol wants to log and discard as pre-START noise.
    if (_shell_pipe) {
        int rfd = fileno(_shell_pipe);
        int queued = 0;
        if (rfd >= 0 && ioctl(rfd, FIONREAD, &queued) == 0) {
            DBG("pre-write pipe queue: %d bytes (non-destructive peek)\n", queued);
        }
    }
#endif

    // Protocol: echo <START>\n { <cmd>; } </dev/null 2>&1 \n echo <END>$?__ — </dev/null seals stdin-readers (P1), 2>&1 orders streams (P2), <START> bounds response top against prior-cmd noise (P4), <END>$?__ carries exit code.
    const std::string start_marker = marker + kMarkerStartSuffix;
    const std::string end_marker_prefix = marker + kMarkerEndPrefix;
    std::string wrapped =
        "echo " + start_marker + "\n"
        "{ " + command + "; } < /dev/null 2>&1\n"
        "echo " + end_marker_prefix + "$?__\n";

    DBG("sending %zu bytes: '%s'\n", wrapped.size(),
#if defined(DEBUG) || defined(_DEBUG)
        EscapeForLog(wrapped).c_str()
#else
        ""
#endif
    );

    // Retry partial writes and EINTR: a signal can interrupt write() and pipes don't
    // guarantee one-shot atomicity for writes > PIPE_BUF (4KB on Linux, 512 on BSD).
    const char *p = wrapped.c_str();
    size_t remaining = wrapped.length();
    while (remaining > 0) {
        ssize_t n = write(_shell_stdin, p, remaining);
        if (n > 0) {
            p += (size_t)n;
            remaining -= (size_t)n;
            continue;
        }
        if (n < 0 && errno == EINTR) continue;
        setError("Failed to write command to shell (errno=" + std::to_string(errno) + ")");
        return false;
    }
    return true;
}

std::string ADBShell::readResponse(const std::string& marker) {
    if (!_is_running || !_shell_pipe) {
        setError("Shell not running");
        return "";
    }

    int fd = fileno(_shell_pipe);
    if (fd < 0) {
        setError("Invalid file descriptor");
        return "";
    }

    const std::string start_marker = marker + "_S__";
    const std::string end_marker_prefix = marker + "_E_";

    std::string raw;
    char buffer[4096];
    bool end_found = false;
    size_t end_pos = std::string::npos;
    // Search floor: rescanning whole `raw` on every chunk is O(n²) for large outputs (e.g. big `cat`); advance past safely-scanned prefix.
    size_t end_search_from = 0;
    _last_exit_code = -1;
    // Only consumed by DBG(); suppress unused-variable warning in release builds where DBG expands to nothing.
    [[maybe_unused]] size_t total_read = 0;
    [[maybe_unused]] int read_chunks = 0;

    constexpr int kReadTimeoutMs = 30000;
    while (!end_found) {
        struct pollfd pfd{};
        pfd.fd = fd;
        pfd.events = POLLIN | POLLHUP | POLLERR;
        int poll_result = poll(&pfd, 1, kReadTimeoutMs);
        if (poll_result == 0) {
            setError("Timeout waiting for ADB shell response");
            break;
        }
        if (poll_result < 0) {
            if (errno == EINTR) continue;
            setError("Poll failed while reading ADB shell response");
            break;
        }

        ssize_t bytes_read = read(fd, buffer, sizeof(buffer));
        if (bytes_read > 0) {
            size_t old_size = raw.size();
            raw.append(buffer, (size_t)bytes_read);
            total_read += (size_t)bytes_read;
            read_chunks++;

            // Match full <prefix><digits>__ so we don't truncate mid-digit when the marker line arrives in pieces.
            size_t ep = raw.find(end_marker_prefix, end_search_from);
            if (ep != std::string::npos) {
                size_t digits_start = ep + end_marker_prefix.size();
                size_t term = raw.find("__", digits_start);
                if (term != std::string::npos && term > digits_start) {
                    std::string digits = raw.substr(digits_start, term - digits_start);
                    bool all_digits = !digits.empty() && std::all_of(digits.begin(), digits.end(),
                        [](char c) { return c >= '0' && c <= '9'; });
                    if (all_digits) {
                        // Guard against out_of_range/invalid_argument if device output somehow produces an absurd digit run.
                        try {
                            _last_exit_code = std::stoi(digits);
                        } catch (...) {
                            _last_exit_code = -1;
                        }
                        end_pos = ep;
                        end_found = true;
                    }
                }
            }
            // Advance floor but leave (prefix_len-1) byte overlap so a marker straddling a chunk boundary still matches.
            if (!end_found) {
                size_t prefix_len = end_marker_prefix.size();
                end_search_from = (old_size + (size_t)bytes_read > prefix_len)
                                  ? (old_size + (size_t)bytes_read) - (prefix_len - 1)
                                  : 0;
            }
        } else if (bytes_read == 0) {
            setError("Unexpected EOF from ADB shell");
            // EOF = shell exited; mark dead so next shellCommand restarts instead of writing into a closed pipe.
            _is_running = false;
            break;
        } else {
            if (errno == EINTR) continue;
            setError("Error reading from ADB shell: " + std::to_string(errno));
            // EPIPE / ECONNRESET / EBADF all mean the pipe is unusable.
            if (errno == EPIPE || errno == ECONNRESET || errno == EBADF) {
                _is_running = false;
            }
            break;
        }
    }

    if (!end_found) {
        DBG("end marker NOT found (timeout/error) total_read=%zu chunks=%d raw_head='%s'\n",
            total_read, read_chunks,
#if defined(DEBUG) || defined(_DEBUG)
            EscapeForLog(raw.substr(0, 200)).c_str()
#else
            ""
#endif
        );
        return "";
    }

    // Pre-START noise = leftover from prior hung commands or device boot messages on a fresh session; discard.
    size_t sp = raw.find(start_marker);
    size_t content_start = 0;
    if (sp != std::string::npos) {
        content_start = sp + start_marker.size();
        // Skip the newline that follows the start marker echo
        if (content_start < raw.size() && raw[content_start] == '\r') content_start++;
        if (content_start < raw.size() && raw[content_start] == '\n') content_start++;
    } else {
        DBG("WARN: start marker not found — returning raw up to end marker\n");
    }

    // Content is between content_start and end_pos (start of END marker line); trailing newlines trimmed below.
    if (end_pos < content_start) {
        // Out-of-order markers — shouldn't happen under the new protocol; fall through to empty output.
        DBG("WARN: end marker appeared before start marker (end=%zu start=%zu)\n",
            end_pos, sp);
        return "";
    }
    std::string output = raw.substr(content_start, end_pos - content_start);
    ADBUtils::TrimTrailingNewlines(output);

#if defined(DEBUG) || defined(_DEBUG)
    // Raw bytes between START/END (pre-filter) for diagnosing interleavings/escapes/prompts; head+tail to avoid flooding on `ls /`.
    std::string between = (sp != std::string::npos && end_pos > content_start)
                          ? raw.substr(content_start, end_pos - content_start)
                          : std::string();
    size_t between_sz = between.size();
    if (between_sz <= 400) {
        DBG("raw-between-markers (%zu B): '%s'\n",
            between_sz, EscapeForLog(between, 400).c_str());
    } else {
        DBG("raw-between-markers (%zu B) head='%s' tail='%s'\n",
            between_sz,
            EscapeForLog(between.substr(0, 200), 200).c_str(),
            EscapeForLog(between.substr(between_sz - 200), 200).c_str());
    }
#endif
    DBG("end_found exit=%d pre_start_discarded=%zu content=%zu chunks=%d total=%zu head80='%s'\n",
        _last_exit_code.load(), (sp != std::string::npos ? sp : (size_t)0),
        output.size(), read_chunks, total_read,
#if defined(DEBUG) || defined(_DEBUG)
        EscapeForLog(output.substr(0, 80)).c_str()
#else
        ""
#endif
    );
    return output;
}

std::string ADBShell::shellCommand(const std::string& command) {
    // Empty command would expand to `{ ; } < /dev/null 2>&1` (shell syntax error); short-circuit. Preserve previous exit code so callers aren't misled by a bogus 0.
    if (command.empty()) {
        return "";
    }

    // Serialize: writeCommand+readResponse is one transaction; parallel callers would cross-corrupt the pipe pair.
    std::lock_guard<std::mutex> lock(_shell_mutex);

    if (!_is_running) {
        if (!start()) {
            return "";
        }
    }
    std::string marker = generateMarker();
    if (!writeCommand(command, marker)) {
        // Session may have died (EPIPE / shell exit); restart once so next caller isn't stuck against a zombie _is_running flag.
        _is_running = false;
        stop();
        if (!start()) {
            return "";
        }
        if (!writeCommand(command, marker)) {
            return "";
        }
    }
    std::string output = readResponse(marker);
    return output;
}

void ADBShell::stop() {
    if (_shell_stdin != -1) {
        close(_shell_stdin);
        _shell_stdin = -1;
    }
    if (_shell_pipe) {
        fclose(_shell_pipe);
        _shell_pipe = nullptr;
    }
    if (_shell_pid > 0) {
        int status = 0;
        pid_t waited = waitpid(_shell_pid, &status, WNOHANG);
        if (waited == 0) {
            kill(_shell_pid, SIGTERM);
            for (int i = 0; i < 10; ++i) {
                waited = waitpid(_shell_pid, &status, WNOHANG);
                if (waited == _shell_pid) {
                    break;
                }
                usleep(10000);
            }
            if (waited == 0) {
                kill(_shell_pid, SIGKILL);
                waitpid(_shell_pid, &status, 0);
            }
        }
    }
    _is_running = false;
    _shell_pid = -1;
}

void ADBShell::setError(const std::string& error) {
    _last_error = error;
}

// Static version for global ADB commands (e.g., "adb devices -l")
std::string ADBShell::adbExec(const std::string& command) {
    std::vector<std::string> args = splitCommandArgs(command);
    return adbExec(args);
}

std::string ADBShell::adbExec(const std::vector<std::string>& args) {
    return runAdbProcess(args);
}

std::string ADBShell::adbExec(const std::vector<std::string>& args, const std::function<void(const std::string&)>& on_chunk) {
    return runAdbProcess(args, &on_chunk);
}

std::vector<std::string> ADBShell::splitCommandArgs(const std::string& command) {
    std::vector<std::string> args;
    std::string current;
    bool in_single = false;
    bool in_double = false;
    bool escaped = false;

    for (char ch : command) {
        if (escaped) {
            current.push_back(ch);
            escaped = false;
            continue;
        }
        if (ch == '\\') {
            escaped = true;
            continue;
        }
        if (ch == '\'' && !in_double) {
            in_single = !in_single;
            continue;
        }
        if (ch == '"' && !in_single) {
            in_double = !in_double;
            continue;
        }
        if (!in_single && !in_double && (ch == ' ' || ch == '\t' || ch == '\n' || ch == '\r')) {
            if (!current.empty()) {
                args.push_back(current);
                current.clear();
            }
            continue;
        }
        current.push_back(ch);
    }
    if (escaped) {
        current.push_back('\\');
    }
    if (!current.empty()) {
        args.push_back(current);
    }
    return args;
}

std::string ADBShell::runAdbProcess(const std::vector<std::string>& args, const std::function<void(const std::string&)>* on_chunk) {
    std::string adbPath = findAdbExecutable();
    if (adbPath.empty()) {
        DBG("runAdbProcess: ADB path is empty, cannot execute\n");
        return "";
    }

    DBG("runAdbProcess: executing %s with %zu args\n", adbPath.c_str(), args.size());

    int pipefd[2] = {-1, -1};
    if (pipe(pipefd) < 0) {
        DBG("runAdbProcess: Failed to create pipe, errno=%d\n", errno);
        return "";
    }

    pid_t pid = fork();
    if (pid < 0) {
        DBG("runAdbProcess: Failed to fork, errno=%d\n", errno);
        close(pipefd[0]);
        close(pipefd[1]);
        return "";
    }
    if (pid == 0) {
        close(pipefd[0]);
        dup2(pipefd[1], STDOUT_FILENO);
        dup2(pipefd[1], STDERR_FILENO);
        close(pipefd[1]);

        SetupChildEnv();
        std::vector<std::string> storage;
        std::vector<char*> argv = PrepareArgv(adbPath, "", args, storage);
        execvp(adbPath.c_str(), argv.data());
        _exit(127);
    }

    close(pipefd[1]);
    std::string output;
    char buffer[4096];
    while (true) {
        ssize_t n = read(pipefd[0], buffer, sizeof(buffer));
        if (n > 0) {
            output.append(buffer, static_cast<size_t>(n));
            if (on_chunk) {
                (*on_chunk)(std::string(buffer, static_cast<size_t>(n)));
            }
            continue;
        }
        if (n == 0) break;
        if (errno == EINTR) continue;
        break;
    }
    close(pipefd[0]);

    int status = 0;
    waitpid(pid, &status, 0);
    ADBUtils::TrimTrailingNewlines(output);
    return output;
}

std::string ADBShell::runAdbProcessWithPty(const std::vector<std::string>& args, const std::function<void(const std::string&)>& on_chunk, const std::function<bool()>& abort_check) {
    std::string adbPath = findAdbExecutable();
    if (adbPath.empty()) return "";

    struct winsize win = {};
    win.ws_col = 80; win.ws_row = 24;

    int master_fd = -1;
    pid_t pid = forkpty(&master_fd, nullptr, nullptr, &win);
    if (pid < 0) return "";

    if (pid == 0) {
        SetupChildEnv();
        std::vector<std::string> storage;
        std::vector<char*> argv = PrepareArgv(adbPath, "", args, storage);
        execvp(adbPath.c_str(), argv.data());
        _exit(127);
    }

    std::string output;
    char buffer[4096];
    bool aborted = false;

    int flags = fcntl(master_fd, F_GETFL, 0);
    fcntl(master_fd, F_SETFL, flags | O_NONBLOCK);

    while (true) {
        if (abort_check && abort_check()) {
            aborted = true;
            break;
        }

        struct pollfd pfd{};
        pfd.fd = master_fd;
        pfd.events = POLLIN | POLLHUP | POLLERR;
        int poll_result = poll(&pfd, 1, 200);

        if (poll_result == 0) continue;
        if (poll_result < 0) {
            if (errno == EINTR) continue;
            break;
        }

        if (pfd.revents & POLLIN) {
            ssize_t n = read(master_fd, buffer, sizeof(buffer));
            if (n > 0) {
                output.append(buffer, static_cast<size_t>(n));
                on_chunk(std::string(buffer, static_cast<size_t>(n)));
            } else if (n == 0) {
                break;
            } else if (errno != EINTR && errno != EAGAIN) {
                break;
            }
        }

        // Check POLLHUP after POLLIN — on Linux both can be set simultaneously
        if (pfd.revents & (POLLHUP | POLLERR)) break;
    }

    close(master_fd);
    if (aborted) {
        kill(pid, SIGTERM);
        usleep(100000);
        kill(pid, SIGKILL);
    }

    int status = 0;
    waitpid(pid, &status, 0);
    ADBUtils::TrimTrailingNewlines(output);
    return output;
}

std::string ADBShell::adbExecWithProgress(const std::vector<std::string>& args, const std::function<void(const std::string&)>& on_chunk, const std::function<bool()>& abort_check) {
    return runAdbProcessWithPty(args, on_chunk, abort_check);
}

// Instance version for device-specific ADB commands with -s <device_serial>
std::string ADBShell::adbCommand(const std::string& command) const {
    // Construct device-specific command with -s <device_serial>
    std::string device_command;
    if (!_device_serial.empty()) {
        device_command = "-s " + _device_serial + " " + command;
    } else {
        device_command = command;
    }
    return ADBShell::adbExec(device_command);
}
