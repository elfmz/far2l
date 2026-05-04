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

// --- ADBUtils ---
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

std::string PathBasename(const std::string& p)
{
    if (p.empty()) return std::string();
    size_t end = p.size();
    while (end > 0 && p[end - 1] == '/') --end;
    if (end == 0) return std::string();
    size_t start = end;
    while (start > 0 && p[start - 1] != '/') --start;
    return p.substr(start, end - start);
}

} // namespace ADBUtils


// --- ProgressParser ---
ProgressParser::ProgressParser(AdbProgressFn on_progress, bool debug_log)
    : _on_progress(std::move(on_progress)), _debug_log(debug_log), _last_percent(-1) {}

// ECMA-48 state machine — per-byte, state survives chunks; drops controls, keeps printable + \r\n\t.
void ProgressParser::VtFeed(const std::string &chunk) {
    for (char c : chunk) {
        unsigned char b = static_cast<unsigned char>(c);
        switch (_vt_state) {
            case VT_NORMAL:
                // C1 controls (0x80-0x9F) NOT decoded — adb output is UTF-8 where those are continuation bytes.
                if (b == 0x1B) { _vt_state = VT_ESC; }
                else if (b == '\r' || b == '\n' || b == '\t') { _pending.push_back(c); }
                else if (b >= 0x20 && b != 0x7F) { _pending.push_back(c); }
                break;
            case VT_ESC:
                if (b == '[')      _vt_state = VT_CSI;
                else if (b == ']') _vt_state = VT_OSC;
                else if (b == ' ' || b == '#' || b == '(' || b == ')'
                      || b == '*' || b == '+' || b == '%')
                    _vt_state = VT_ESC_INTERM;  // 2-byte intro: ESC <int> <final>
                else _vt_state = VT_NORMAL;     // single-char ESC <X>: discard X
                break;
            case VT_ESC_INTERM:
                _vt_state = VT_NORMAL;
                break;
            case VT_CSI:
                // Params 0x30-0x3F, intermediates 0x20-0x2F, terminator 0x40-0x7E.
                if (b >= 0x40 && b <= 0x7E) _vt_state = VT_NORMAL;
                // else: param/intermediate — discard, stay in CSI
                break;
            case VT_OSC:
                if (b == 0x07) _vt_state = VT_NORMAL;       // BEL terminator
                else if (b == 0x1B) _vt_state = VT_OSC_ESC; // possible ST ahead
                // else: OSC payload — discard
                break;
            case VT_OSC_ESC:
                if (b == '\\') _vt_state = VT_NORMAL;  // ST (string terminator)
                else _vt_state = VT_ESC;               // restart escape parse
                break;
        }
    }
}

void ProgressParser::operator()(const std::string &chunk) {
    if (_debug_log) {
        DBG(" PTY chunk received (%zu bytes): [", chunk.size());
        for (unsigned char c : chunk) {
            if (c >= 32 && c < 127) DBG("%c", c);
            else                    DBG("\\x%02X", c);
        }
        DBG("]\n");
    }

    VtFeed(chunk);
    size_t split = 0;
    while ((split = _pending.find_first_of("\r\n")) != std::string::npos) {
        std::string line = _pending.substr(0, split);
        _pending.erase(0, split + 1);
        int percent = -1;
        std::string path;
        if (!ExtractProgress(line, percent, path)) continue;
        // Fire on percent OR path change so file boundaries surface.
        if (percent != _last_percent || path != _last_path) {
            _last_percent = percent;
            _last_path = path;
            _on_progress(percent, path);
        }
    }
}

void ProgressParser::drain() {
    if (!_pending.empty()) {
        int percent = -1;
        std::string path;
        if (ExtractProgress(_pending, percent, path)
                && (percent != _last_percent || path != _last_path)) {
            _on_progress(percent, path);
        }
        _pending.clear();
    }
}

void ProgressParser::complete() { _on_progress(100, std::string()); }
void ProgressParser::start()    { _on_progress(0,   std::string()); }

bool ProgressParser::ExtractProgress(const std::string &s, int &percent, std::string &path) {
    // Input is VT-cleaned. Find the LAST '%' so an in-path '%' can't poison detection.
    size_t p = s.rfind('%');
    if (p == std::string::npos || p == 0) return false;
    size_t start = p;
    while (start > 0 && std::isdigit(static_cast<unsigned char>(s[start - 1]))) --start;
    if (start == p) return false;
    int v = std::atoi(s.substr(start, p - start).c_str());
    if (v < 0 || v > 100) return false;
    percent = v;
    path.clear();

    // Legacy adb -p: "[<spaces>NN%] /path" — variable padding ("[  5%]" / "[100%]").
    bool legacy = false;
    {
        size_t i = start;
        while (i > 0 && s[i - 1] == ' ') --i;
        if (i > 0 && s[i - 1] == '[') legacy = true;
    }
    if (legacy) {
        size_t close = s.find(']', p);
        if (close != std::string::npos) {
            size_t ps = close + 1;
            while (ps < s.size() && std::isspace(static_cast<unsigned char>(s[ps]))) ++ps;
            path.assign(s, ps, std::string::npos);
        }
    }
    // Modern adb -p format: "<path>: NN%[\x1B[K]" — ": " just before digits.
    else if (start >= 2 && s[start - 1] == ' ' && s[start - 2] == ':') {
        path.assign(s, 0, start - 2);
    }
    // Strip a leading "[<spaces>NN%]<spaces>" overall-batch prefix that adb prepends in the
    // combined "[ NN%] <path>: NN%" format. Without this, every emit for the same file
    // produces a different _cur_path (the [NN%] varies) → spurious path-changes → file count
    // races to unit_files clamp instantly while transfer is still running.
    if (!path.empty() && path[0] == '[') {
        size_t close = path.find(']');
        if (close != std::string::npos) {
            bool only_pct_inside = true;
            for (size_t i = 1; i < close; ++i) {
                char c = path[i];
                if (!(c == ' ' || std::isdigit(static_cast<unsigned char>(c)) || c == '%')) {
                    only_pct_inside = false; break;
                }
            }
            if (only_pct_inside) {
                size_t ps = close + 1;
                while (ps < path.size() && std::isspace(static_cast<unsigned char>(path[ps]))) ++ps;
                path.erase(0, ps);
            }
        }
    }
    // Path leftover: trailing whitespace from the source line.
    while (!path.empty()) {
        unsigned char c = static_cast<unsigned char>(path.back());
        if (std::isspace(c)) { path.pop_back(); continue; }
        break;
    }
    return true;
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
    // Trailer "(<N> bytes in <T>s)" — only emitted on success and never abbreviated by adb's
    // terminal-width path truncation (which can swallow "file pushed"/"skipped" tokens).
    if (result.find("bytes in ") != std::string::npos) return true;
    if (result.find("skipped") != std::string::npos) return true;
    if (is_push) {
        return result.find("file pushed") != std::string::npos ||
               result.find("files pushed") != std::string::npos;
    }
    return result.find("file pulled") != std::string::npos ||
           result.find("files pulled") != std::string::npos;
}

std::string ADBDevice::RunShellCommand(const std::string &command)
{
    EnsureConnection();
    return _adb_shell->shellCommand(command);
}

int ADBDevice::LastShellExitCode() const
{
    return _adb_shell ? _adb_shell->lastExitCode() : -1;
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
        std::string extracted = ExtractPathFromPwd(pwd_output);
        // Keep previous path if validation rejected pwd output (timeout / malformed marker) — don't blank a valid path.
        if (!extracted.empty()) {
            _current_path = extracted;
        }
    } catch (const std::exception& e) {
        // Ignore - keep current path
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
    // Reject non-absolute / contaminated output: a broken session returns error text or shell fragments, which poison _current_path and leak into cmd-line echo.
    size_t end = pwd_output.size();
    while (end > 0 && (pwd_output[end - 1] == '\n' || pwd_output[end - 1] == '\r')) --end;
    if (end == 0 || pwd_output[0] != '/') {
        return "";
    }
    for (size_t i = 0; i < end; ++i) {
        char c = pwd_output[i];
        if (c == '\n' || c == '\r' || c == ';' || c == '`' || c == '$') {
            return "";
        }
    }
    return pwd_output.substr(0, end);
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
                           const AdbProgressFn& on_progress,
                           const std::function<bool()>& abort_check)
{
    EnsureConnection();
    if (int err = ADBUtils::CheckConnection(_connected)) return err;

    std::vector<std::string> args = {is_push ? "push" : "pull"};
    if (on_progress) args.push_back("-p");
    // Pull `-a` preserves timestamp+mode (verified `adb help`); push has no equivalent flag.
    if (!is_push) args.push_back("-a");
    args.push_back(src);
    args.push_back(dst);

    std::string result;
    if (on_progress) {
        ProgressParser parser(on_progress);
        parser.start();
        result = RunAdbCommandWithProgress(args, std::ref(parser), abort_check);
        parser.drain();
        // Exit code is the truth source — adb's stdout trailer ("X file(s) pushed") gets
        // truncated by terminal-width path abbreviation, breaking text-based detection.
        if (ADBShell::lastPtyExit() == 0 || IsSuccessResult(result, is_push)) {
            parser.complete();
            return 0;
        }
    } else {
        result = RunAdbCommand(args);
        if (IsSuccessResult(result, is_push)) return 0;
    }

    int errno_mapped = Str2Errno(result);
    const size_t tail_len = std::min<size_t>(result.size(), 400);
    const char* tail = result.c_str() + (result.size() - tail_len);
    DBG("TransferItem FAIL is_push=%d pty_exit=%d errno=%d src='%s' dst='%s' tail[%zu]='%.*s'\n",
        is_push, ADBShell::lastPtyExit(), errno_mapped, src.c_str(), dst.c_str(),
        tail_len, (int)tail_len, tail);
    return errno_mapped;
}

int ADBDevice::PullFile(const std::string &devicePath, const std::string &localPath) {
    return TransferItem(devicePath, localPath, false, false);
}

int ADBDevice::PullFile(const std::string &devicePath, const std::string &localPath, const AdbProgressFn &on_progress, const std::function<bool()> &abort_check) {
    return TransferItem(devicePath, localPath, false, false, on_progress, abort_check);
}

int ADBDevice::PushFile(const std::string &localPath, const std::string &devicePath) {
    return TransferItem(localPath, devicePath, true, false);
}

int ADBDevice::PushFile(const std::string &localPath, const std::string &devicePath, const AdbProgressFn &on_progress, const std::function<bool()> &abort_check) {
    return TransferItem(localPath, devicePath, true, false, on_progress, abort_check);
}

int ADBDevice::PullDirectory(const std::string &devicePath, const std::string &localPath) {
    return TransferItem(devicePath, localPath, false, true);
}

int ADBDevice::PullDirectory(const std::string &devicePath, const std::string &localPath, const AdbProgressFn &on_progress, const std::function<bool()> &abort_check) {
    return TransferItem(devicePath, localPath, false, true, on_progress, abort_check);
}

int ADBDevice::PushDirectory(const std::string &localPath, const std::string &devicePath) {
    return TransferItem(localPath, devicePath, true, true);
}

int ADBDevice::PushDirectory(const std::string &localPath, const std::string &devicePath, const AdbProgressFn &on_progress, const std::function<bool()> &abort_check) {
    return TransferItem(localPath, devicePath, true, true, on_progress, abort_check);
}



// Exit-code-first errno mapping: `result.empty()` alone confused warnings-on-success with real errors; the marker-protocol exit code is authoritative.
static int MutationResultToErrno(int exitCode, const std::string &result) {
    if (exitCode == 0) return 0;
    if (result.empty()) return EIO;
    return ADBDevice::Str2Errno(result);
}

int ADBDevice::DeleteFile(const std::string &devicePath) {
    EnsureConnection();
    if (int err = ADBUtils::CheckConnection(_connected)) return err;

    std::string command = "rm -- " + ADBUtils::ShellQuote(devicePath);
    std::string result = RunShellCommand(command);
    return MutationResultToErrno(LastShellExitCode(), result);
}

int ADBDevice::DeleteDirectory(const std::string &devicePath) {
    EnsureConnection();
    if (int err = ADBUtils::CheckConnection(_connected)) return err;

    std::string command = "rm -rf -- " + ADBUtils::ShellQuote(devicePath);
    std::string result = RunShellCommand(command);
    return MutationResultToErrno(LastShellExitCode(), result);
}

int ADBDevice::CreateDirectory(const std::string &devicePath) {
    EnsureConnection();
    if (int err = ADBUtils::CheckConnection(_connected)) return err;

    std::string command = "mkdir -p -- " + ADBUtils::ShellQuote(devicePath);
    std::string result = RunShellCommand(command);
    return MutationResultToErrno(LastShellExitCode(), result);
}

int ADBDevice::CopyRemote(const std::string &srcDevicePath, const std::string &dstDeviceDir) {
    EnsureConnection();
    if (int err = ADBUtils::CheckConnection(_connected)) return err;

    // cp -a (mode + symlinks) → fallback cp -Rp (mode-preserving, no -a needed on old toolbox).
    std::string command =
        "cp -a -- " + ADBUtils::ShellQuote(srcDevicePath) + " " + ADBUtils::ShellQuote(dstDeviceDir) +
        " 2>/dev/null || cp -Rp -- " + ADBUtils::ShellQuote(srcDevicePath) + " " + ADBUtils::ShellQuote(dstDeviceDir);
    std::string result = RunShellCommand(command);
    return MutationResultToErrno(LastShellExitCode(), result);
}

int ADBDevice::MoveRemote(const std::string &srcDevicePath, const std::string &dstDeviceDir) {
    EnsureConnection();
    if (int err = ADBUtils::CheckConnection(_connected)) return err;

    std::string command = "mv -- " + ADBUtils::ShellQuote(srcDevicePath) + " " + ADBUtils::ShellQuote(dstDeviceDir);
    std::string result = RunShellCommand(command);
    return MutationResultToErrno(LastShellExitCode(), result);
}

int ADBDevice::CopyRemoteAs(const std::string &srcDevicePath, const std::string &dstDevicePath) {
    EnsureConnection();
    if (int err = ADBUtils::CheckConnection(_connected)) return err;

    // cp -a → cp -r fallback; takes full dst path (basename may change). Caller pre-handles overwrite.
    std::string command =
        "cp -a -- " + ADBUtils::ShellQuote(srcDevicePath) + " " + ADBUtils::ShellQuote(dstDevicePath) +
        " 2>/dev/null || cp -Rp -- " + ADBUtils::ShellQuote(srcDevicePath) + " " + ADBUtils::ShellQuote(dstDevicePath);
    std::string result = RunShellCommand(command);
    return MutationResultToErrno(LastShellExitCode(), result);
}

int ADBDevice::MoveRemoteAs(const std::string &srcDevicePath, const std::string &dstDevicePath) {
    EnsureConnection();
    if (int err = ADBUtils::CheckConnection(_connected)) return err;

    std::string command = "mv -- " + ADBUtils::ShellQuote(srcDevicePath) + " " + ADBUtils::ShellQuote(dstDevicePath);
    std::string result = RunShellCommand(command);
    return MutationResultToErrno(LastShellExitCode(), result);
}

int ADBDevice::CopyRemoteAs(const std::string &srcDevicePath, const std::string &dstDevicePath,
                            const std::function<bool()>& abort_check) {
    if (!abort_check) return CopyRemoteAs(srcDevicePath, dstDevicePath);
    if (int err = ADBUtils::CheckConnection(_connected)) return err;
    // One-shot `adb shell -c` so the spawned process can be killed on abort.
    std::string sh =
        "cp -a -- " + ADBUtils::ShellQuote(srcDevicePath) + " " + ADBUtils::ShellQuote(dstDevicePath) +
        " 2>/dev/null || cp -Rp -- " + ADBUtils::ShellQuote(srcDevicePath) + " " + ADBUtils::ShellQuote(dstDevicePath);
    RunAdbCommandWithProgress({"shell", sh}, [](const std::string&){}, abort_check);
    if (abort_check()) return ECANCELED;
    return 0;
}

int ADBDevice::MoveRemoteAs(const std::string &srcDevicePath, const std::string &dstDevicePath,
                            const std::function<bool()>& abort_check) {
    if (!abort_check) return MoveRemoteAs(srcDevicePath, dstDevicePath);
    if (int err = ADBUtils::CheckConnection(_connected)) return err;
    std::string sh = "mv -- " + ADBUtils::ShellQuote(srcDevicePath) + " " + ADBUtils::ShellQuote(dstDevicePath);
    RunAdbCommandWithProgress({"shell", sh}, [](const std::string&){}, abort_check);
    if (abort_check()) return ECANCELED;
    return 0;
}

int ADBDevice::CopyRemote(const std::string &srcDevicePath, const std::string &dstDeviceDir,
                          const std::function<bool()>& abort_check) {
    if (!abort_check) return CopyRemote(srcDevicePath, dstDeviceDir);
    if (int err = ADBUtils::CheckConnection(_connected)) return err;
    std::string sh =
        "cp -a -- " + ADBUtils::ShellQuote(srcDevicePath) + " " + ADBUtils::ShellQuote(dstDeviceDir) +
        " 2>/dev/null || cp -Rp -- " + ADBUtils::ShellQuote(srcDevicePath) + " " + ADBUtils::ShellQuote(dstDeviceDir);
    RunAdbCommandWithProgress({"shell", sh}, [](const std::string&){}, abort_check);
    if (abort_check()) return ECANCELED;
    return 0;
}

int ADBDevice::MoveRemote(const std::string &srcDevicePath, const std::string &dstDeviceDir,
                          const std::function<bool()>& abort_check) {
    if (!abort_check) return MoveRemote(srcDevicePath, dstDeviceDir);
    if (int err = ADBUtils::CheckConnection(_connected)) return err;
    std::string sh = "mv -- " + ADBUtils::ShellQuote(srcDevicePath) + " " + ADBUtils::ShellQuote(dstDeviceDir);
    RunAdbCommandWithProgress({"shell", sh}, [](const std::string&){}, abort_check);
    if (abort_check()) return ECANCELED;
    return 0;
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

bool ADBDevice::IsDirectory(const std::string &devicePath) {
    EnsureConnection();
    if (!_connected) return false;
    std::string command = "test -d " + ADBUtils::ShellQuote(devicePath) + " && echo 1 || echo 0";
    std::string result = RunShellCommand(command);
    ADBUtils::TrimTrailingNewlines(result);
    while (!result.empty() && result.back() == ' ') result.pop_back();
    return result == "1";
}

void ADBDevice::ListDirNames(const std::string &devicePath, std::unordered_set<std::string>& out) {
    EnsureConnection();
    if (!_connected) return;
    // -A includes dotfiles, -1 one-per-line; ignore stderr (missing dir → empty set).
    std::string command = "ls -A1 -- " + ADBUtils::ShellQuote(devicePath) + " 2>/dev/null";
    std::string result = RunShellCommand(command);
    size_t pos = 0;
    while (pos < result.size()) {
        size_t eol = result.find('\n', pos);
        std::string line = result.substr(pos, eol == std::string::npos ? std::string::npos : eol - pos);
        pos = (eol == std::string::npos) ? result.size() : eol + 1;
        while (!line.empty() && line.back() == '\r') line.pop_back();
        if (!line.empty()) out.insert(std::move(line));
    }
}

void ADBDevice::BatchDirectoryFileSizes(const std::vector<std::string>& devicePaths,
                                         std::map<std::string, std::unordered_map<std::string, uint64_t>>& out) {
    EnsureConnection();
    if (!_connected || devicePaths.empty()) return;

    // Pre-create entries so empty top-level dirs (no files via -type f) still produce an entry.
    for (const auto& p : devicePaths) out[p];

    // One `find` over all top-level dirs → one shell roundtrip (~200ms persistent-shell overhead
    // dominates; per-dir cost amortizes to near-zero). %p (full path) lets us prefix-match each
    // emitted file back to its top-level dir, %P would be ambiguous across multiple roots.
    std::string command = "find";
    for (const auto& p : devicePaths) command += " " + ADBUtils::ShellQuote(p);
    command += " -type f -printf '%s\\t%p\\n' 2>/dev/null";
    std::string result = RunShellCommand(command);

    // Longest-prefix first — handles the case where one input dir is nested inside another.
    std::vector<std::string> sorted = devicePaths;
    std::sort(sorted.begin(), sorted.end(),
              [](const std::string& a, const std::string& b) { return a.size() > b.size(); });

    size_t pos = 0;
    while (pos < result.size()) {
        size_t eol = result.find('\n', pos);
        std::string line = result.substr(pos, eol == std::string::npos ? std::string::npos : eol - pos);
        pos = (eol == std::string::npos) ? result.size() : eol + 1;
        if (line.empty()) continue;
        size_t tab = line.find('\t');
        if (tab == std::string::npos) continue;
        const std::string size_str = line.substr(0, tab);
        std::string full_path = line.substr(tab + 1);
        while (!full_path.empty() && full_path.back() == '\r') full_path.pop_back();
        if (size_str.empty() || full_path.empty()) continue;

        for (const auto& dir : sorted) {
            if (full_path.size() > dir.size() + 1
                && full_path.compare(0, dir.size(), dir) == 0
                && full_path[dir.size()] == '/') {
                out[dir][full_path.substr(dir.size() + 1)] =
                    strtoull(size_str.c_str(), nullptr, 10);
                break;
            }
        }
    }
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
