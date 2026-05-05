#include "ADBDialogs.h"
#include "ADBLog.h"
#include "farplug-wide.h"
#include <utils.h>
#include <algorithm>
#include <chrono>
#include <sstream>
#include <iomanip>

extern PluginStartupInfo g_Info;
extern FarStandardFunctions g_FSF;

// --- Helpers ---

// Native far2l format (copy.cpp:3090): 17-col label + 25-col size + date/time. 17+25+20=62 cells = OverwriteDialog row width (5..66), so size+date hug the right border.
static std::wstring FormatFileInfo(const wchar_t* label, uint64_t size, int64_t mtime)
{
    std::wostringstream out;
    out << label;
    int pad = 17 - (int)wcslen(label);
    if (pad < 1) pad = 1;
    out << std::wstring(pad, L' ');
    std::wstring sz = std::to_wstring(size);
    if (sz.size() < 25) out << std::wstring(25 - sz.size(), L' ');
    out << sz;
    if (mtime > 0) {
        std::time_t t = static_cast<std::time_t>(mtime);
        std::tm tm{};
        if (localtime_r(&t, &tm)) {
            wchar_t buf[64];
            swprintf(buf, sizeof(buf)/sizeof(buf[0]),
                     L" %02d-%02d-%04d %02d:%02d:%02d",
                     tm.tm_mday, tm.tm_mon + 1, tm.tm_year + 1900,
                     tm.tm_hour, tm.tm_min, tm.tm_sec);
            out << buf;
        }
    }
    return out.str();
}

static std::wstring FormatTimeLong(uint64_t total_seconds)
{
    if (total_seconds > 999999) total_seconds = 999999;

    uint64_t hours = total_seconds / 3600;
    uint64_t minutes = (total_seconds % 3600) / 60;
    uint64_t seconds = total_seconds % 60;

    std::wostringstream out;
    out << std::setw(2) << std::setfill(L'0') << hours << L":"
        << std::setw(2) << std::setfill(L'0') << minutes << L":"
        << std::setw(2) << std::setfill(L'0') << seconds;
    return out.str();
}

static std::wstring FormatSpeed(uint64_t bytes_per_sec)
{
    if (bytes_per_sec == 0) return L"- B/s";

    static const wchar_t* units[] = {L"B/s", L"KB/s", L"MB/s", L"GB/s"};
    size_t unit = 0;
    double value = static_cast<double>(bytes_per_sec);
    while (value >= 1024.0 && unit < 3) {
        value /= 1024.0;
        ++unit;
    }

    std::wostringstream out;
    if (unit == 0) {
        out << bytes_per_sec << L" " << units[unit];
    } else {
        out << std::fixed << std::setprecision(1) << value << L" " << units[unit];
    }
    return out.str();
}

static std::wstring AbbreviatePathLeft(const std::wstring &path, size_t max_len)
{
    // Trim from left, show end: "...path/end"
    if (path.size() <= max_len) return path;
    if (max_len < 6) return path.substr(path.size() - max_len);
    return L"..." + path.substr(path.size() - max_len + 3);
}

static std::wstring AbbreviatePathRight(const std::wstring &path, size_t max_len)
{
    // Trim from right, show start: "/start/path..."
    if (path.size() <= max_len) return path;
    if (max_len < 6) return path.substr(0, max_len);
    return path.substr(0, max_len - 3) + L"...";
}

// --- FarDialogItems ---

const wchar_t *FarDialogItems::MB2WidePooled(const char *sz)
{
    if (!sz) return nullptr;
    if (!*sz) return L"";

    MB2Wide(sz, _str_pool_tmp);
    return _str_pool.emplace(_str_pool_tmp).first->c_str();
}

const wchar_t *FarDialogItems::WidePooled(const wchar_t *wz)
{
    if (!wz) return nullptr;
    if (!*wz) return L"";

    return _str_pool.emplace(wz).first->c_str();
}

int FarDialogItems::AddInternal(int type, int x1, int y1, int x2, int y2, unsigned int flags, const wchar_t *data)
{
    int index = (int)size();
    resize(index + 1);
    auto &item = back();

    item.Type = type;
    item.X1 = x1;
    item.Y1 = y1;
    item.X2 = x2;
    item.Y2 = y2;
    item.Focus = 0;
    item.History = nullptr;
    item.Flags = flags;
    item.DefaultButton = 0;
    item.PtrData = data;

    if (index > 0) {
        auto &box = operator[](0);
        if (box.Y2 < y2 + 1) box.Y2 = y2 + 1;
        if (box.X2 < x2 + 2) box.X2 = x2 + 2;
    }

    return index;
}

FarDialogItems::FarDialogItems()
{
    AddInternal(DI_DOUBLEBOX, 3, 1, 5, 3, 0, L"");
}

int FarDialogItems::SetBoxTitleItem(const wchar_t *title)
{
    if (empty()) return -1;
    operator[](0).PtrData = WidePooled(title);
    return 0;
}

int FarDialogItems::Add(int type, int x1, int y1, int x2, int y2, unsigned int flags, const wchar_t *data)
{
    return AddInternal(type, x1, y1, x2, y2, flags, WidePooled(data));
}

int FarDialogItems::Add(int type, int x1, int y1, int x2, int y2, unsigned int flags, const char *data)
{
    return AddInternal(type, x1, y1, x2, y2, flags, MB2WidePooled(data));
}

int FarDialogItems::EstimateWidth() const
{
    int min_x = 0, max_x = 0;
    for (const auto &item : *this) {
        if (min_x > item.X1 || &item == &front()) min_x = item.X1;
        if (max_x < item.X1 || &item == &front()) max_x = item.X1;
        if (max_x < item.X2) max_x = item.X2;
    }
    return max_x + 1 - min_x;
}

int FarDialogItems::EstimateHeight() const
{
    int min_y = 0, max_y = 0;
    for (const auto &item : *this) {
        if (min_y > item.Y1 || &item == &front()) min_y = item.Y1;
        if (max_y < item.Y1 || &item == &front()) max_y = item.Y1;
        if (max_y < item.Y2) max_y = item.Y2;
    }
    return max_y + 1 - min_y;
}

// --- FarDialogItemsLineGrouped ---

void FarDialogItemsLineGrouped::SetLine(int y)
{
    _y = y;
}

void FarDialogItemsLineGrouped::NextLine()
{
    ++_y;
}

int FarDialogItemsLineGrouped::AddAtLine(int type, int x1, int x2, unsigned int flags, const char *data)
{
    return Add(type, x1, _y, x2, _y, flags, data);
}

int FarDialogItemsLineGrouped::AddAtLine(int type, int x1, int x2, unsigned int flags, const wchar_t *data)
{
    return Add(type, x1, _y, x2, _y, flags, data);
}

// --- BaseDialog ---

BaseDialog::~BaseDialog()
{
    if (_dlg != INVALID_HANDLE_VALUE) {
        g_Info.DialogFree(_dlg);
    }
}

LONG_PTR BaseDialog::sSendDlgMessage(HANDLE dlg, int msg, int param1, LONG_PTR param2)
{
    return g_Info.SendDlgMessage(dlg, msg, param1, param2);
}

LONG_PTR BaseDialog::SendDlgMessage(int msg, int param1, LONG_PTR param2)
{
    return sSendDlgMessage(_dlg, msg, param1, param2);
}

LONG_PTR WINAPI BaseDialog::sDlgProc(HANDLE dlg, int msg, int param1, LONG_PTR param2)
{
    BaseDialog *it = (BaseDialog *)sSendDlgMessage(dlg, DM_GETDLGDATA, 0, 0);
    if (it && dlg == it->_dlg) {
        return it->DlgProc(msg, param1, param2);
    }
    return g_Info.DefDlgProc(dlg, msg, param1, param2);
}

LONG_PTR BaseDialog::DlgProc(int msg, int param1, LONG_PTR param2)
{
    return g_Info.DefDlgProc(_dlg, msg, param1, param2);
}

int BaseDialog::Show(const wchar_t *help_topic, int extra_width, int extra_height, unsigned int flags)
{
    if (_dlg == INVALID_HANDLE_VALUE) {
        // Universal DIF_SEPARATOR-flush: pin box.X2 = W-4 so horizontal rule joins box border cleanly (fixes "-||" artefact).
        int desired_W = std::max(_di[0].X2 + 4,
                                 _di.EstimateWidth() + extra_width);
        _di[0].X2 = desired_W - 4;
        _dlg = g_Info.DialogInit(g_Info.ModuleNumber, -1, -1,
            desired_W, _di.EstimateHeight() + extra_height,
            help_topic, &_di[0], _di.size(), 0, flags, &sDlgProc, (LONG_PTR)(uintptr_t)this);
        if (_dlg == INVALID_HANDLE_VALUE) return -2;
    }

    return g_Info.DialogRun(_dlg);
}

void BaseDialog::Close(int code)
{
    SendDlgMessage(DM_CLOSE, code, 0);
}

void BaseDialog::SetDefaultDialogControl(int ctl)
{
    if (ctl == -1) {
        if (!_di.empty()) SetDefaultDialogControl((int)(_di.size() - 1));
        return;
    }
    for (size_t i = 0; i < _di.size(); ++i) {
        _di[i].DefaultButton = ((int)i == ctl) ? 1 : 0;
    }
}

void BaseDialog::SetFocusedDialogControl(int ctl)
{
    if (ctl == -1) {
        if (!_di.empty()) SetFocusedDialogControl((int)(_di.size() - 1));
        return;
    }
    for (size_t i = 0; i < _di.size(); ++i) {
        _di[i].Focus = ((int)i == ctl) ? 1 : 0;
    }
}

void BaseDialog::TextToDialogControl(int ctl, const std::wstring &str)
{
    if (ctl < 0 || (size_t)ctl >= _di.size()) return;

    if (_dlg == INVALID_HANDLE_VALUE) {
        _di[ctl].PtrData = _di.WidePooled(str.c_str());
        return;
    }

    FarDialogItemData dd = { str.size(), (wchar_t*)str.c_str() };
    SendDlgMessage(DM_SETTEXT, ctl, (LONG_PTR)&dd);
}

void BaseDialog::TextToDialogControl(int ctl, const char *str)
{
    if (!str) return;
    std::wstring tmp;
    MB2Wide(str, tmp);
    TextToDialogControl(ctl, tmp);
}

void BaseDialog::ProgressBarToDialogControl(int ctl, int percents)
{
    if (ctl < 0 || (size_t)ctl >= _di.size()) return;

    if (_progress_bg == 0) {
        if (g_FSF.BoxSymbols) _progress_bg = g_FSF.BoxSymbols[BS_X_B0];
        if (_progress_bg == 0) _progress_bg = L'.';
    }
    if (_progress_fg == 0) {
        if (g_FSF.BoxSymbols) _progress_fg = g_FSF.BoxSymbols[BS_X_DB];
        if (_progress_fg == 0) _progress_fg = L'#';
    }

    std::wstring str;
    int width = _di[ctl].X2 + 1 - _di[ctl].X1;
    str.resize(width);

    if (percents >= 0) {
        int filled = (percents * width) / 100;
        for (int i = 0; i < filled; ++i) str[i] = _progress_fg;
        for (int i = filled; i < width; ++i) str[i] = _progress_bg;
    } else {
        for (auto &c : str) c = L'-';
    }

    TextToDialogControl(ctl, str);
}

// --- AbortConfirmDialog ---

AbortConfirmDialog::AbortConfirmDialog()
{
    _di.SetBoxTitleItem(L"Abort operation");
    _di.SetLine(2);
    _di.AddAtLine(DI_TEXT, 5, 50, DIF_CENTERGROUP, L"Confirm abort current operation");
    _di.NextLine();
    _di.AddAtLine(DI_TEXT, 5, 50, DIF_BOXCOLOR | DIF_SEPARATOR);
    _di.NextLine();
    _i_confirm = _di.AddAtLine(DI_BUTTON, 6, 27, DIF_CENTERGROUP, L"&Abort operation");
    _i_cancel = _di.AddAtLine(DI_BUTTON, 32, 45, DIF_CENTERGROUP, L"&Continue");
    SetFocusedDialogControl(_i_cancel);
    SetDefaultDialogControl(_i_cancel);
}

LONG_PTR AbortConfirmDialog::DlgProc(int msg, int param1, LONG_PTR param2)
{
    if (msg == DM_KEY && param2 == 0x1b) {
        Close(_i_cancel);
        return TRUE;
    }
    return BaseDialog::DlgProc(msg, param1, param2);
}

bool AbortConfirmDialog::Ask()
{
    int reply = Show(L"ADBAbortConfirm", 3, 2, FDLG_WARNING);
    return (reply == _i_confirm || reply < 0);
}

// --- OverwriteDialog ---

OverwriteDialog::OverwriteDialog(const std::wstring &filename, bool is_multiple, bool is_directory,
                                 uint64_t src_size, int64_t src_mtime,
                                 uint64_t dst_size, int64_t dst_mtime,
                                 ViewFn view_new, ViewFn view_existing)
    : _is_multiple(is_multiple),
      _view_new(std::move(view_new)),
      _view_existing(std::move(view_existing))
{
    // Layout matches MTP plugin's OverwriteDialog: "Warning" box title + centered header line + filename + separators bracketing optional info rows + "Remember choice" checkbox driving sticky-all (no separate "All" buttons). Width 72 (box X2=68) — content area 5..66 = 62 cells.
    _di.SetBoxTitleItem(L"Warning");
    _di.SetLine(2);
    _di.AddAtLine(DI_TEXT, 5, 66, DIF_CENTERGROUP,
                  is_directory ? L"Folder already exists" : L"File already exists");
    _di.NextLine();
    _di.AddAtLine(DI_TEXT, 5, 66, 0, AbbreviatePathLeft(filename, 62).c_str());
    _di.NextLine();
    _di.AddAtLine(DI_TEXT, 5, 0, DIF_BOXCOLOR | DIF_SEPARATOR);

    // New/Existing info rows: clickable button when caller wired a viewer (DN_BTNCLICK in DlgProc), inert text otherwise. Suppressed entirely if no metadata supplied (back-compat for callers that didn't pass sizes/mtimes).
    const bool show_info = (src_size != 0 || src_mtime != 0 || dst_size != 0 || dst_mtime != 0);
    if (show_info) {
        _di.NextLine();
        const int new_type = _view_new ? DI_BUTTON : DI_TEXT;
        const unsigned int new_flags = _view_new ? (DIF_BTNNOCLOSE | DIF_NOBRACKETS) : 0;
        _i_src_info = _di.AddAtLine(new_type, 5, 66, new_flags,
                                    FormatFileInfo(L"New", src_size, src_mtime).c_str());
        _di.NextLine();
        const int dst_type = _view_existing ? DI_BUTTON : DI_TEXT;
        const unsigned int dst_flags = _view_existing ? (DIF_BTNNOCLOSE | DIF_NOBRACKETS) : 0;
        _i_dst_info = _di.AddAtLine(dst_type, 5, 66, dst_flags,
                                    FormatFileInfo(L"Existing", dst_size, dst_mtime).c_str());
        _di.NextLine();
        _di.AddAtLine(DI_TEXT, 5, 0, DIF_BOXCOLOR | DIF_SEPARATOR);
    }

    _di.NextLine();
    _i_remember = _di.AddAtLine(DI_CHECKBOX, 5, 30, 0, L"&Remember choice");
    _di.NextLine();
    _di.AddAtLine(DI_TEXT, 5, 0, DIF_BOXCOLOR | DIF_SEPARATOR);
    _di.NextLine();
    _i_overwrite  = _di.AddAtLine(DI_BUTTON, 0, 0, DIF_CENTERGROUP, L"&Overwrite");
    _i_skip       = _di.AddAtLine(DI_BUTTON, 0, 0, DIF_CENTERGROUP, L"&Skip");
    _i_only_newer = _di.AddAtLine(DI_BUTTON, 0, 0, DIF_CENTERGROUP, L"Ne&wer");
    _i_rename     = _di.AddAtLine(DI_BUTTON, 0, 0, DIF_CENTERGROUP, L"&Rename");
    _i_cancel     = _di.AddAtLine(DI_BUTTON, 0, 0, DIF_CENTERGROUP, L"&Cancel");
    SetFocusedDialogControl(_i_remember);
    SetDefaultDialogControl(_i_overwrite);
}

LONG_PTR OverwriteDialog::DlgProc(int msg, int param1, LONG_PTR param2)
{
    if (msg == DM_KEY && param2 == 0x1b) {
        Close(_i_cancel);
        return TRUE;
    }
    if (msg == DN_BTNCLICK) {
        if (param1 == _i_src_info && _view_new)      { _view_new();      return TRUE; }
        if (param1 == _i_dst_info && _view_existing) { _view_existing(); return TRUE; }
    }
    return BaseDialog::DlgProc(msg, param1, param2);
}

OverwriteDialog::Result OverwriteDialog::Ask()
{
    // W = box.X2+4 = 72; EW=69 → extra_width=3 (matches MTP).
    int reply = Show(L"ADBOverwrite", 3, 2, FDLG_WARNING);
    const bool remember = _is_multiple
        && (SendDlgMessage(DM_GETCHECK, _i_remember, 0) == BSTATE_CHECKED);
    if (reply == _i_overwrite)  return remember ? OVERWRITE_ALL : OVERWRITE;
    if (reply == _i_skip)       return remember ? SKIP_ALL : SKIP;
    if (reply == _i_only_newer) return ONLY_NEWER;  // sticky-newer not currently consumed by ADB callsites
    if (reply == _i_rename)     return RENAME;
    return CANCEL;
}

// --- ProgressDialog ---

ProgressDialog::ProgressDialog(ProgressState &state, const std::wstring &title, bool is_multi)
    : _state(state), _is_multi(is_multi)
{
    InitLayout(title);
}

void ProgressDialog::InitLayout(const std::wstring &title)
{
    _di.SetBoxTitleItem(title.c_str());

    _di.SetLine(2);
    _i_operation_label = _di.AddAtLine(DI_TEXT, 5, 58, 0, L"Copy from:");

    _di.NextLine();
    _i_from_path = _di.AddAtLine(DI_TEXT, 5, 58, 0, L"...");

    _di.NextLine();
    _di.AddAtLine(DI_TEXT, 5, 7, 0, L"to:");

    _di.NextLine();
    _i_to_path = _di.AddAtLine(DI_TEXT, 5, 58, 0, L"...");

    _di.NextLine();
    _i_progress_bar = _di.AddAtLine(DI_TEXT, 5, 54, 0);
    _i_percent = _di.AddAtLine(DI_TEXT, 56, 58, 0, L"0%");

    if (_is_multi) {
        // Multi: file bar / Total-bytes header / aggregate bar / "Files processed" / divider / time.
        _di.NextLine();
        _i_total_bytes = _di.AddAtLine(DI_TEXT, 4, 60, DIF_BOXCOLOR | DIF_SEPARATOR, L" Total: 0 bytes ");

        _di.NextLine();
        _i_total_bar = _di.AddAtLine(DI_TEXT, 5, 54, 0);
        _i_total_pct = _di.AddAtLine(DI_TEXT, 56, 58, 0, L"0%");

        _di.NextLine();
        _di.AddAtLine(DI_TEXT, 5, 21, 0, L"Files processed:");
        // Wide counter field — "<count> of <total>" can hit double-digit pairs (e.g. "170 of 171") which a narrow field truncates visually.
        _i_files_processed = _di.AddAtLine(DI_TEXT, 22, 58, 0, L"0");

        _di.NextLine();
        _di.AddAtLine(DI_TEXT, 4, 60, DIF_BOXCOLOR | DIF_SEPARATOR);
    } else {
        // Single: bare divider before time line; total bytes implicit in file bar.
        _di.NextLine();
        _di.AddAtLine(DI_TEXT, 4, 60, DIF_BOXCOLOR | DIF_SEPARATOR);
        _i_total_bytes = -1;
        _i_total_bar = -1;
        _i_total_pct = -1;
        _i_files_processed = -1;
    }

    _di.NextLine();
    _i_time = _di.AddAtLine(DI_TEXT, 5, 58, 0, L"");

    _i_cancel = -1;
    _di[0].X2 = 61;
}

void ProgressDialog::Show()
{
    while (!_state.finished) {
        _finished = false;
        BaseDialog::Show(L"ADBProgress", 6, 2, FDLG_REGULARIDLE);
        if (_finished) break;

        if (ShowAbortConfirmation()) {
            _state.SetAborting();
            break;
        }
    }
}

bool ProgressDialog::ShowAbortConfirmation()
{
    if (_state.IsAborting()) return true;
    AbortConfirmDialog dlg;
    return dlg.Ask();
}

LONG_PTR ProgressDialog::DlgProc(int msg, int param1, LONG_PTR param2)
{
    if (msg == DN_ENTERIDLE) {
        if (_state.finished) {
            if (!_finished) {
                _finished = true;
                Close();
            }
        } else {
            UpdateDialog();
        }
    } else if (msg == DN_BTNCLICK && param1 == _i_cancel) {
        if (ShowAbortConfirmation()) {
            _state.SetAborting();
            Close();
        }
        return TRUE;
    }

    return BaseDialog::DlgProc(msg, param1, param2);
}

void ProgressDialog::UpdateDialog()
{
    // Read string values with mutex protection
    std::wstring source_path, dest_path, current_file;
    {
        std::lock_guard<std::mutex> locker(_state.mtx_strings);
        source_path = _state.source_path;
        dest_path = _state.dest_path;
        current_file = _state.current_file;
    }

    uint64_t all_complete = _state.all_complete.load();
    uint64_t all_total = _state.all_total.load();
    uint64_t count_complete = _state.count_complete.load();
    uint64_t count_total = _state.count_total.load();
    bool is_directory = _state.is_directory.load();
    int file_percent = (int)_state.file_complete.load();
    if (file_percent > 100) file_percent = 100;

    // Trailing "/" only when current_file is empty — never glued onto an inner filename.
    std::wstring full_from = source_path;
    std::wstring full_to = dest_path;
    if (!current_file.empty()) {
        if (!full_from.empty() && full_from.back() != L'/') full_from += L'/';
        full_from += current_file;
        if (!full_to.empty() && full_to.back() != L'/' && full_to.back() != L'\\') full_to += L'/';
        full_to += current_file;
    } else if (is_directory) {
        if (!full_from.empty() && full_from.back() != L'/') full_from += L'/';
        if (!full_to.empty() && full_to.back() != L'/' && full_to.back() != L'\\') full_to += L'/';
    }

    std::wstring op_label = _is_multi
        ? L"Copy from:"
        : (is_directory ? L"Copy the folder from:" : L"Copy the file from:");

    bool progress_changed = _first_update || all_complete != _last_complete || all_total != _last_total || file_percent != _last_file_percent;
    bool count_changed = _first_update || count_complete != _last_count;
    bool path_changed = _first_update || full_from != _last_from || full_to != _last_to;

    if (path_changed) {
        TextToDialogControl(_i_operation_label, op_label);
    }

    if (_first_update) {
        _first_update = false;
    }

    if (path_changed) {
        _last_from = full_from;
        _last_to = full_to;
        TextToDialogControl(_i_from_path, AbbreviatePathLeft(full_from, 54));
        TextToDialogControl(_i_to_path, AbbreviatePathRight(full_to, 54));
    }

    int total_percent = (all_total > 0 && all_complete <= all_total)
        ? static_cast<int>((all_complete * 100) / all_total) : 0;

    if (progress_changed || total_percent != _last_total_percent) {
        _last_complete = all_complete;
        _last_total = all_total;
        _last_file_percent = file_percent;
        _last_total_percent = total_percent;

        ProgressBarToDialogControl(_i_progress_bar, file_percent);
        TextToDialogControl(_i_percent, std::to_wstring(file_percent) + L"%");

        if (_i_total_bytes >= 0) {
            std::wstring bytes_str;
            for (uint64_t t = all_total; t > 0; t /= 10) {
                if (bytes_str.size() % 4 == 3) bytes_str = L' ' + bytes_str;
                bytes_str = std::to_wstring(t % 10) + bytes_str;
            }
            if (bytes_str.empty()) bytes_str = L"0";
            TextToDialogControl(_i_total_bytes, L" Total: " + bytes_str + L" bytes ");
        }

        if (_is_multi && _i_total_bar >= 0) {
            ProgressBarToDialogControl(_i_total_bar, total_percent);
            TextToDialogControl(_i_total_pct, std::to_wstring(total_percent) + L"%");
        }
    }

    if (count_changed && _i_files_processed >= 0) {
        _last_count = count_complete;
        std::wstring counter = std::to_wstring(count_complete);
        if (count_total > 1) counter += L" of " + std::to_wstring(count_total);
        TextToDialogControl(_i_files_processed, counter);
    }

    // Time, remaining, speed - properly aligned
    auto now = std::chrono::steady_clock::now();
    auto elapsed_ms = std::chrono::duration_cast<std::chrono::milliseconds>(now - _state.start_time).count();
    if (elapsed_ms < 1) elapsed_ms = 1;

    // Windowed instant speed + EMA smoothing — adb -p emits in bursts; raw avg/instant jumps.
    auto speed_elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(now - _prev_ts).count();
    if (_prev_ts.time_since_epoch().count() == 0) {
        _prev_bytes = all_complete;
        _prev_ts = now;
    } else if (speed_elapsed >= 500) {
        uint64_t inst = 0;
        if (all_complete > _prev_bytes) {
            inst = ((all_complete - _prev_bytes) * 1000) / (uint64_t)speed_elapsed;
        }
        _speed = _speed ? (uint64_t)(_speed * 0.7 + inst * 0.3) : inst;
        _prev_bytes = all_complete;
        _prev_ts = now;
    }

    std::wstring time_part = L"Time: " + FormatTimeLong(elapsed_ms / 1000);

    uint64_t new_remaining = 0;
    bool have_remaining = false;
    if (_speed > 0 && all_complete < all_total && all_total > 0) {
        new_remaining = (all_total - all_complete) / _speed;
        have_remaining = true;
    } else if (file_percent > 0 && file_percent < 100) {
        uint64_t total_estimated = (elapsed_ms * 100) / file_percent;
        new_remaining = (total_estimated - elapsed_ms) / 1000;
        have_remaining = true;
    }

    std::wstring remaining_str;
    if (have_remaining) {
        _smoothed_remaining_sec = _smoothed_remaining_sec
            ? (uint64_t)(_smoothed_remaining_sec * 0.6 + new_remaining * 0.4)
            : new_remaining;
        remaining_str = FormatTimeLong(_smoothed_remaining_sec);
    } else {
        remaining_str = L"??:??:??";
    }
    std::wstring remaining_part = L"Remaining: " + remaining_str;

    std::wstring speed_str = FormatSpeed(_speed);

    const int line_width = 54;  // positions 5-58
    int time_len = time_part.size();
    int remaining_len = remaining_part.size();
    int speed_len = speed_str.size();

    // Space between time and remaining, and between remaining and speed
    int total_fixed = time_len + remaining_len + speed_len;
    int total_space = line_width - total_fixed;
    int space_left = total_space / 2;
    int space_right = total_space - space_left;

    // Build aligned line
    std::wstring time_line = time_part + std::wstring(space_left, L' ') + remaining_part + std::wstring(space_right, L' ') + speed_str;

    TextToDialogControl(_i_time, time_line);
}

// --- ProgressOperation ---

ProgressOperation::ProgressOperation(const std::wstring& title, bool is_multi)
    : _state(std::make_shared<ProgressState>()), _title(title), _is_multi(is_multi)
{
    _state->Reset();
}

ProgressOperation::~ProgressOperation()
{
    if (_worker_thread.joinable()) {
        _worker_thread.join();
    }
}

void ProgressOperation::Run(WorkFunc work_func)
{
    // Capture _state (shared_ptr) by value so it stays alive even if we detach
    auto state_ptr = _state;
    _worker_thread = std::thread([state_ptr, work_func]() {
        try {
            work_func(*state_ptr);
        } catch (const std::exception& e) {
            DBG("ProgressOperation worker exception: %s\n", e.what());
        } catch (...) {
            DBG("ProgressOperation worker unknown exception\n");
        }
        state_ptr->SetFinished();

        // NOOP_EVENT -> KEY_IDLE -> DN_ENTERIDLE
        INPUT_RECORD ir = {};
        ir.EventType = NOOP_EVENT;
        DWORD dw = 0;
        WINPORT(WriteConsoleInput)(0, &ir, 1, &dw);
    });

    // Delay-show 300ms via cv_finish — sub-second ops don't flash a modal; wakes instantly on completion.
    {
        std::unique_lock<std::mutex> lock(_state->mtx_finish);
        _state->cv_finish.wait_for(lock, std::chrono::milliseconds(300),
                                   [&]{ return _state->IsFinished(); });
    }
    if (!_state->IsFinished()) {
        ProgressDialog dlg(*_state, _title, _is_multi);
        dlg.Show();
    }

    // Join: lambda captures locals by ref; abort kills adb within ~200ms.
    if (_worker_thread.joinable()) {
        _worker_thread.join();
    }
}

// --- DeleteProgressDialog ---

DeleteProgressDialog::DeleteProgressDialog(ProgressState& state)
    : _state(state) {
    // 40-char DIF_CENTERTEXT field; box.X2=46, EW=44, W=50, extra_width=6.
    _di.SetBoxTitleItem(L"Delete");
    _di.SetLine(2);
    _di.AddAtLine(DI_TEXT, 5, 44, DIF_CENTERTEXT, L"Deleting the file or folder");
    _di.NextLine();
    _i_filename = _di.AddAtLine(DI_TEXT, 5, 44, DIF_CENTERTEXT, L"");
}

void DeleteProgressDialog::Show() {
    while (!_state.finished) {
        _finished = false;
        // box.X2=46, EW=44 (min_x=3, no buttons at X1=0), extra_width=6 → W=50=46+4.
        BaseDialog::Show(L"ADBProgress", 6, 2, FDLG_REGULARIDLE);
        if (_finished) break;
        if (_state.IsAborting()) break;
        if (ShowAbortConfirmation()) {
            _state.SetAborting();
            break;
        }
    }
}

bool DeleteProgressDialog::ShowAbortConfirmation() {
    if (_state.IsAborting()) return true;
    AbortConfirmDialog dlg;
    return dlg.Ask();
}

LONG_PTR DeleteProgressDialog::DlgProc(int msg, int param1, LONG_PTR param2) {
    if (msg == DN_ENTERIDLE) {
        if (_state.finished) {
            if (!_finished) {
                _finished = true;
                Close();
            }
        } else {
            UpdateDialog();
        }
    } else if (msg == DM_KEY && param2 == 0x1b) {
        if (ShowAbortConfirmation()) {
            _state.SetAborting();
            Close();
        }
        return TRUE;
    }
    return BaseDialog::DlgProc(msg, param1, param2);
}

void DeleteProgressDialog::UpdateDialog() {
    std::wstring current_file;
    {
        std::lock_guard<std::mutex> locker(_state.mtx_strings);
        current_file = _state.current_file;
    }
    if (current_file == _last_filename) return;
    _last_filename = current_file;
    TextToDialogControl(_i_filename, AbbreviatePathLeft(current_file, 40));
}

// --- DeleteOperation ---

DeleteOperation::DeleteOperation()
    : _state(std::make_shared<ProgressState>()) {
    _state->Reset();
}

void DeleteOperation::Run(WorkFunc work_func) {
    auto state_ptr = _state;
    std::thread worker([state_ptr, work_func]() {
        try { work_func(*state_ptr); } catch (...) {}
        state_ptr->SetFinished();
        INPUT_RECORD ir = {};
        ir.EventType = NOOP_EVENT;
        DWORD dw = 0;
        WINPORT(WriteConsoleInput)(0, &ir, 1, &dw);
    });
    DeleteProgressDialog dlg(*_state);
    dlg.Show();
    if (worker.joinable()) worker.join();
}

// --- ADBDialogs ---

bool ADBDialogs::AskCopyMove(bool is_move, bool is_upload, std::string& destination,
                            const std::string& source_name, int item_count)
{
    const wchar_t* title;
    if (is_upload && is_move)      title = L"Move to device";
    else if (is_upload)            title = L"Copy to device";
    else if (is_move)              title = L"Move from device";
    else                           title = L"Copy from device";

    std::wstring prompt;
    const std::wstring verb = is_move ? L"Move " : L"Copy ";
    if (item_count > 1) {
        wchar_t count_str[32];
        swprintf(count_str, ARRAYSIZE(count_str), L"%d", item_count);
        prompt = verb + count_str + L" items to:";
    } else if (!source_name.empty()) {
        prompt = verb + L"\"" + StrMB2Wide(source_name) + L"\" to:";
    } else {
        prompt = L"Enter destination path:";
    }

    std::string default_path = destination;
    std::string other_panel_path = default_path;
    if (!is_upload) {
        int size = g_Info.Control(PANEL_PASSIVE, FCTL_GETPANELDIR, 0, (LONG_PTR)0);
        if (size > 0) {
            std::vector<wchar_t> buffer(size);
            if (g_Info.Control(PANEL_PASSIVE, FCTL_GETPANELDIR, size, (LONG_PTR)buffer.data())) {
                other_panel_path = StrWide2MB(buffer.data());
            }
        }
    }

    return AskInput(title, prompt.c_str(), L"ADB_CopyMove", destination,
                    other_panel_path.empty() ? default_path : other_panel_path);
}

bool ADBDialogs::AskCreateDirectory(std::string& dir_name)
{
    if (!AskInput(L"Create directory", L"Enter name of directory to create:",
                  L"ADB_MakeDir", dir_name, dir_name)) {
        return false;
    }
    // Reject whitespace/./.. early — would fail or silently no-op on device.
    auto ltrim = dir_name.find_first_not_of(" \t\r\n");
    if (ltrim == std::string::npos) return false;
    auto rtrim = dir_name.find_last_not_of(" \t\r\n");
    dir_name = dir_name.substr(ltrim, rtrim - ltrim + 1);
    if (dir_name == "." || dir_name == "..") return false;
    return true;
}

bool ADBDialogs::AskInput(const wchar_t* title, const wchar_t* prompt,
                         const wchar_t* history_name, std::string& input,
                         const std::string& default_value)
{
    wchar_t input_buffer[4096] = {0};
    if (!default_value.empty()) {
        std::wstring wval = StrMB2Wide(default_value);
        wcsncpy(input_buffer, wval.c_str(), ARRAYSIZE(input_buffer) - 1);
    }

    std::wstring src_text_wstr = default_value.empty() ? L"" : StrMB2Wide(default_value);
    const wchar_t* src_text = src_text_wstr.empty() ? nullptr : src_text_wstr.c_str();

    bool result = g_Info.InputBox(title, prompt, history_name, src_text, input_buffer,
                                  ARRAYSIZE(input_buffer) - 1, nullptr, FIB_BUTTONS | FIB_NOUSELASTHISTORY);

    if (result) {
        input = StrWide2MB(input_buffer);
    }

    return result && !input.empty();
}

bool ADBDialogs::AskConfirmation(const wchar_t* title, const wchar_t* message)
{
    const wchar_t* msg[] = { title, message, L"OK", L"Cancel" };
    int result = g_Info.Message(g_Info.ModuleNumber, FMSG_MB_YESNO, nullptr, msg, ARRAYSIZE(msg), 0);
    return (result == 0);
}

bool ADBDialogs::AskWarning(const wchar_t* title, const wchar_t* message)
{
    const wchar_t* msg[] = { title, message, L"OK", L"Cancel" };
    int result = g_Info.Message(g_Info.ModuleNumber, FMSG_WARNING | FMSG_MB_YESNO, nullptr, msg, ARRAYSIZE(msg), 0);
    return (result == 0);
}

int ADBDialogs::MessageWrapped(unsigned int flags,
                               const std::wstring& title,
                               const std::wstring& body,
                               size_t wrap)
{
    std::vector<std::wstring> lines;
    lines.push_back(title);
    if (body.empty()) {
        lines.push_back(L"unknown error");
    } else {
        for (size_t i = 0; i < body.size(); i += wrap) {
            lines.push_back(body.substr(i, wrap));
        }
    }
    std::vector<const wchar_t*> ptrs;
    ptrs.reserve(lines.size());
    for (const auto& s : lines) ptrs.push_back(s.c_str());
    return g_Info.Message(g_Info.ModuleNumber, flags, nullptr,
                          ptrs.data(), static_cast<int>(ptrs.size()), 0);
}
