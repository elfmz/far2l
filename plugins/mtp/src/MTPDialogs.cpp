// Ported 1:1 from adb/src/ADBDialogs.cpp; keep cherry-pick-identical.

#include "MTPDialogs.h"
#include "MTPLog.h"
#include "lng.h"
#include "farplug-wide.h"
#include <utils.h>
#include <algorithm>
#include <chrono>
#include <sstream>
#include <iomanip>

extern PluginStartupInfo g_Info;
extern FarStandardFunctions g_FSF;

static std::wstring FormatTimeLong(uint64_t total_seconds) {
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

static std::wstring FormatBytes(uint64_t bytes) {
    static const wchar_t* units[] = {L"B", L"KB", L"MB", L"GB", L"TB"};
    size_t unit = 0;
    double value = static_cast<double>(bytes);
    while (value >= 1024.0 && unit < 4) {
        value /= 1024.0;
        ++unit;
    }
    std::wostringstream out;
    if (unit == 0) {
        out << bytes << L" " << units[unit];
    } else {
        out << std::fixed << std::setprecision(2) << value << L" " << units[unit];
    }
    return out.str();
}

// Native far2l Total separator format: raw byte count, thousands space-separated (e.g. "2 761 173 732").
static std::wstring FormatBytesWithSpaces(uint64_t bytes) {
    std::wstring s = std::to_wstring(bytes);
    if (s.size() <= 3) return s;
    std::wstring out;
    out.reserve(s.size() + s.size() / 3);
    int n = static_cast<int>(s.size());
    for (int i = 0; i < n; ++i) {
        if (i > 0 && ((n - i) % 3) == 0) out.push_back(L' ');
        out.push_back(s[i]);
    }
    return out;
}

// Native format "<value> <prefix>b/s"; lowercase 'b' is literal.
static std::wstring FormatSpeed(uint64_t bytes_per_sec) {
    if (bytes_per_sec == 0) return L"- b/s";
    static const wchar_t* units[] = {L"", L"K", L"M", L"G"};
    size_t unit = 0;
    double value = static_cast<double>(bytes_per_sec);
    while (value >= 1024.0 && unit < 3) {
        value /= 1024.0;
        ++unit;
    }
    std::wostringstream out;
    if (unit == 0) {
        out << bytes_per_sec << L" b/s";
    } else {
        out << std::fixed << std::setprecision(2) << value << L" " << units[unit] << L"b/s";
    }
    return out.str();
}

static std::wstring AbbreviatePathLeft(const std::wstring& path, size_t max_len) {
    if (path.size() <= max_len) return path;
    if (max_len < 6) return path.substr(path.size() - max_len);
    return L"..." + path.substr(path.size() - max_len + 3);
}

static std::wstring AbbreviatePathRight(const std::wstring& path, size_t max_len) {
    if (path.size() <= max_len) return path;
    if (max_len < 6) return path.substr(0, max_len);
    return path.substr(0, max_len - 3) + L"...";
}

const wchar_t* FarDialogItems::MB2WidePooled(const char* sz) {
    if (!sz) return nullptr;
    if (!*sz) return L"";
    MB2Wide(sz, _str_pool_tmp);
    return _str_pool.emplace(_str_pool_tmp).first->c_str();
}

const wchar_t* FarDialogItems::WidePooled(const wchar_t* wz) {
    if (!wz) return nullptr;
    if (!*wz) return L"";
    return _str_pool.emplace(wz).first->c_str();
}

int FarDialogItems::AddInternal(int type, int x1, int y1, int x2, int y2, unsigned int flags, const wchar_t* data) {
    int index = (int)size();
    resize(index + 1);
    auto& item = back();
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
        auto& box = operator[](0);
        if (box.Y2 < y2 + 1) box.Y2 = y2 + 1;
        if (box.X2 < x2 + 2) box.X2 = x2 + 2;
    }
    return index;
}

FarDialogItems::FarDialogItems() {
    AddInternal(DI_DOUBLEBOX, 3, 1, 5, 3, 0, L"");
}

int FarDialogItems::SetBoxTitleItem(const wchar_t* title) {
    if (empty()) return -1;
    operator[](0).PtrData = WidePooled(title);
    return 0;
}

int FarDialogItems::Add(int type, int x1, int y1, int x2, int y2, unsigned int flags, const wchar_t* data) {
    return AddInternal(type, x1, y1, x2, y2, flags, WidePooled(data));
}

int FarDialogItems::Add(int type, int x1, int y1, int x2, int y2, unsigned int flags, const char* data) {
    return AddInternal(type, x1, y1, x2, y2, flags, MB2WidePooled(data));
}

int FarDialogItems::EstimateWidth() const {
    int min_x = 0, max_x = 0;
    for (const auto& item : *this) {
        if (min_x > item.X1 || &item == &front()) min_x = item.X1;
        if (max_x < item.X1 || &item == &front()) max_x = item.X1;
        if (max_x < item.X2) max_x = item.X2;
    }
    return max_x + 1 - min_x;
}

int FarDialogItems::EstimateHeight() const {
    int min_y = 0, max_y = 0;
    for (const auto& item : *this) {
        if (min_y > item.Y1 || &item == &front()) min_y = item.Y1;
        if (max_y < item.Y1 || &item == &front()) max_y = item.Y1;
        if (max_y < item.Y2) max_y = item.Y2;
    }
    return max_y + 1 - min_y;
}

void FarDialogItemsLineGrouped::SetLine(int y) { _y = y; }
void FarDialogItemsLineGrouped::NextLine() { ++_y; }

int FarDialogItemsLineGrouped::AddAtLine(int type, int x1, int x2, unsigned int flags, const char* data) {
    return Add(type, x1, _y, x2, _y, flags, data);
}

int FarDialogItemsLineGrouped::AddAtLine(int type, int x1, int x2, unsigned int flags, const wchar_t* data) {
    return Add(type, x1, _y, x2, _y, flags, data);
}

BaseDialog::~BaseDialog() {
    if (_dlg != INVALID_HANDLE_VALUE) {
        g_Info.DialogFree(_dlg);
    }
}

LONG_PTR BaseDialog::sSendDlgMessage(HANDLE dlg, int msg, int param1, LONG_PTR param2) {
    return g_Info.SendDlgMessage(dlg, msg, param1, param2);
}

LONG_PTR BaseDialog::SendDlgMessage(int msg, int param1, LONG_PTR param2) {
    return sSendDlgMessage(_dlg, msg, param1, param2);
}

LONG_PTR WINAPI BaseDialog::sDlgProc(HANDLE dlg, int msg, int param1, LONG_PTR param2) {
    BaseDialog* it = (BaseDialog*)sSendDlgMessage(dlg, DM_GETDLGDATA, 0, 0);
    if (it && dlg == it->_dlg) {
        return it->DlgProc(msg, param1, param2);
    }
    return g_Info.DefDlgProc(dlg, msg, param1, param2);
}

LONG_PTR BaseDialog::DlgProc(int msg, int param1, LONG_PTR param2) {
    return g_Info.DefDlgProc(_dlg, msg, param1, param2);
}

int BaseDialog::Show(const wchar_t* help_topic, int extra_width, int extra_height, unsigned int flags) {
    if (_dlg == INVALID_HANDLE_VALUE) {
        // Auto-flush DIF_SEPARATOR ↔ box right border: far2l draws separator screen_left+3..+W-4 ignoring X1/X2; box right at +box.X2 — need W-4==box.X2 → W=box.X2+4; extra_width is minimum, box.X2 grows to keep flush.
        int desired_W = std::max(_di[0].X2 + 4,
                                 _di.EstimateWidth() + extra_width);
        _di[0].X2 = desired_W - 4;
        DBG("BaseDialog::Show DialogInit help=%ls items=%zu w=%d h=%d flags=0x%x\n",
            help_topic, _di.size(),
            desired_W, _di.EstimateHeight() + extra_height, flags);
        _dlg = g_Info.DialogInit(g_Info.ModuleNumber, -1, -1,
            desired_W, _di.EstimateHeight() + extra_height,
            help_topic, &_di[0], _di.size(), 0, flags, &sDlgProc, (LONG_PTR)(uintptr_t)this);
        DBG("BaseDialog::Show DialogInit returned dlg=%p\n", (void*)_dlg);
        if (_dlg == INVALID_HANDLE_VALUE) return -2;
    }
    int rc = g_Info.DialogRun(_dlg);
    DBG("BaseDialog::Show DialogRun returned rc=%d\n", rc);
    return rc;
}

void BaseDialog::Close(int code) {
    SendDlgMessage(DM_CLOSE, code, 0);
}

void BaseDialog::SetDefaultDialogControl(int ctl) {
    if (ctl == -1) {
        if (!_di.empty()) SetDefaultDialogControl((int)(_di.size() - 1));
        return;
    }
    for (size_t i = 0; i < _di.size(); ++i) {
        _di[i].DefaultButton = ((int)i == ctl) ? 1 : 0;
    }
}

void BaseDialog::SetFocusedDialogControl(int ctl) {
    if (ctl == -1) {
        if (!_di.empty()) SetFocusedDialogControl((int)(_di.size() - 1));
        return;
    }
    for (size_t i = 0; i < _di.size(); ++i) {
        _di[i].Focus = ((int)i == ctl) ? 1 : 0;
    }
}

void BaseDialog::TextToDialogControl(int ctl, const std::wstring& str) {
    if (ctl < 0 || (size_t)ctl >= _di.size()) return;
    if (_dlg == INVALID_HANDLE_VALUE) {
        _di[ctl].PtrData = _di.WidePooled(str.c_str());
        return;
    }
    FarDialogItemData dd = { str.size(), (wchar_t*)str.c_str() };
    SendDlgMessage(DM_SETTEXT, ctl, (LONG_PTR)&dd);
}

void BaseDialog::TextToDialogControl(int ctl, const char* str) {
    if (!str) return;
    std::wstring tmp;
    MB2Wide(str, tmp);
    TextToDialogControl(ctl, tmp);
}

void BaseDialog::ProgressBarToDialogControl(int ctl, int percents) {
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
        for (auto& c : str) c = L'-';
    }
    TextToDialogControl(ctl, str);
}

AbortConfirmDialog::AbortConfirmDialog() {
    _di.SetBoxTitleItem(Lng(MAbortBoxTitle));
    _di.SetLine(2);
    _di.AddAtLine(DI_TEXT, 5, 48, DIF_CENTERGROUP, Lng(MAbortConfirmText));
    _di.NextLine();
    _di.AddAtLine(DI_TEXT, 4, 49, DIF_BOXCOLOR | DIF_SEPARATOR);
    _di.NextLine();
    _i_confirm = _di.AddAtLine(DI_BUTTON, 6, 27, DIF_CENTERGROUP, Lng(MAbortBtn));
    _i_cancel = _di.AddAtLine(DI_BUTTON, 32, 45, DIF_CENTERGROUP, Lng(MContinueBtn));
    SetFocusedDialogControl(_i_cancel);
    SetDefaultDialogControl(_i_cancel);
}

LONG_PTR AbortConfirmDialog::DlgProc(int msg, int param1, LONG_PTR param2) {
    if (msg == DM_KEY && param2 == 0x1b) {
        Close(_i_cancel);
        return TRUE;
    }
    return BaseDialog::DlgProc(msg, param1, param2);
}

bool AbortConfirmDialog::Ask() {
    int reply = Show(L"MTPAbortConfirm", 6, 2, FDLG_WARNING);
    return (reply == _i_confirm || reply < 0);
}

static std::wstring FormatFileInfo(const wchar_t* label, uint64_t size, int64_t mtime) {
    // Native far2l format (copy.cpp:3090): 17-col label + 25-col size + date/time; padding intentional, don't compress.
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

OverwriteDialog::OverwriteDialog(const std::wstring& dest_name,
                                 uint64_t src_size, int64_t src_mtime,
                                 uint64_t dst_size, int64_t dst_mtime,
                                 ViewFn view_new, ViewFn view_existing)
    : _view_new(std::move(view_new)),
      _view_existing(std::move(view_existing)) {
    // DI_TEXT (not DI_EDIT) so filename shares dialog bg; content width 62 (=5..66); separators X2=0 use FAR auto-stretch.
    _di.SetBoxTitleItem(Lng(MWarning));
    _di.SetLine(2);
    _di.AddAtLine(DI_TEXT, 5, 66, DIF_CENTERGROUP, Lng(MFileExists));
    _di.NextLine();
    _i_filename = _di.AddAtLine(DI_TEXT, 5, 66, 0,
                                AbbreviatePathLeft(dest_name, 62).c_str());
    _di.NextLine();
    _di.AddAtLine(DI_TEXT, 5, 0, DIF_BOXCOLOR | DIF_SEPARATOR);
    _di.NextLine();
    // Info rows: clickable button when caller wired a viewer (DN_BTNCLICK in DlgProc), inert text otherwise.
    const int src_type = _view_new ? DI_BUTTON : DI_TEXT;
    const unsigned int src_flags = _view_new ? (DIF_BTNNOCLOSE | DIF_NOBRACKETS) : 0;
    _i_src_info = _di.AddAtLine(src_type, 5, 66, src_flags,
                                FormatFileInfo(Lng(MNewLabel), src_size, src_mtime).c_str());
    _di.NextLine();
    const int dst_type = _view_existing ? DI_BUTTON : DI_TEXT;
    const unsigned int dst_flags = _view_existing ? (DIF_BTNNOCLOSE | DIF_NOBRACKETS) : 0;
    _i_dst_info = _di.AddAtLine(dst_type, 5, 66, dst_flags,
                                FormatFileInfo(Lng(MExistingLabel), dst_size, dst_mtime).c_str());
    _di.NextLine();
    _di.AddAtLine(DI_TEXT, 5, 0, DIF_BOXCOLOR | DIF_SEPARATOR);
    _di.NextLine();
    _i_remember = _di.AddAtLine(DI_CHECKBOX, 5, 30, 0, Lng(MRememberChoice));
    _di.NextLine();
    _di.AddAtLine(DI_TEXT, 5, 0, DIF_BOXCOLOR | DIF_SEPARATOR);
    _di.NextLine();
    _i_overwrite = _di.AddAtLine(DI_BUTTON, 0, 0, DIF_CENTERGROUP, Lng(MOverwriteBtn));
    _i_skip      = _di.AddAtLine(DI_BUTTON, 0, 0, DIF_CENTERGROUP, Lng(MSkipBtn));
    _i_newer     = _di.AddAtLine(DI_BUTTON, 0, 0, DIF_CENTERGROUP, Lng(MNewerBtn));
    _i_rename    = _di.AddAtLine(DI_BUTTON, 0, 0, DIF_CENTERGROUP, Lng(MRenameBtn));
    _i_cancel    = _di.AddAtLine(DI_BUTTON, 0, 0, DIF_CENTERGROUP, Lng(MCancelBtn));
    SetFocusedDialogControl(_i_remember);
    SetDefaultDialogControl(_i_overwrite);
}

LONG_PTR OverwriteDialog::DlgProc(int msg, int param1, LONG_PTR param2) {
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

OverwriteDialog::Result OverwriteDialog::Ask() {
    // W = box.X2+4 = 72; EW=69 → extra_width=3.
    int reply = Show(L"MTPOverwrite", 3, 2, FDLG_WARNING);
    const bool remember = (SendDlgMessage(DM_GETCHECK, _i_remember, 0) == BSTATE_CHECKED);
    if (reply == _i_overwrite) return remember ? OVERWRITE_ALL : OVERWRITE;
    if (reply == _i_skip)      return remember ? SKIP_ALL : SKIP;
    if (reply == _i_newer)     return remember ? ONLY_NEWER_ALL : ONLY_NEWER;
    if (reply == _i_rename)    return remember ? RENAME_ALL : RENAME;
    return CANCEL;
}

OverwriteDialog::Decision OverwriteDialog::AskSticky(
        const std::wstring& dest_name,
        uint64_t src_size, int64_t src_mtime,
        uint64_t dst_size, int64_t dst_mtime,
        bool& overwrite_all, bool& skip_all,
        bool& newer_all, bool& rename_all,
        ViewFn view_new, ViewFn view_existing) {
    if (overwrite_all) return D_OVERWRITE;
    if (skip_all)      return D_SKIP;
    if (rename_all)    return D_RENAME;
    if (newer_all)     return (src_mtime > dst_mtime) ? D_OVERWRITE : D_SKIP;
    OverwriteDialog dlg(dest_name, src_size, src_mtime, dst_size, dst_mtime,
                        std::move(view_new), std::move(view_existing));
    switch (dlg.Ask()) {
        case OVERWRITE:      return D_OVERWRITE;
        case SKIP:           return D_SKIP;
        case OVERWRITE_ALL:  overwrite_all = true; return D_OVERWRITE;
        case SKIP_ALL:       skip_all = true;      return D_SKIP;
        case ONLY_NEWER:     return (src_mtime > dst_mtime) ? D_OVERWRITE : D_SKIP;
        case ONLY_NEWER_ALL: newer_all = true;     return (src_mtime > dst_mtime) ? D_OVERWRITE : D_SKIP;
        case RENAME:         return D_RENAME;
        case RENAME_ALL:     rename_all = true;    return D_RENAME;
        case CANCEL:         return D_CANCEL;
    }
    return D_CANCEL;
}

ProgressDialog::ProgressDialog(ProgressState& state, bool is_move, bool is_multi, bool is_upload)
    : _state(state), _is_move(is_move), _is_multi(is_multi), _is_upload(is_upload) {
    InitLayout();
}

void ProgressDialog::InitLayout() {
    // Mirrors far2l CopyProgress::CreateBackground (copy.cpp:312): 9/11 rows, BarSize=52, Esc-only abort; direction-aware title.
    const wchar_t* title;
    if (_is_move) {
        title = _is_upload ? Lng(MMoveToDevice) : Lng(MMoveFromDevice);
    } else {
        title = _is_upload ? Lng(MCopyToDevice) : Lng(MCopyFromDevice);
    }
    _di.SetBoxTitleItem(title);
    _di.SetLine(2);
    _i_verb = _di.AddAtLine(DI_TEXT, 5, 60, 0,
                            _is_move ? Lng(MMovingFile) : Lng(MCopyingFile));
    _di.NextLine();
    _i_from_path = _di.AddAtLine(DI_TEXT, 5, 60, 0, L"");
    _di.NextLine();
    _di.AddAtLine(DI_TEXT, 5, 7, 0, Lng(MTo));
    _di.NextLine();
    _i_to_path = _di.AddAtLine(DI_TEXT, 5, 60, 0, L"");
    _di.NextLine();
    _i_progress_bar = _di.AddAtLine(DI_TEXT, 5, 56, 0);     // 52-cell bar
    _i_percent = _di.AddAtLine(DI_TEXT, 58, 62, 0, L"0%");  // " 100%" — 1 col gap from bar
    _di.NextLine();
    if (_is_multi) {
        _i_total_bytes = _di.AddAtLine(DI_TEXT, 5, 63, DIF_BOXCOLOR | DIF_SEPARATOR, L"");
        _di.NextLine();
        _i_total_bar = _di.AddAtLine(DI_TEXT, 5, 56, 0);    // 52-cell total bar
        _di.NextLine();
    }
    // Pin box.X2=63 (line 447) overrides separator auto-grow to 65.
    _di.AddAtLine(DI_TEXT, 5, 63, DIF_BOXCOLOR | DIF_SEPARATOR);
    _di.NextLine();
    _i_files_processed = _di.AddAtLine(DI_TEXT, 5, 60, 0, L"");
    _di.NextLine();
    _di.AddAtLine(DI_TEXT, 5, 63, DIF_BOXCOLOR | DIF_SEPARATOR);
    _di.NextLine();
    _i_time = _di.AddAtLine(DI_TEXT, 5, 60, 0, L"");
    _di[0].X2 = 63;
}

void ProgressDialog::Show() {
    while (!_state.finished) {
        _finished = false;
        // W = box.X2+4 = 67; no buttons → EW=61, extra_width=6.
        BaseDialog::Show(L"MTPProgress", 6, 2, FDLG_REGULARIDLE);
        if (_finished) break;
        if (_state.IsAborting()) break;  // Esc path in DlgProc already aborted+closed; don't re-prompt.
        if (ShowAbortConfirmation()) {
            _state.SetAborting();
            break;
        }
    }
}

bool ProgressDialog::ShowAbortConfirmation() {
    if (_state.IsAborting()) return true;
    AbortConfirmDialog dlg;
    return dlg.Ask();
}

LONG_PTR ProgressDialog::DlgProc(int msg, int param1, LONG_PTR param2) {
    if (msg == DN_ENTERIDLE) {
        if (!_shown_at_set) {
            _shown_at = std::chrono::steady_clock::now();
            _shown_at_set = true;
        }
        if (_state.finished) {
            if (!_finished) {
                _finished = true;
                Close();
            }
        } else {
            UpdateDialog();
        }
    } else if (msg == DM_KEY && param2 == 0x1b) {
        // Esc → abort, gated 200ms after show (see header comment).
        if (_shown_at_set) {
            auto age = std::chrono::duration_cast<std::chrono::milliseconds>(
                std::chrono::steady_clock::now() - _shown_at).count();
            if (age < 200) return TRUE;
        }
        if (ShowAbortConfirmation()) {
            _state.SetAborting();
            Close();
        }
        return TRUE;
    }
    return BaseDialog::DlgProc(msg, param1, param2);
}

void ProgressDialog::UpdateDialog() {
    std::wstring source_path, dest_path, current_file;
    {
        std::lock_guard<std::mutex> locker(_state.mtx_strings);
        source_path = _state.source_path;
        dest_path = _state.dest_path;
        current_file = _state.current_file;
    }
    const uint64_t all_complete = _state.all_complete.load();
    const uint64_t all_total = _state.all_total.load();
    const uint64_t count_complete = _state.count_complete.load();
    const uint64_t count_total = _state.count_total.load();
    const uint64_t file_complete_bytes = _state.file_complete.load();
    const uint64_t file_total_bytes = _state.file_total.load();

    // File bar prefers bytes; falls back to count for sizeless backends.
    int file_percent = 0;
    if (file_total_bytes > 0 && file_complete_bytes <= file_total_bytes) {
        file_percent = static_cast<int>((file_complete_bytes * 100) / file_total_bytes);
    } else if (all_total > 0 && all_complete <= all_total) {
        file_percent = static_cast<int>((all_complete * 100) / all_total);
    } else if (count_total > 0) {
        file_percent = static_cast<int>((count_complete * 100) / count_total);
    }
    if (file_percent > 100) file_percent = 100;

    // _is_multi caller-set (single folder copy has count_total=1 but is still multi-file); byte progress preferred, count fallback.
    int total_percent = file_percent;
    if (_is_multi) {
        if (all_total > 0 && all_complete <= all_total) {
            total_percent = static_cast<int>((all_complete * 100) / all_total);
        } else if (count_total > 1) {
            total_percent = static_cast<int>(std::min<uint64_t>(100, (count_complete * 100) / count_total));
        }
    }

    // Append current item to panel-relative source/dest lines.
    std::wstring full_from = source_path;
    std::wstring full_to = dest_path;
    if (!current_file.empty()) {
        if (!full_from.empty() && full_from.back() != L'/') full_from += L'/';
        full_from += current_file;
        if (!full_to.empty() && full_to.back() != L'/' && full_to.back() != L'\\') full_to += L'/';
        full_to += current_file;
    }

    const bool path_changed = _first_update || full_from != _last_from || full_to != _last_to;
    const bool progress_changed = _first_update || all_complete != _last_complete
                                  || all_total != _last_total
                                  || file_percent != _last_file_percent
                                  || total_percent != _last_total_percent;
    const bool count_changed = _first_update || count_complete != _last_count;

    if (path_changed) {
        _last_from = full_from;
        _last_to = full_to;
        // Truncate to item width (56 cells); overflow shifts the frame.
        TextToDialogControl(_i_from_path, AbbreviatePathLeft(full_from, 56));
        TextToDialogControl(_i_to_path, AbbreviatePathRight(full_to, 56));
    }
    if (progress_changed) {
        _last_complete = all_complete;
        _last_total = all_total;
        _last_file_percent = file_percent;
        _last_total_percent = total_percent;
        ProgressBarToDialogControl(_i_progress_bar, file_percent);
        TextToDialogControl(_i_percent, std::to_wstring(file_percent) + L"%");
        if (_is_multi) {
            // Match native " Total: <raw-bytes-with-spaces> " (copy.cpp:331).
            ProgressBarToDialogControl(_i_total_bar, total_percent);
            const std::wstring total_label = std::wstring(L" ") + Lng(MTotal);
            std::wstring bytes_str = total_label + L" ";
            if (all_total > 0) {
                bytes_str = total_label + L": " + FormatBytesWithSpaces(all_total) + L" ";
            } else if (file_total_bytes > 0) {
                bytes_str = total_label + L": " + FormatBytesWithSpaces(file_total_bytes) + L" ";
            }
            TextToDialogControl(_i_total_bytes, bytes_str);
        }
    }
    if (count_changed) {
        _last_count = count_complete;
        // single: "Files processed: N"; multi: "... N of M".
        std::wstring counter = std::wstring(Lng(MFilesProcessed)) + L" " + std::to_wstring(count_complete);
        if (count_total > 1) counter += std::wstring(L" ") + Lng(MOf) + L" " + std::to_wstring(count_total);
        TextToDialogControl(_i_files_processed, counter);
    }
    _first_update = false;

    // Msg::CopyTimeInfo = "Time: %8.8ls    Remaining: %8.8ls    %8.8lsb/s"
    const auto now = std::chrono::steady_clock::now();
    int64_t elapsed_ms = std::chrono::duration_cast<std::chrono::milliseconds>(now - _state.start_time).count();
    if (elapsed_ms < 1) elapsed_ms = 1;
    if (std::chrono::duration_cast<std::chrono::milliseconds>(now - _prev_ts).count() >= 1000
        || _prev_ts.time_since_epoch().count() == 0) {
        if (all_complete > 0) _speed = (all_complete * 1000) / elapsed_ms;
        _prev_ts = now;
    }
    std::wstring remaining_str = L"??:??:??";
    if (_speed > 0 && all_complete < all_total && all_total > 0) {
        remaining_str = FormatTimeLong((all_total - all_complete) / _speed);
    } else if (file_percent > 0 && file_percent < 100) {
        remaining_str = FormatTimeLong((((uint64_t)elapsed_ms * 100) / file_percent - elapsed_ms) / 1000);
    }
    std::wstring time_str = FormatTimeLong(elapsed_ms / 1000);
    std::wstring speed_str = FormatSpeed(_speed);
    TextToDialogControl(_i_time,
        std::wstring(Lng(MTime)) + L" " + time_str + L"    "
        + Lng(MRemaining) + L" " + remaining_str + L"    " + speed_str);
}

// Wakes modal so DN_ENTERIDLE fires; FDLG_REGULARIDLE alone insufficient on macOS far2l (only one idle event without this).
static void PostNoopEvent() {
    INPUT_RECORD ir = {};
    ir.EventType = NOOP_EVENT;
    DWORD dw = 0;
    WINPORT(WriteConsoleInput)(0, &ir, 1, &dw);
}

// Worker thread + 250ms ticker + modal show, joined on return.
static void RunModalProgress(std::shared_ptr<ProgressState> state,
                             std::function<void(ProgressState&)> work,
                             std::function<void()> show_dialog) {
    std::thread worker([state, work = std::move(work)]() {
        try { work(*state); } catch (...) {}
        state->SetFinished();
        PostNoopEvent();
    });
    // Delay-show 300ms (ADB Lessons E1): sub-second ops (in-device MoveObject, single-file rename, etc.) never flash the modal — feels like a plain mv.
    constexpr int kDelayMs = 300;
    constexpr int kPollMs = 25;
    int waited = 0;
    while (waited < kDelayMs && !state->IsFinished()) {
        std::this_thread::sleep_for(std::chrono::milliseconds(kPollMs));
        waited += kPollMs;
    }
    if (!state->IsFinished()) {
        std::atomic<bool> stop{false};
        std::thread ticker([state, &stop]() {
            while (!stop.load() && !state->IsFinished()) {
                std::this_thread::sleep_for(std::chrono::milliseconds(250));
                if (stop.load() || state->IsFinished()) break;
                PostNoopEvent();
            }
        });
        show_dialog();
        stop.store(true);
        if (ticker.joinable()) ticker.join();
    }
    if (worker.joinable()) worker.join();
}

ProgressOperation::ProgressOperation(bool is_move, bool is_multi, bool is_upload)
    : _state(std::make_shared<ProgressState>()),
      _is_move(is_move), _is_multi(is_multi), _is_upload(is_upload) {
    _state->Reset();
}

void ProgressOperation::Run(WorkFunc work_func) {
    RunModalProgress(_state, std::move(work_func), [this]() {
        ProgressDialog dlg(*_state, _is_move, _is_multi, _is_upload);
        dlg.Show();
    });
}

DeleteProgressDialog::DeleteProgressDialog(ProgressState& state)
    : _state(state) {
    // 40-char DIF_CENTERTEXT field; box.X2=46, EW=44, W=50, extra_width=6.
    _di.SetBoxTitleItem(Lng(MDeleteBoxTitle));
    _di.SetLine(2);
    _di.AddAtLine(DI_TEXT, 5, 44, DIF_CENTERTEXT, Lng(MDeletingFileOrFolder));
    _di.NextLine();
    _i_filename = _di.AddAtLine(DI_TEXT, 5, 44, DIF_CENTERTEXT, L"");
}

void DeleteProgressDialog::Show() {
    while (!_state.finished) {
        _finished = false;
        // box.X2=46, EW=44 (min_x=3, no buttons at X1=0), extra_width=6 → W=50=46+4.
        BaseDialog::Show(L"MTPProgress", 6, 2, FDLG_REGULARIDLE);
        if (_finished) break;
        if (_state.IsAborting()) break;  // Esc path already aborted.
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

DeleteOperation::DeleteOperation()
    : _state(std::make_shared<ProgressState>()) {
    _state->Reset();
}

void DeleteOperation::Run(WorkFunc work_func) {
    RunModalProgress(_state, std::move(work_func), [this]() {
        DeleteProgressDialog dlg(*_state);
        dlg.Show();
    });
}

bool MTPDialogs::AskCopyMove(bool is_move, bool is_upload, std::string& destination,
                             const std::string& source_name, int item_count) {
    // Direction-aware title (parity with ADB): separate "Rename" lives at Shift+F6, so plain F6 says "Move", not "Rename/Move".
    const wchar_t* title = is_move
        ? (is_upload ? Lng(MMoveToDevice) : Lng(MMoveFromDevice))
        : (is_upload ? Lng(MCopyToDevice) : Lng(MCopyFromDevice));
    std::wstring prompt;
    std::string default_path = destination;

    const std::wstring verb = is_move ? Lng(MPromptMoveTitle) : Lng(MPromptCopyTitle);
    const std::wstring to_colon = std::wstring(L" ") + Lng(MPromptToColon);
    if (item_count > 1) {
        prompt = verb + L" " + std::to_wstring(item_count) + L" "
               + Lng(MPromptItems) + to_colon;
    } else if (!source_name.empty()) {
        prompt = verb + L" " + StrMB2Wide(source_name) + to_colon;
    } else {
        prompt = verb + to_colon;
    }

    std::string other_panel_path;
    if (!is_upload) {
        int size = g_Info.Control(PANEL_PASSIVE, FCTL_GETPANELDIR, 0, (LONG_PTR)0);
        if (size > 0) {
            wchar_t* buffer = new wchar_t[size];
            int result = g_Info.Control(PANEL_PASSIVE, FCTL_GETPANELDIR, size, (LONG_PTR)buffer);
            other_panel_path = result ? StrWide2MB(buffer) : default_path;
            delete[] buffer;
        } else {
            other_panel_path = default_path;
        }
    } else {
        other_panel_path = default_path;
    }

    return AskInput(title, prompt.c_str(), L"MTP_CopyMove", destination,
                    other_panel_path.empty() ? default_path : other_panel_path);
}

bool MTPDialogs::AskCreateDirectory(std::string& dir_name) {
    if (!AskInput(L"Create directory", L"Enter name of directory to create:",
                  L"MTP_MakeDir", dir_name, dir_name)) {
        return false;
    }
    StrTrim(dir_name, " \t\r\n");
    if (dir_name.empty() || dir_name == "." || dir_name == "..") return false;
    return true;
}

bool MTPDialogs::AskInput(const wchar_t* title, const wchar_t* prompt,
                          const wchar_t* history_name, std::string& input,
                          const std::string& default_value) {
    wchar_t input_buffer[4096] = {0};
    if (!default_value.empty()) {
        std::wstring wval = StrMB2Wide(default_value);
        wcsncpy(input_buffer, wval.c_str(), ARRAYSIZE(input_buffer) - 1);
    }
    std::wstring src_text_wstr = default_value.empty() ? L"" : StrMB2Wide(default_value);
    const wchar_t* src_text = src_text_wstr.empty() ? nullptr : src_text_wstr.c_str();

    bool result = g_Info.InputBox(title, prompt, history_name, src_text, input_buffer,
                                  ARRAYSIZE(input_buffer) - 1, nullptr, FIB_BUTTONS | FIB_NOUSELASTHISTORY);
    if (result) input = StrWide2MB(input_buffer);
    return result && !input.empty();
}

bool MTPDialogs::AskConfirmation(const wchar_t* title, const wchar_t* message) {
    const wchar_t* msg[] = { title, message, L"OK", L"Cancel" };
    int result = g_Info.Message(g_Info.ModuleNumber, FMSG_MB_YESNO, nullptr, msg, ARRAYSIZE(msg), 0);
    return (result == 0);
}

bool MTPDialogs::AskWarning(const wchar_t* title, const wchar_t* message) {
    const wchar_t* msg[] = { title, message, L"OK", L"Cancel" };
    int result = g_Info.Message(g_Info.ModuleNumber, FMSG_WARNING | FMSG_MB_YESNO, nullptr, msg, ARRAYSIZE(msg), 0);
    return (result == 0);
}

int MTPDialogs::MessageWrapped(unsigned int flags,
                               const std::wstring& title,
                               const std::wstring& body,
                               size_t wrap) {
    std::vector<std::wstring> lines;
    lines.push_back(title);
    if (body.empty()) {
        lines.push_back(Lng(MUnknownError));
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
