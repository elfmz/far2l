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

// "drwxr-xr-x" -> S_IFDIR|0755. Panels need the real bits: far2l derives the permission
// column, the executable/symlink highlighting and the attribute flags from dwUnixMode, so a
// hardcoded 0755/0644 makes all three wrong.
// NB: a fixed variant of NetRocks' ShellParseUtils::{Char2FileType,Triplet2FileMode,Str2Mode},
// which is private to that plugin, omits the setuid/setgid/sticky bits and tests the wrong
// character for 's'. Promoting a shared implementation to utils/ would suit both.
mode_t LsPermsToMode(const std::string &perms)
{
    static const auto triplet = [](const char *c) -> mode_t {
        mode_t out = 0;
        if (c[0] == 'r') out |= 4;
        if (c[1] == 'w') out |= 2;
        // x, or s/t which mean "set-id/sticky *and* executable"; S/T mean the bit without x.
        if (c[2] == 'x' || c[2] == 's' || c[2] == 't') out |= 1;
        return out;
    };

    mode_t mode = 0;
    switch (perms.empty() ? '-' : perms[0]) {
        case 'd': mode = S_IFDIR; break;
        case 'l': mode = S_IFLNK; break;
        case 'c': mode = S_IFCHR; break;
        case 'b': mode = S_IFBLK; break;
        case 'p': mode = S_IFIFO; break;
        case 's': mode = S_IFSOCK; break;
        default:  mode = S_IFREG; break;
    }
    // Entries the shell could not stat come back as "d?????????" - keep the type, no bits.
    if (perms.size() >= 4)  mode |= triplet(perms.c_str() + 1) << 6;
    if (perms.size() >= 7)  mode |= triplet(perms.c_str() + 4) << 3;
    if (perms.size() >= 10) mode |= triplet(perms.c_str() + 7);

    // The set-id and sticky bits live in the x positions as s/S (user, group) and t/T (other);
    // far2l renders them in the permission column, so losing them shows a setuid binary as a
    // plain -rwxr-xr-x.
    if (perms.size() >= 4  && (perms[3] == 's' || perms[3] == 'S')) mode |= S_ISUID;
    if (perms.size() >= 7  && (perms[6] == 's' || perms[6] == 'S')) mode |= S_ISGID;
    if (perms.size() >= 10 && (perms[9] == 't' || perms[9] == 'T')) mode |= S_ISVTX;
    return mode;
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
    // Strip leading "[NN%]" batch prefix from adb's combined format — varying prefix would otherwise spam path-changes and inflate file count.
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

std::string ADBDevice::RunAdbCommandWithProgress(const std::vector<std::string> &args, const std::function<void(const std::string&)> &on_chunk, const std::function<bool()> &abort_check, int idle_timeout_ms) {
    return ADBShell::adbExecWithProgress(BuildArgs(args), on_chunk, abort_check, idle_timeout_ms);
}

bool ADBDevice::IsSuccessResult(const std::string& result, bool is_push) const
{
    // Trailer "(<N> bytes in <T>s)" — only on success, never truncated by adb's path abbreviation.
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

bool ADBDevice::HaveFindPrintf()
{
    if (_find_printf < 0) {
        // Probe on a path that always exists and needs no directory traversal. -printf is a
        // GNU extension that toybox implements only in newer builds; where it is missing the
        // whole option is rejected, hence testing the option rather than the toybox version.
        // Probe every specifier the real command relies on, not just -printf: a build that
        // supports the option but not, say, %l would otherwise return silently empty records.
        std::string out = RunShellCommand(
            "find / -maxdepth 0 -printf '%s\\t%T@\\t%u\\t%g\\t%l\\t%M\\n' >/dev/null 2>&1"
            " && echo Y || echo N");
        // Only cache a definite answer. RunShellCommand yields "" on a shell timeout or restart,
        // and caching that as "no" would drop the session onto the slow path permanently.
        if (out.find('Y') != std::string::npos) {
            _find_printf = 1;
        } else if (out.find('N') != std::string::npos) {
            _find_printf = 0;
        }
        DBG("HaveFindPrintf: %d (probe returned '%s')\n", _find_printf, out.c_str());
        if (_find_printf < 0) {
            return false;   // undecided: use the portable path this time, retry the probe later
        }
    }
    return _find_printf == 1;
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

    // One roundtrip. `ls -la` stays the authoritative enumerator and cannot be replaced: it is
    // the only listing that never drops an entry. SELinux denies getattr on some paths to the
    // shell user - 97 of the 401 entries under /system/bin on a stock Android 14 - and stat,
    // find and `adb ls` all skip those silently, while ls still prints the name and recovers
    // the type from d_type.
    //
    // Where `find -printf` exists it is layered on top, because it supplies what parsing
    // human-formatted ls cannot:
    //   * records are NUL-framed, so names and symlink targets survive spaces, quotes, " -> "
    //     and even an embedded newline;
    //   * %T@ is epoch seconds, so there is no date format and no timezone to get wrong. ls
    //     prints the device's local wall time, which the host would otherwise convert with its
    //     own mktime() - two hours out when the device is +0400 and the host +0200;
    //   * %l gives the symlink target and `find -L`'s %M gives the *target's* mode, which is how
    //     a symlink to a directory becomes enterable. The legacy branch instead forks
    //     [ -L ]/[ -d ]/[ -f ] per entry: measured 2.03 s versus 0.22 s on /system/bin, which
    //     holds 188 symlinks.
    const bool fast = HaveFindPrintf();

    const std::string separator = "<<<!>>>";
    const std::string arrow = ":->";

    std::ostringstream bulk_cmd;
    // `if cd` rather than `cd; ...`: on a chdir failure this yields no output at all, instead of
    // silently listing whichever directory we were in before under the new directory's name.
    bulk_cmd << "if cd " << ADBUtils::ShellQuote(path) << " 2>/dev/null; then pwd; ls -la; ";
    if (fast) {
        // A single NUL ends the ls section. It has to be a NUL and nothing else: ls output can
        // never contain one, whereas it CAN contain any other byte verbatim - toybox 0.8.9 prints
        // control characters raw, so a file named "bell\001" makes ls emit "\001\n" and a marker
        // built from printable/control bytes would be forged by the listing it is meant to
        // delimit, truncating the panel.
        //
        // After that boundary everything is NUL-framed "field\0name\0" pairs, tagged by the first
        // byte of the field so both find passes can share one stream and no second marker is
        // needed: 'B' carries the metadata, 'D' the dereferenced mode. %l and %M sit at the end
        // of their field, so a target containing a tab cannot shift anything.
        bulk_cmd << "printf '\\000'; "
                 << "find . -mindepth 1 -maxdepth 1 -printf 'B%s\\t%T@\\t%u\\t%g\\t%l\\0%f\\0' 2>/dev/null; "
                 << "find -L . -mindepth 1 -maxdepth 1 -printf 'D%M\\0%f\\0' 2>/dev/null; ";
    } else {
        bulk_cmd << "echo \"" << separator << "\"; "
                 << "for f in *; do "
                 << "[ -L \"$f\" ] && ([ -d \"$f\" ] && echo \"$f" << arrow << "D\" "
                 << "|| ([ -f \"$f\" ] && echo \"$f" << arrow << "F\" || echo \"$f" << arrow << "B\")); "
                 << "done; ";
    }
    bulk_cmd << "fi";

    const std::string bulk_output = RunShellCommand(bulk_cmd.str());

    // Split off the ls text before any line splitting: what follows is NUL-framed binary and
    // must never go through getline(). If the boundary is absent - find missing, killed, or the
    // whole command failed - fall back to ls-only rather than feeding binary to the line parser.
    std::string sec_names = bulk_output, sec_records;
    bool have_records = false;
    if (fast) {
        const size_t nul = bulk_output.find('\0');
        if (nul != std::string::npos) {
            sec_names = bulk_output.substr(0, nul);
            sec_records = bulk_output.substr(nul + 1);
            have_records = true;
        } else {
            DBG("DirectoryEnum: no record boundary in output, using ls only\n");
        }
    }

    std::vector<std::string> ls_lines;
    std::vector<std::string> symlink_info;
    std::string current_path;
    bool after_separator = false;

    // Split lines once
    std::istringstream output_stream(sec_names);
    std::string line;
    while (std::getline(output_stream, line)) {
        if (line.empty()) continue;
        if (line == separator) { after_separator = true; continue; }

        if (!after_separator) {
            if (current_path.empty()) {
                current_path = ExtractPathFromPwd(line);
                // Only publish a path that actually parsed: assigning the empty rejection value
                // would leave GetCurrentDevicePath() returning "", and every path joined from it
                // relative - which delete and copy then act on.
                if (!current_path.empty()) {
                    _current_path = current_path;
                }
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

        // Tokenize remembering each token's offset: the name is taken verbatim from the
        // original line afterwards, so filenames containing runs of spaces survive.
        std::vector<std::pair<size_t, std::string>> tok;
        for (size_t i = 0; i < ls_line.size(); ) {
            while (i < ls_line.size() && ls_line[i] == ' ') ++i;
            if (i >= ls_line.size()) break;
            const size_t b = i;
            while (i < ls_line.size() && ls_line[i] != ' ') ++i;
            tok.emplace_back(b, ls_line.substr(b, i - b));
        }
        if (tok.size() < 3)
            continue;

        // Field count is not fixed at 7. Two shapes occur on real devices:
        //   char/block nodes split the size column in two - "crw-rw---- 1 system camera 490,   0 <date> <time> CAM"
        //   un-stattable entries have every field replaced by '?' and no date/time at all -
        //     "l?????????   ? ?      ?      ?      ? cache -> ?"
        // A blind 7-token split shifts the fields and yields filenames like "18:56 CAM" or "-> ?".
        const std::string &perms = tok[0].second;
        std::string links = "1", owner = "?", group = "?", size = "0", date, time_str;
        size_t f = 1;
        if (perms.find('?') != std::string::npos) {
            // ls emits exactly five placeholders (nlink, owner, group, size, and a single '?'
            // where the date and time would be), so step over them by count. Scanning for the
            // first non-'?' token instead would misfire on a file actually named "?".
            if (tok.size() < 7)
                continue;
            f = 6;
        } else {
            if (tok.size() < 7)
                continue;
            links = tok[1].second; owner = tok[2].second; group = tok[3].second;
            size = tok[4].second;
            f = 5;
            if (!size.empty() && size.back() == ',') { size = "0"; ++f; } // skip device minor

            // The number of date tokens is device-dependent: toybox (Android 6+) prints ISO
            // "2026-08-06 18:57" (2 tokens), while busybox/toolbox - and any device with a
            // BusyBox ahead of toybox in PATH - prints "Aug  6 18:57" or "Aug  6 2024" (3).
            // So don't count tokens: the last metadata field is always either HH:MM or a bare
            // 4-digit year, so scan forward to that and treat everything before it as the date.
            size_t stamp = std::string::npos;
            for (size_t i = f; i < tok.size(); ++i) {
                const std::string &t = tok[i].second;
                if (t.find(':') != std::string::npos
                        || (t.size() == 4 && t.find_first_not_of("0123456789") == std::string::npos)) {
                    stamp = i;
                    break;
                }
            }
            if (stamp == std::string::npos || stamp + 1 >= tok.size())
                continue;
            for (size_t i = f; i < stamp; ++i) {
                if (!date.empty()) date += ' ';
                date += tok[i].second;
            }
            time_str = tok[stamp].second;
            f = stamp + 1;
        }
        if (f >= tok.size())
            continue;

        // Anchor one byte past the previous field rather than at the first non-space byte: ls
        // puts exactly one space before the name, so this keeps leading spaces that are part of
        // the filename (a file called " leading" must not become "leading", or every subsequent
        // path built from it points at something that does not exist).
        const size_t prev_end = tok[f - 1].first + tok[f - 1].second.size();
        if (prev_end + 1 > ls_line.size())
            continue;
        const std::string rest = ls_line.substr(prev_end + 1);

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
        item.FindData.dwUnixMode = ADBUtils::LsPermsToMode(perms);
        item.FindData.dwFileAttributes = WINPORT(EvaluateAttributesA)(item.FindData.dwUnixMode, filename.c_str());
        if (perms[0] == 'd') item.FindData.dwFileAttributes |= FILE_ATTRIBUTE_DIRECTORY;

        if (is_symlink) {
            item.Description = AllocateItemString(symlink_target.empty() ? "Symlink (no target)" : symlink_target);
        }

        try { item.FindData.nFileSize = item.FindData.nPhysicalSize = std::stoull(size); } catch (...) { item.FindData.nFileSize = item.FindData.nPhysicalSize = 0; }

        item.Owner = AllocateItemString(owner);
        item.Group = AllocateItemString(group);
        
        try { item.NumberOfLinks = std::stoi(links); } catch (...) { item.NumberOfLinks = 1; }

        // Rows for entries the shell may not stat carry no date at all. Leave their timestamps
        // zeroed rather than substituting "now", which made every SELinux-denied entry sort and
        // display as the newest file in the directory.
        if (!date.empty() && !time_str.empty()) {
            FILETIME ft{};
            time_t t = ParseLsDateTime(date, time_str);
            if (t) {
                ULARGE_INTEGER uli; uli.QuadPart = (t * 10000000ULL) + 116444736000000000ULL;
                ft.dwLowDateTime = uli.LowPart; ft.dwHighDateTime = uli.HighPart;
                item.FindData.ftCreationTime = item.FindData.ftLastAccessTime = item.FindData.ftLastWriteTime = ft;
            }
        }

        files.push_back(item);
    }

    // Index by name once; both the fast overlay and the legacy symlink pass need it.
    std::unordered_map<std::string, PluginPanelItem*> file_map;
    for (auto& f : files) {
        if (f.FindData.lpwszFileName) {
            file_map[StrWide2MB(f.FindData.lpwszFileName)] = &f;
        }
    }

    if (have_records) {
        // One tagged stream: "B<size>\t<mtime@>\t<owner>\t<group>\t<target>" NUL "<name>" NUL for
        // the metadata pass, "D<mode>" NUL "<name>" NUL for the dereferenced one. Entries absent
        // from the metadata pass are the ones the shell may not stat; they keep the name and type
        // ls gave them and are left without size or timestamp.
        for (size_t i = 0; i < sec_records.size(); ) {
            const size_t e1 = sec_records.find('\0', i);
            if (e1 == std::string::npos) break;
            const size_t e2 = sec_records.find('\0', e1 + 1);
            if (e2 == std::string::npos) break;
            const std::string field = sec_records.substr(i, e1 - i);
            const std::string name = sec_records.substr(e1 + 1, e2 - e1 - 1);
            i = e2 + 1;

            if (field.empty()) continue;
            const char tag = field[0];
            const std::string body = field.substr(1);

            auto it = file_map.find(name);
            if (it == file_map.end()) continue;
            PluginPanelItem *item = it->second;

            if (tag == 'D') {
                // `find -L` followed the link, so this mode describes the *target*: a leading 'd'
                // means "symlink pointing at a directory", which is what makes it enterable.
                // Broken links are reported as the link itself and correctly stay non-directories.
                if (!body.empty() && body[0] == 'd') {
                    item->FindData.dwFileAttributes |= FILE_ATTRIBUTE_DIRECTORY;
                }
                continue;
            }
            if (tag != 'B') continue;

            // Four tabs delimit five fields; whatever follows the fourth is %l verbatim.
            std::string fld[4];
            size_t fp = 0;
            int n = 0;
            for (; n < 4; ++n) {
                const size_t tp = body.find('\t', fp);
                if (tp == std::string::npos) break;
                fld[n] = body.substr(fp, tp - fp);
                fp = tp + 1;
            }
            if (n < 4) continue;
            const std::string target = body.substr(fp);

            try {
                item->FindData.nFileSize = item->FindData.nPhysicalSize = std::stoull(fld[0]);
            } catch (...) {
            }

            // %T@ is "<epoch>.<nanoseconds>". Apply it even when it is 0 - some pseudo-filesystem
            // roots really do report epoch 0, and keeping ls's value for them would reintroduce
            // the timezone error this pass exists to remove.
            bool mtime_ok = false;
            time_t mt = 0;
            try {
                mt = (time_t)std::stoll(fld[1].substr(0, fld[1].find('.')));
                mtime_ok = true;
            } catch (...) {
                mtime_ok = false;
            }
            if (mtime_ok) {
                FILETIME ft{};
                ULARGE_INTEGER uli;
                uli.QuadPart = ((unsigned long long)mt * 10000000ULL) + 116444736000000000ULL;
                ft.dwLowDateTime = uli.LowPart;
                ft.dwHighDateTime = uli.HighPart;
                item->FindData.ftCreationTime = item->FindData.ftLastAccessTime =
                    item->FindData.ftLastWriteTime = ft;
            }

            // Allocate first, then release the string the ls pass made: AllocateItemString may
            // throw, and freeing up front would leave a dangling pointer for the unwind path.
            if (!fld[2].empty()) {
                wchar_t *owner = AllocateItemString(fld[2]);
                free((void*)item->Owner);
                item->Owner = owner;
            }
            if (!fld[3].empty()) {
                wchar_t *group = AllocateItemString(fld[3]);
                free((void*)item->Group);
                item->Group = group;
            }
            if (!target.empty()) {
                wchar_t *descr = AllocateItemString(target);
                free((void*)item->Description);
                item->Description = descr;
            }
        }
    } else {
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
    int hour = 0, minute = 0, year_from_time = 0;

    // For entries older than ~6 months "ls -l" prints a 4-digit year where HH:MM normally
    // goes. Feeding that to "%d:%d" would set tm_hour=2024 and mktime() would then slide the
    // date weeks forward, so detect it and use it as the year at midnight instead.
    if (time_str.find(':') == std::string::npos) {
        year_from_time = atoi(time_str.c_str());
    } else {
        sscanf(time_str.c_str(), "%d:%d", &hour, &minute);
    }
    timeinfo.tm_hour = hour;
    timeinfo.tm_min = minute;
    timeinfo.tm_sec = 0;
    timeinfo.tm_isdst = -1; // let mktime work out DST rather than assuming standard time

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
            timeinfo.tm_year = year_from_time ? (year_from_time - 1900)
                                             : current_time->tm_year; // "MMM DD HH:MM" omits it
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

// Verdict for one `adb push`/`pull`. pty_exit is the child's exit code, or -1 when it died on a
// signal / was killed by the idle bound, in which case there is no status to trust and the
// trailer is the only evidence left.
bool ADBDevice::TransferSucceeded(int pty_exit, const std::string& result, bool is_push) const
{
    if (pty_exit == 0) return true;
    if (pty_exit > 0) return false;
    if (pty_exit == ADBShell::kPtyExitKilled) return false;   // we killed it: definitely not done
    return IsSuccessResult(result, is_push);
}

int ADBDevice::TransferItem(const std::string& src, const std::string& dst, bool is_push, bool recursive,
                           const AdbProgressFn& on_progress,
                           const std::function<bool()>& abort_check)
{
    EnsureConnection();
    if (int err = ADBUtils::CheckConnection(_connected)) return err;

    std::vector<std::string> args = {is_push ? "push" : "pull"};
    // Always -p: besides driving the progress dialog, the continuous emits are what let the
    // runner tell "slow transfer" from "device fell off the bus" (see kTransferIdleTimeoutMs).
    args.push_back("-p");
    // Pull `-a` preserves timestamp+mode (verified `adb help`); push has no equivalent flag.
    if (!is_push) args.push_back("-a");
    args.push_back(src);
    args.push_back(dst);

    std::string result;
    if (on_progress) {
        ProgressParser parser(on_progress);
        parser.start();
        result = RunAdbCommandWithProgress(args, std::ref(parser), abort_check,
                                          ADBShell::kTransferIdleTimeoutMs);
        parser.drain();
        // Both branches now run under a pty, so the child's exit status is always available and
        // is authoritative. Do NOT fall back to sniffing the trailer when it says failure: adb
        // prints "1 file pushed, 0 skipped. ... (N bytes in Ts)" even for a push that failed
        // (e.g. onto a read-only mount, where it also prints "remote couldn't create file"), so
        // an OR here reports a completed copy that wrote nothing. The sniff stays only for the
        // case where no status could be collected at all.
        if (TransferSucceeded(ADBShell::lastPtyExit(), result, is_push)) {
            parser.complete();
            return 0;
        }
    } else {
        // No progress dialog (e.g. F3 view pulls the file to a temp dir), but still go through
        // the pty runner: a plain pipe gives adb no tty, so -p prints nothing and a dead device
        // would block this call - on far2l's main thread - forever.
        result = RunAdbCommandWithProgress(args, [](const std::string&){}, {},
                                           ADBShell::kTransferIdleTimeoutMs);
        if (TransferSucceeded(ADBShell::lastPtyExit(), result, is_push)) return 0;
    }

    int errno_mapped = Str2Errno(result);
#if defined(DEBUG) || defined(_DEBUG)
    const size_t tail_len = std::min<size_t>(result.size(), 400);
    const char* tail = result.c_str() + (result.size() - tail_len);
    DBG("TransferItem FAIL is_push=%d pty_exit=%d errno=%d src='%s' dst='%s' tail[%zu]='%.*s'\n",
        is_push, ADBShell::lastPtyExit(), errno_mapped, src.c_str(), dst.c_str(),
        tail_len, (int)tail_len, tail);
#endif
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
    const std::string out = RunAdbCommandWithProgress({"shell", sh}, [](const std::string&){}, abort_check);
    if (abort_check()) return ECANCELED;
    return MutationResultToErrno(ADBShell::lastPtyExit(), out);
}

int ADBDevice::MoveRemoteAs(const std::string &srcDevicePath, const std::string &dstDevicePath,
                            const std::function<bool()>& abort_check) {
    if (!abort_check) return MoveRemoteAs(srcDevicePath, dstDevicePath);
    if (int err = ADBUtils::CheckConnection(_connected)) return err;
    std::string sh = "mv -- " + ADBUtils::ShellQuote(srcDevicePath) + " " + ADBUtils::ShellQuote(dstDevicePath);
    const std::string out = RunAdbCommandWithProgress({"shell", sh}, [](const std::string&){}, abort_check);
    if (abort_check()) return ECANCELED;
    return MutationResultToErrno(ADBShell::lastPtyExit(), out);
}

int ADBDevice::CopyRemote(const std::string &srcDevicePath, const std::string &dstDeviceDir,
                          const std::function<bool()>& abort_check) {
    if (!abort_check) return CopyRemote(srcDevicePath, dstDeviceDir);
    if (int err = ADBUtils::CheckConnection(_connected)) return err;
    std::string sh =
        "cp -a -- " + ADBUtils::ShellQuote(srcDevicePath) + " " + ADBUtils::ShellQuote(dstDeviceDir) +
        " 2>/dev/null || cp -Rp -- " + ADBUtils::ShellQuote(srcDevicePath) + " " + ADBUtils::ShellQuote(dstDeviceDir);
    const std::string out = RunAdbCommandWithProgress({"shell", sh}, [](const std::string&){}, abort_check);
    if (abort_check()) return ECANCELED;
    return MutationResultToErrno(ADBShell::lastPtyExit(), out);
}

int ADBDevice::MoveRemote(const std::string &srcDevicePath, const std::string &dstDeviceDir,
                          const std::function<bool()>& abort_check) {
    if (!abort_check) return MoveRemote(srcDevicePath, dstDeviceDir);
    if (int err = ADBUtils::CheckConnection(_connected)) return err;
    std::string sh = "mv -- " + ADBUtils::ShellQuote(srcDevicePath) + " " + ADBUtils::ShellQuote(dstDeviceDir);
    const std::string out = RunAdbCommandWithProgress({"shell", sh}, [](const std::string&){}, abort_check);
    if (abort_check()) return ECANCELED;
    return MutationResultToErrno(ADBShell::lastPtyExit(), out);
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

    // Single `find` over all roots — one shell roundtrip; %p (full path) lets us prefix-match each file back to its root.
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
