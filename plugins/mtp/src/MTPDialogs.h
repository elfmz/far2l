#pragma once

// Ported 1:1 from adb/src/ADBDialogs.h (renames + MTP_* histories).

#include <string>
#include <vector>
#include <cstdint>
#include <memory>
#include <set>
#include <chrono>
#include <mutex>
#include <atomic>
#include <thread>
#include <functional>
#include <windows.h>
#include <farplug-wide.h>
#include <utils.h>

extern PluginStartupInfo g_Info;
extern FarStandardFunctions g_FSF;

struct FarDialogItems : std::vector<struct FarDialogItem> {
    FarDialogItems();
    int SetBoxTitleItem(const wchar_t* title);
    int Add(int type, int x1, int y1, int x2, int y2, unsigned int flags = 0, const wchar_t* data = nullptr);
    int Add(int type, int x1, int y1, int x2, int y2, unsigned int flags, const char* data);
    int EstimateWidth() const;
    int EstimateHeight() const;
    const wchar_t* MB2WidePooled(const char* sz);
    const wchar_t* WidePooled(const wchar_t* wz);
private:
    int AddInternal(int type, int x1, int y1, int x2, int y2, unsigned int flags, const wchar_t* data);
    std::set<std::wstring> _str_pool;
    std::wstring _str_pool_tmp;
};

struct FarDialogItemsLineGrouped : FarDialogItems {
    void SetLine(int y);
    void NextLine();
    int AddAtLine(int type, int x1, int x2, unsigned int flags = 0, const char* data = nullptr);
    int AddAtLine(int type, int x1, int x2, unsigned int flags, const wchar_t* data);
private:
    int _y = 1;
};

class BaseDialog {
    HANDLE _dlg = INVALID_HANDLE_VALUE;
    wchar_t _progress_bg = 0, _progress_fg = 0;
protected:
    FarDialogItemsLineGrouped _di;
    static LONG_PTR sSendDlgMessage(HANDLE dlg, int msg, int param1, LONG_PTR param2);
    static LONG_PTR WINAPI sDlgProc(HANDLE dlg, int msg, int param1, LONG_PTR param2);
    LONG_PTR SendDlgMessage(int msg, int param1, LONG_PTR param2);
    virtual LONG_PTR DlgProc(int msg, int param1, LONG_PTR param2);
    int Show(const wchar_t* help_topic, int extra_width, int extra_height, unsigned int flags = 0);
    void Close(int code = -1);
    void SetDefaultDialogControl(int ctl = -1);
    void SetFocusedDialogControl(int ctl = -1);
    void TextToDialogControl(int ctl, const std::wstring& str);
    void TextToDialogControl(int ctl, const char* str);
    void ProgressBarToDialogControl(int ctl, int percents = -1);
public:
    virtual ~BaseDialog();
};

struct ProgressState {
    std::atomic<bool> finished{false};
    std::atomic<bool> aborting{false};
    std::atomic<uint64_t> file_complete{0};
    std::atomic<uint64_t> file_total{0};
    std::atomic<uint64_t> all_complete{0};
    std::atomic<uint64_t> all_total{0};
    std::atomic<uint64_t> count_complete{0};
    std::atomic<uint64_t> count_total{0};
    std::atomic<bool> is_directory{false};
    std::chrono::steady_clock::time_point start_time;
    mutable std::mutex mtx_strings;
    std::wstring current_file;
    std::wstring source_path;
    std::wstring dest_path;
    // Fires once on aborting false→true; callers forward to whatever blocks the worker (e.g. libmtp progress trampoline cancel).
    std::function<void()> on_abort;

    void Reset() {
        finished = false;
        aborting = false;
        file_complete = 0;
        file_total = 0;
        all_complete = 0;
        all_total = 0;
        count_complete = 0;
        count_total = 0;
        is_directory = false;
        {
            std::lock_guard<std::mutex> lk(mtx_strings);
            current_file.clear();
            source_path.clear();
            dest_path.clear();
        }
        start_time = std::chrono::steady_clock::now();
        // Note: on_abort is not cleared — caller owns that lifetime.
    }
    bool IsAborting() const { return aborting.load(); }
    bool IsFinished() const { return finished.load(); }
    void SetAborting() {
        if (!aborting.exchange(true) && on_abort) on_abort();
    }
    void SetFinished() { finished = true; }
    bool ShouldAbort() const { return aborting.load(); }
};

class AbortConfirmDialog : protected BaseDialog {
public:
    AbortConfirmDialog();
    bool Ask();
protected:
    LONG_PTR DlgProc(int msg, int param1, LONG_PTR param2) override;
private:
    int _i_confirm, _i_cancel;
};

// Mirrors far2l's WarnCopyDlg layout (copy.cpp:3017) minus Append/Resume/Rename (don't apply to MTP transfers).
class OverwriteDialog : protected BaseDialog {
public:
    enum Result { OVERWRITE = 0, SKIP = 1, CANCEL = 2,
                  OVERWRITE_ALL = 3, SKIP_ALL = 4,
                  ONLY_NEWER = 5, ONLY_NEWER_ALL = 6,
                  RENAME = 7, RENAME_ALL = 8 };
    using ViewFn = std::function<void()>;
    // view_new / view_existing (optional): clickable rows that open a viewer for each file. Empty → row is inert text.
    OverwriteDialog(const std::wstring& dest_name,
                    uint64_t src_size, int64_t src_mtime,
                    uint64_t dst_size, int64_t dst_mtime,
                    ViewFn view_new = nullptr,
                    ViewFn view_existing = nullptr);
    Result Ask();

    // Per-item batch wrapper: honors+updates sticky-all flags, collapses 9-way Result into 4-way Decision (Newer resolved via mtime compare; Rename returned to caller who picks a free name).
    enum Decision { D_OVERWRITE, D_SKIP, D_CANCEL, D_RENAME };
    static Decision AskSticky(const std::wstring& dest_name,
                              uint64_t src_size, int64_t src_mtime,
                              uint64_t dst_size, int64_t dst_mtime,
                              bool& overwrite_all, bool& skip_all,
                              bool& newer_all, bool& rename_all,
                              ViewFn view_new = nullptr,
                              ViewFn view_existing = nullptr);
protected:
    LONG_PTR DlgProc(int msg, int param1, LONG_PTR param2) override;
private:
    int _i_filename, _i_src_info, _i_dst_info, _i_remember;
    int _i_overwrite, _i_skip, _i_newer, _i_rename, _i_cancel;
    ViewFn _view_new, _view_existing;
};

// Mirrors far2l's CopyProgress::CreateBackground; single-file=9 content rows, multi-file=11 (adds Total bar + separator).
class ProgressDialog : protected BaseDialog {
public:
    ProgressDialog(ProgressState& state, bool is_move, bool is_multi, bool is_upload);
    void Show();
protected:
    LONG_PTR DlgProc(int msg, int param1, LONG_PTR param2) override;
private:
    ProgressState& _state;
    bool _is_move;
    bool _is_multi;
    bool _is_upload;
    bool _finished = false;
    bool _first_update = true;
    // Esc gate: ignore Esc within ~200ms of show (drops buffered keys + F-key-emitted-Esc that would abort instantly).
    std::chrono::steady_clock::time_point _shown_at;
    bool _shown_at_set = false;
    int _i_verb, _i_from_path, _i_to_path;
    int _i_progress_bar, _i_percent;
    int _i_total_bytes = -1, _i_total_bar = -1;  // present only in multi mode
    int _i_files_processed, _i_time;
    uint64_t _last_complete = 0, _last_total = 0, _last_count = 0;
    int _last_file_percent = -1, _last_total_percent = -1;
    std::wstring _last_from, _last_to;
    uint64_t _speed = 0;
    std::chrono::steady_clock::time_point _prev_ts;
    void InitLayout();
    void UpdateDialog();
    bool ShowAbortConfirmation();
};

class MTPDialogs {
public:
    static bool AskCopyMove(bool is_move, bool is_upload, std::string& destination,
                            const std::string& source_name = "", int item_count = 0);
    static bool AskCreateDirectory(std::string& dir_name);
    static bool AskInput(const wchar_t* title, const wchar_t* prompt,
                         const wchar_t* history_name, std::string& input,
                         const std::string& default_value = "");
    static bool AskConfirmation(const wchar_t* title, const wchar_t* message);
    static bool AskWarning(const wchar_t* title, const wchar_t* message);
    template<typename... Args>
    static int Message(unsigned int flags, Args&&... extra_lines) {
        std::vector<std::wstring> storage{ std::wstring(extra_lines)... };
        std::vector<const wchar_t*> ptrs;
        ptrs.reserve(storage.size());
        for (auto& s : storage) ptrs.push_back(s.c_str());
        return g_Info.Message(g_Info.ModuleNumber, flags, nullptr, ptrs.data(), (int)ptrs.size(), 0);
    }

    // Title + body word-wrapped — avoids Far's right-edge truncation on long libmtp error strings.
    static int MessageWrapped(unsigned int flags,
                              const std::wstring& title,
                              const std::wstring& body,
                              size_t wrap = 70);
};

class ProgressOperation {
public:
    using WorkFunc = std::function<void(ProgressState&)>;
    ProgressOperation(bool is_move, bool is_multi, bool is_upload);
    void Run(WorkFunc work_func);
    bool WasAborted() const { return _state->IsAborting(); }
    ProgressState& GetState() { return *_state; }
private:
    std::shared_ptr<ProgressState> _state;
    bool _is_move;
    bool _is_multi;
    bool _is_upload;
};

// Mirrors far2l's shell-delete UI: small centered modal, current item name debounced (repaints only on worker advance).
class DeleteProgressDialog : protected BaseDialog {
public:
    DeleteProgressDialog(ProgressState& state);
    void Show();
protected:
    LONG_PTR DlgProc(int msg, int param1, LONG_PTR param2) override;
private:
    ProgressState& _state;
    bool _finished = false;
    int _i_filename;
    std::wstring _last_filename;
    void UpdateDialog();
    bool ShowAbortConfirmation();
};

class DeleteOperation {
public:
    using WorkFunc = std::function<void(ProgressState&)>;
    DeleteOperation();
    void Run(WorkFunc work_func);
    bool WasAborted() const { return _state->IsAborting(); }
    ProgressState& GetState() { return *_state; }
private:
    std::shared_ptr<ProgressState> _state;
};
