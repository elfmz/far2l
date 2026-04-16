#include "ADBDevice.h"
#include "ADBShell.h"
#include "ADBLog.h"
#include <sstream>
#include <cstring>
#include <stdexcept>
#include <cstdlib>
#include <unistd.h>
#include <sys/wait.h>
#include <fcntl.h>
#include <wchar.h>
#include <errno.h>
#include <utils.h>
#include <fstream>
#include <chrono>
#include <cctype>

// ============================================================================
// ADBUtils namespace implementation
// ============================================================================

namespace ADBUtils {

std::string ShellQuote(const std::string &value)
{
    std::string quoted = "'";
    for (char c : value) {
        if (c == '\'') {
            quoted += "'\"'\"'";
        } else {
            quoted.push_back(c);
        }
    }
    quoted += "'";
    return quoted;
}

std::string JoinPath(const std::string& base, const std::string& component)
{
    if (base.empty()) return component;
    if (component.empty()) return base;

    bool baseEndsSep = (!base.empty() && (base.back() == '/' || base.back() == '\\'));
    bool compStartsSep = (!component.empty() && (component.front() == '/' || component.front() == '\\'));

    if (baseEndsSep && compStartsSep) {
        return base + component.substr(1);
    } else if (!baseEndsSep && !compStartsSep) {
        return base + "/" + component;
    }
    return base + component;
}

void TrimTrailingNewlines(std::string& s)
{
    while (!s.empty() && (s.back() == '\n' || s.back() == '\r')) {
        s.pop_back();
    }
}

int CheckConnection(bool connected)
{
    return connected ? 0 : EIO;
}

} // namespace ADBUtils


// ============================================================================
// ProgressParser implementation
// ============================================================================

ProgressParser::ProgressParser(std::function<void(int)> on_progress, bool debug_log)
    : _on_progress(std::move(on_progress)), _debug_log(debug_log), _last_percent(-1) {}

void ProgressParser::operator()(const std::string &chunk) {
    if (_debug_log) {
        DBG(" PTY chunk received (%zu bytes): [", chunk.size());
        for (unsigned char c : chunk) {
            if (c >= 32 && c < 127) {
                DBG("%c", c);
            } else {
                DBG("\\x%02X", c);
            }
        }
        DBG("]\n");
    }

    _pending += chunk;
    size_t split = 0;
    while ((split = _pending.find_first_of("\r\n")) != std::string::npos) {
        std::string line = _pending.substr(0, split);
        _pending.erase(0, split + 1);
        if (_debug_log) {
            DBG("Parsing line: [%s]\n", line.c_str());
        }
        const int percent = ExtractPercent(line);
        if (_debug_log) {
            DBG("Extracted percent: %d\n", percent);
        }
        if (percent >= 0 && percent != _last_percent) {
            _last_percent = percent;
            if (_debug_log) {
                DBG("Calling on_progress(%d)\n", percent);
            }
            _on_progress(percent);
        }
    }
}

void ProgressParser::drain() {
    if (!_pending.empty()) {
        if (_debug_log) {
            DBG("Pending tail: [%s]\n", _pending.c_str());
        }
        const int tail_percent = ExtractPercent(_pending);
        if (tail_percent >= 0 && tail_percent != _last_percent) {
            _on_progress(tail_percent);
        }
        _pending.clear();
    }
}

void ProgressParser::complete() {
    _on_progress(100);
}

void ProgressParser::start() {
    _on_progress(0);
}

int ProgressParser::ExtractPercent(const std::string &s) {
    size_t p = s.find('%');
    if (p == std::string::npos || p == 0) {
        return -1;
    }
    size_t start = p;
    while (start > 0 && std::isdigit(static_cast<unsigned char>(s[start - 1]))) {
        --start;
    }
    if (start == p) {
        return -1;
    }
    int percent = std::atoi(s.substr(start, p - start).c_str());
    if (percent < 0 || percent > 100) {
        return -1;
    }
    return percent;
}

ADBDevice::ADBDevice(const std::string &device_serial)
    : _device_serial(device_serial), _current_path("/"), _adb_shell(nullptr), _connected(false)
{
    
    
    Connect();
    
}

ADBDevice::~ADBDevice()
{
    Disconnect();
}

bool ADBDevice::Connect()
{
    
    if (_connected) {
        return true;
    }
    
    try {
        
        _adb_shell = std::make_unique<ADBShell>(_device_serial);
        
        
        if (!_adb_shell->start()) {
            return false;
        }
        
        
        std::string pwd_response = _adb_shell->shellCommand("pwd");
        
        
        if (pwd_response.empty()) {
            return false;
        }
        _current_path = ExtractPathFromPwd(pwd_response);
        _connected = true;
        
        
        return true;
        
    } catch (const std::exception& e) {
        return false;
    }
}

void ADBDevice::Disconnect()
{
    if (_adb_shell) {
        _adb_shell->stop();
        _adb_shell.reset();
    }
    _connected = false;
}

void ADBDevice::EnsureConnection()
{
    if (!_connected || !_adb_shell) {
        if (!Connect()) {
            throw std::runtime_error("Failed to connect to ADB shell");
        }
    }
}

std::vector<std::string> ADBDevice::BuildArgs(const std::vector<std::string>& args) const
{
    std::vector<std::string> full_args;
    if (!_device_serial.empty()) {
        full_args.emplace_back("-s");
        full_args.emplace_back(_device_serial);
    }
    full_args.insert(full_args.end(), args.begin(), args.end());
    return full_args;
}

std::string ADBDevice::RunAdbCommand(const std::string &command) {
    if (_device_serial.empty()) {
        return ADBShell::adbExec(command);
    }
    return ADBShell::adbExec("-s " + _device_serial + " " + command);
}

std::string ADBDevice::RunAdbCommand(const std::vector<std::string> &args) {
    return ADBShell::adbExec(BuildArgs(args));
}

std::string ADBDevice::RunAdbCommand(const std::vector<std::string> &args, const std::function<void(const std::string&)> &on_chunk) {
    return ADBShell::adbExec(BuildArgs(args), on_chunk);
}

std::string ADBDevice::RunAdbCommandWithProgress(const std::vector<std::string> &args, const std::function<void(const std::string&)> &on_chunk, const std::function<bool()> &abort_check) {
    return ADBShell::adbExecWithProgress(BuildArgs(args), on_chunk, abort_check);
}

bool ADBDevice::IsSuccessResult(const std::string& result, bool is_push) const
{
    if (result.find("skipped") != std::string::npos) return true;

    if (is_push) {
        return result.find("file pushed") != std::string::npos ||
               result.find("files pushed") != std::string::npos;
    } else {
        return result.find("file pulled") != std::string::npos ||
               result.find("files pulled") != std::string::npos;
    }
}

std::string ADBDevice::RunShellCommand(const std::string &command)
{
    EnsureConnection();
    return _adb_shell->shellCommand(command);
}

std::string ADBDevice::GetCurrentWorkingDirectory()
{
    if (!_connected || !_adb_shell) {
        return "/";
    }

    try {
        std::string pwd_output = _adb_shell->shellCommand("pwd");
        return ExtractPathFromPwd(pwd_output);
    } catch (const std::exception& e) {
        return "/";
    }
}

void ADBDevice::SyncPath()
{
    if (!_connected || !_adb_shell) {
        return;
    }
    try {
        std::string pwd_output = _adb_shell->shellCommand("pwd");
        _current_path = ExtractPathFromPwd(pwd_output);
    } catch (const std::exception& e) {
        // Ignore - keep current path
    }
}

void ADBDevice::RunShellCommandStreaming(const std::string &command, const std::function<void(const std::string&)> &on_line)
{
    // Build args: shell, -c, command
    std::vector<std::string> args = {"shell", "-c", command};

    // Buffer for incomplete lines
    std::string buffer;

    // Callback to process chunks and extract complete lines
    auto process_chunk = [&](const std::string &chunk) {
        buffer += chunk;
        size_t pos;
        while ((pos = buffer.find('\n')) != std::string::npos) {
            std::string line = buffer.substr(0, pos);
            buffer.erase(0, pos + 1);
            // Trim CR
            if (!line.empty() && line.back() == '\r') {
                line.pop_back();
            }
            // Skip lines that are just shell prompts (e.g., "/$", "/#")
            if (line == "/$" || line == "/#" || line == "$" || line == "#") {
                continue;
            }
            on_line(line);
        }
    };

    // Use PTY-based execution for streaming
    RunAdbCommandWithProgress(args, process_chunk, [] { return false; });

    // Flush remaining buffer
    if (!buffer.empty()) {
        if (!buffer.empty() && buffer.back() == '\r') {
            buffer.pop_back();
        }
        if (!buffer.empty()) {
            on_line(buffer);
        }
    }
}

wchar_t* ADBDevice::AllocateItemString(const std::string& s) {
    std::wstring ws = s.empty() ? std::wstring() : StrMB2Wide(s);
    size_t len = ws.length() + 1;
    wchar_t* buf = (wchar_t*)malloc(len * sizeof(wchar_t));
    if (!buf) {
        throw std::bad_alloc();
    }
    wcscpy(buf, ws.c_str());
    return buf;
}

std::string ADBDevice::DirectoryEnum(const std::string &path, std::vector<PluginPanelItem> &files)
{

    if (!_connected || !_adb_shell) {
        throw std::runtime_error("ADB shell not connected");
    }

    // Validate path doesn't contain problematic characters that could break bulk command parsing
    if (path.find('\0') != std::string::npos || path.find('\n') != std::string::npos) {
        throw std::runtime_error("Invalid path: contains null or newline characters");
    }

    const std::string separator = "<<<!>>>";
    const std::string arrow = ":->";

    // Bulk command: cd, pwd, ls -la, then symlink info (properly quote path for spaces)
    std::ostringstream bulk_cmd;
    bulk_cmd << "cd " << ADBUtils::ShellQuote(path) << " 2>/dev/null; pwd; ls -la; echo \"" << separator << "\";"
             << "for f in *; do "
             << "[ -L \"$f\" ] && ([ -d \"$f\" ] && echo \"$f" << arrow << "D\" "
             << "|| ([ -f \"$f\" ] && echo \"$f" << arrow << "F\" || echo \"$f" << arrow << "B\")); "
             << "done";
    
    std::string bulk_output = RunShellCommand(bulk_cmd.str());

    std::vector<std::string> ls_lines;
    std::vector<std::string> symlink_info;
    std::string current_path;
    bool after_separator = false;

    // Split lines once
    std::istringstream output_stream(bulk_output);
    std::string line;
    while (std::getline(output_stream, line)) {
        if (line.empty()) continue;
        if (line == separator) { after_separator = true; continue; }

        if (!after_separator) {
            if (current_path.empty()) {
                current_path = ExtractPathFromPwd(line);
                _current_path = current_path;
            } else {
                ls_lines.push_back(line);
            }
        } else {
            symlink_info.push_back(line);
        }
    }

    // Add hardcoded ".." entry
    files.clear();

    // Parse ls -la lines
    for (const auto& ls_line : ls_lines) {
        if (ls_line.find("Permission denied") != std::string::npos || ls_line.find("total") == 0)
            continue;
        if (ls_line.size() > 0 && ls_line[0] == '?')
            continue;

        std::istringstream ls_stream(ls_line);
        std::string perms, links, owner, group, size, date, time_str;
        if (!(ls_stream >> perms >> links >> owner >> group >> size >> date >> time_str))
            continue;

        std::string rest;
        std::getline(ls_stream, rest);
        if (!rest.empty() && rest[0] == ' ') rest.erase(0, 1);

        std::string filename = rest;
        std::string symlink_target;
        bool is_symlink = (perms[0] == 'l');
        if (is_symlink) {
            auto pos = rest.find(" -> ");
            if (pos != std::string::npos) {
                filename = rest.substr(0, pos);
                symlink_target = rest.substr(pos + 4);
            }
        }

        if (filename.empty() || filename == "." || filename == "..") continue;

        PluginPanelItem item{};
        item.FindData.lpwszFileName = AllocateItemString(filename);
        item.FindData.dwUnixMode = (perms[0] == 'd') ? (S_IFDIR | 0755) : (is_symlink ? (S_IFLNK | 0644) : (S_IFREG | 0644));
        item.FindData.dwFileAttributes = WINPORT(EvaluateAttributesA)(item.FindData.dwUnixMode, filename.c_str());
        if (perms[0] == 'd') item.FindData.dwFileAttributes |= FILE_ATTRIBUTE_DIRECTORY;

        if (is_symlink) {
            item.Description = AllocateItemString(symlink_target.empty() ? "Symlink (no target)" : symlink_target);
        }

        try { item.FindData.nFileSize = item.FindData.nPhysicalSize = std::stoull(size); } catch (...) { item.FindData.nFileSize = item.FindData.nPhysicalSize = 0; }

        item.Owner = AllocateItemString(owner);
        item.Group = AllocateItemString(group);
        
        try { item.NumberOfLinks = std::stoi(links); } catch (...) { item.NumberOfLinks = 1; }

        FILETIME ft{};
        time_t t = ParseLsDateTime(date, time_str);
        if (!t) t = time(nullptr);
        ULARGE_INTEGER uli; uli.QuadPart = (t * 10000000ULL) + 116444736000000000ULL;
        ft.dwLowDateTime = uli.LowPart; ft.dwHighDateTime = uli.HighPart;
        item.FindData.ftCreationTime = item.FindData.ftLastAccessTime = item.FindData.ftLastWriteTime = ft;

        files.push_back(item);
    }

    // Map filenames for fast symlink update
    std::unordered_map<std::string, PluginPanelItem*> file_map;
    for (auto& f : files)
        		if (f.FindData.lpwszFileName)
			file_map[StrWide2MB(f.FindData.lpwszFileName)] = &f;

    for (const auto& symlink_line : symlink_info) {
        auto colon_pos = symlink_line.find(arrow);
        if (colon_pos == std::string::npos) continue;
        std::string filename = symlink_line.substr(0, colon_pos);
        std::string type = symlink_line.substr(colon_pos + arrow.size());

        auto it = file_map.find(filename);
        if (it != file_map.end()) {
            PluginPanelItem* file = it->second;
            if (type == "D") file->FindData.dwFileAttributes |= FILE_ATTRIBUTE_DIRECTORY;
            // F and B need no action
        }
    }

    return current_path.empty() ? path : current_path;
}

std::string ADBDevice::ExtractPathFromPwd(const std::string &pwd_output)
{
    std::string path = pwd_output;
    ADBUtils::TrimTrailingNewlines(path);
    return path;
}

time_t ADBDevice::ParseLsDateTime(const std::string &date, const std::string &time_str) {
    struct tm timeinfo = {};
    time_t result = 0;
    int hour = 0, minute = 0;

    // Parse time if present
    sscanf(time_str.c_str(), "%d:%d", &hour, &minute);
    timeinfo.tm_hour = hour;
    timeinfo.tm_min = minute;
    timeinfo.tm_sec = 0;

    if (date.find('-') != std::string::npos) {
        int year, month, day;
        if (sscanf(date.c_str(), "%d-%d-%d", &year, &month, &day) == 3) {
            timeinfo.tm_year = year - 1900;
            timeinfo.tm_mon = month - 1;
            timeinfo.tm_mday = day;
            result = mktime(&timeinfo);
        }
    } else {
        // "MMM DD" format
        const char* months[] = {"Jan","Feb","Mar","Apr","May","Jun","Jul","Aug","Sep","Oct","Nov","Dec"};
        int month = -1, day;
        for (int i=0;i<12;i++) if (date.find(months[i]) != std::string::npos) { month=i; break; }
        if (month != -1 && sscanf(date.c_str(), "%*s %d", &day) == 1) {
            time_t now = time(nullptr);
            struct tm* current_time = localtime(&now);
            timeinfo.tm_year = current_time->tm_year; // current year
            timeinfo.tm_mon = month;
            timeinfo.tm_mday = day;
            result = mktime(&timeinfo);
        }
    }

    if (result == 0) result = time(nullptr); // fallback
    return result;
}

bool ADBDevice::SetDirectory(const std::string &path) {
    
    if (!_connected || !_adb_shell) {
        return false;
    }
    
    // Execute cd command and get new working directory (properly quote path for spaces)
    std::string cd_command = "cd " + ADBUtils::ShellQuote(path) + " 2>/dev/null && pwd";
    std::string result = RunShellCommand(cd_command);
    
    if (result.empty()) {
        return false;
    }
    
    // Extract the new path from pwd output
    std::string new_path = ExtractPathFromPwd(result);
    if (new_path.empty()) {
        return false;
    }
    
    // Update current path
    _current_path = new_path;
    return true;
}

int ADBDevice::TransferItem(const std::string& src, const std::string& dst, bool is_push, bool recursive,
                           const std::function<void(int)>& on_progress,
                           const std::function<bool()>& abort_check)
{
    EnsureConnection();
    if (int err = ADBUtils::CheckConnection(_connected)) return err;

    std::vector<std::string> args = {is_push ? "push" : "pull"};
    if (on_progress) args.push_back("-p");
    args.push_back(src);
    args.push_back(dst);

    std::string result;
    if (on_progress) {
        ProgressParser parser(on_progress);
        parser.start();
        result = RunAdbCommandWithProgress(args, std::ref(parser), abort_check);
        parser.drain();
        if (IsSuccessResult(result, is_push)) {
            parser.complete();
            return 0;
        }
    } else {
        result = RunAdbCommand(args);
        if (IsSuccessResult(result, is_push)) return 0;
    }

    return Str2Errno(result);
}

int ADBDevice::PullFile(const std::string &devicePath, const std::string &localPath) {
    return TransferItem(devicePath, localPath, false, false);
}

int ADBDevice::PullFile(const std::string &devicePath, const std::string &localPath, const std::function<void(int)> &on_progress, const std::function<bool()> &abort_check) {
    return TransferItem(devicePath, localPath, false, false, on_progress, abort_check);
}

int ADBDevice::PushFile(const std::string &localPath, const std::string &devicePath) {
    return TransferItem(localPath, devicePath, true, false);
}

int ADBDevice::PushFile(const std::string &localPath, const std::string &devicePath, const std::function<void(int)> &on_progress, const std::function<bool()> &abort_check) {
    return TransferItem(localPath, devicePath, true, false, on_progress, abort_check);
}

int ADBDevice::PullDirectory(const std::string &devicePath, const std::string &localPath) {
    return TransferItem(devicePath, localPath, false, true);
}

int ADBDevice::PullDirectory(const std::string &devicePath, const std::string &localPath, const std::function<void(int)> &on_progress, const std::function<bool()> &abort_check) {
    return TransferItem(devicePath, localPath, false, true, on_progress, abort_check);
}

int ADBDevice::PushDirectory(const std::string &localPath, const std::string &devicePath) {
    return TransferItem(localPath, devicePath, true, true);
}

int ADBDevice::PushDirectory(const std::string &localPath, const std::string &devicePath, const std::function<void(int)> &on_progress, const std::function<bool()> &abort_check) {
    return TransferItem(localPath, devicePath, true, true, on_progress, abort_check);
}



int ADBDevice::DeleteFile(const std::string &devicePath) {
    EnsureConnection();
    if (int err = ADBUtils::CheckConnection(_connected)) return err;

    std::string command = "rm -- " + ADBUtils::ShellQuote(devicePath);
    std::string result = RunShellCommand(command);

    return result.empty() ? 0 : Str2Errno(result);
}

int ADBDevice::DeleteDirectory(const std::string &devicePath) {
    EnsureConnection();
    if (int err = ADBUtils::CheckConnection(_connected)) return err;

    std::string command = "rm -rf -- " + ADBUtils::ShellQuote(devicePath);
    std::string result = RunShellCommand(command);

    return result.empty() ? 0 : Str2Errno(result);
}

int ADBDevice::CreateDirectory(const std::string &devicePath) {
    EnsureConnection();
    if (int err = ADBUtils::CheckConnection(_connected)) return err;

    std::string command = "mkdir -p -- " + ADBUtils::ShellQuote(devicePath);
    std::string result = RunShellCommand(command);

    return result.empty() ? 0 : Str2Errno(result);
}

int ADBDevice::CopyRemote(const std::string &srcDevicePath, const std::string &dstDeviceDir) {
    EnsureConnection();
    if (int err = ADBUtils::CheckConnection(_connected)) return err;

    // Try cp -a first (preserve attrs where possible), then fallback to cp -r for older toolboxes.
    std::string command =
        "cp -a -- " + ADBUtils::ShellQuote(srcDevicePath) + " " + ADBUtils::ShellQuote(dstDeviceDir) +
        " 2>/dev/null || cp -r -- " + ADBUtils::ShellQuote(srcDevicePath) + " " + ADBUtils::ShellQuote(dstDeviceDir);
    std::string result = RunShellCommand(command);
    return result.empty() ? 0 : Str2Errno(result);
}

int ADBDevice::MoveRemote(const std::string &srcDevicePath, const std::string &dstDeviceDir) {
    EnsureConnection();
    if (int err = ADBUtils::CheckConnection(_connected)) return err;

    std::string command = "mv -- " + ADBUtils::ShellQuote(srcDevicePath) + " " + ADBUtils::ShellQuote(dstDeviceDir);
    std::string result = RunShellCommand(command);
    return result.empty() ? 0 : Str2Errno(result);
}

bool ADBDevice::FileExists(const std::string &devicePath) {
    EnsureConnection();
    if (!_connected) return false;

    std::string command = "test -e " + ADBUtils::ShellQuote(devicePath) + " && echo 1 || echo 0";
    std::string result = RunShellCommand(command);

    ADBUtils::TrimTrailingNewlines(result);
    // Also trim spaces
    while (!result.empty() && result.back() == ' ') {
        result.pop_back();
    }

    return result == "1";
}

ADBDevice::DirectoryInfo ADBDevice::GetDirectoryInfo(const std::string &devicePath) {
    DirectoryInfo info = {0, 0};
    EnsureConnection();
    if (!_connected) {
        return info;
    }

    // Get file count and total size in one command
    // Output format: "count size" where size is in bytes
    // Use ls -l instead of stat -c for portability across Android toolboxes (BusyBox/toybox)
    std::string command = "find " + ADBUtils::ShellQuote(devicePath) +
        " -type f -exec ls -l {} \\; 2>/dev/null | awk '{c++;s+=$5}END{print c,s}'";
    std::string result = RunShellCommand(command);

    // Trim whitespace
    ADBUtils::TrimTrailingNewlines(result);
    while (!result.empty() && result.front() == ' ') result.erase(0, 1);
    while (!result.empty() && result.back() == ' ') result.pop_back();

    // Parse result: "count size"
    // Validate that we have two numbers separated by space
    size_t space = result.find(' ');
    if (space != std::string::npos && space > 0 && space < result.size() - 1) {
        std::string count_str = result.substr(0, space);
        std::string size_str = result.substr(space + 1);

        // Validate both strings contain only digits
        bool count_valid = !count_str.empty() && std::all_of(count_str.begin(), count_str.end(),
            [](unsigned char c) { return std::isdigit(c); });
        bool size_valid = !size_str.empty() && std::all_of(size_str.begin(), size_str.end(),
            [](unsigned char c) { return std::isdigit(c); });

        if (count_valid && size_valid) {
            info.file_count = strtoull(count_str.c_str(), nullptr, 10);
            info.total_size = strtoull(size_str.c_str(), nullptr, 10);
        }
    }

    return info;
}

int ADBDevice::Str2Errno(const std::string &adbError) {
    static const std::vector<std::pair<const char*, int>> errorMap = {
        {"remote object", ENOENT},
        {"does not exist", ENOENT},
        {"No such file or directory", ENOENT},
        {"File exists", EEXIST},
        {"Permission denied", EACCES},
        {"insufficient permissions for device", EACCES},
        {"No space left on device", ENOSPC},
        {"Read-only file system", EROFS},
        {"Broken pipe", EPIPE},
        {"error: closed", EPIPE},
        {"Operation not permitted", EPERM},
        {"Directory not empty", ENOTEMPTY},
        {"Device not found", ENODEV},
        {"no devices/emulators found", ENODEV},
        {"more than one device/emulator", EINVAL}
    };

    for (const auto& [key, code] : errorMap) {
        if (adbError.find(key) != std::string::npos) {
            return code;
        }
    }

    return EIO;
}
