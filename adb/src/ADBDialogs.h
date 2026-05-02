#pragma once

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

// ============================================================================
// FarDialogItems - Dialog item container
// ============================================================================

struct FarDialogItems : std::vector<struct FarDialogItem>
{
    FarDialogItems();

    int SetBoxTitleItem(const wchar_t *title);

    int Add(int type, int x1, int y1, int x2, int y2, unsigned int flags = 0, const wchar_t *data = nullptr);
    int Add(int type, int x1, int y1, int x2, int y2, unsigned int flags, const char *data);

    int EstimateWidth() const;
    int EstimateHeight() const;

    const wchar_t *MB2WidePooled(const char *sz);
    const wchar_t *WidePooled(const wchar_t *wz);

private:
    int AddInternal(int type, int x1, int y1, int x2, int y2, unsigned int flags, const wchar_t *data);

    std::set<std::wstring> _str_pool;
    std::wstring _str_pool_tmp;
};

// ============================================================================
// FarDialogItemsLineGrouped - Auto line increment
// ============================================================================

struct FarDialogItemsLineGrouped : FarDialogItems
{
    void SetLine(int y);
    void NextLine();

    int AddAtLine(int type, int x1, int x2, unsigned int flags = 0, const char *data = nullptr);
    int AddAtLine(int type, int x1, int x2, unsigned int flags, const wchar_t *data);

private:
    int _y = 1;
};

// ============================================================================
// BaseDialog - Base class for dialogs
// ============================================================================

class BaseDialog
{
    HANDLE _dlg = INVALID_HANDLE_VALUE;
    wchar_t _progress_bg = 0, _progress_fg = 0;

protected:
    FarDialogItemsLineGrouped _di;

    static LONG_PTR sSendDlgMessage(HANDLE dlg, int msg, int param1, LONG_PTR param2);
    static LONG_PTR WINAPI sDlgProc(HANDLE dlg, int msg, int param1, LONG_PTR param2);

    LONG_PTR SendDlgMessage(int msg, int param1, LONG_PTR param2);
    virtual LONG_PTR DlgProc(int msg, int param1, LONG_PTR param2);

    int Show(const wchar_t *help_topic, int extra_width, int extra_height, unsigned int flags = 0);

    void Close(int code = -1);

    void SetDefaultDialogControl(int ctl = -1);
    void SetFocusedDialogControl(int ctl = -1);

    void TextToDialogControl(int ctl, const std::wstring &str);
    void TextToDialogControl(int ctl, const char *str);

    void ProgressBarToDialogControl(int ctl, int percents = -1);

public:
    virtual ~BaseDialog();
};

// ============================================================================
// ProgressState - Shared state for progress tracking (thread-safe)
// ============================================================================

struct ProgressState
{
    // Use atomics for flags and numeric values (fork-safe, no mutex needed)
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

    // Mutex only for string access (strings are complex, need protection)
    mutable std::mutex mtx_strings;
    std::wstring current_file;
    std::wstring source_path;
    std::wstring dest_path;

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
            std::lock_guard<std::mutex> lock(mtx_strings);
            current_file.clear();
            source_path.clear();
            dest_path.clear();
        }
        start_time = std::chrono::steady_clock::now();
    }

    bool IsAborting() const { return aborting.load(); }
    bool IsFinished() const { return finished.load(); }

    void SetAborting() { aborting = true; }
    void SetFinished() { finished = true; }

    bool ShouldAbort() const { return aborting.load(); }
};

// ============================================================================
// AbortConfirmDialog - Confirmation dialog for aborting
// ============================================================================

class AbortConfirmDialog : protected BaseDialog
{
public:
    AbortConfirmDialog();
    bool Ask();

protected:
    LONG_PTR DlgProc(int msg, int param1, LONG_PTR param2) override;

private:
    int _i_confirm, _i_cancel;
};

// ============================================================================
// OverwriteDialog - Confirmation dialog for file overwrite
// ============================================================================

class OverwriteDialog : protected BaseDialog
{
public:
    enum Result { OVERWRITE = 0, SKIP = 1, CANCEL = 2, OVERWRITE_ALL = 3, SKIP_ALL = 4 };

    OverwriteDialog(const std::wstring &filename, bool is_multiple, bool is_directory);
    Result Ask();

protected:
    LONG_PTR DlgProc(int msg, int param1, LONG_PTR param2) override;

private:
    int _i_overwrite, _i_skip, _i_cancel, _i_overwrite_all, _i_skip_all;
    bool _is_multiple;
};

// ============================================================================
// ProgressDialog - Progress dialog for operations
// ============================================================================

class ProgressDialog : protected BaseDialog
{
public:
    ProgressDialog(ProgressState &state, const std::wstring &title, bool is_multi = false);
    void Show();

protected:
    LONG_PTR DlgProc(int msg, int param1, LONG_PTR param2) override;

private:
    ProgressState &_state;
    std::wstring _title;
    bool _is_multi;
    bool _finished = false;
    bool _first_update = true;

    // Dialog control indices (assigned during InitLayout)
    int _i_title, _i_operation_label, _i_from_path, _i_to_path;
    int _i_total_bytes, _i_progress_bar, _i_percent, _i_files_processed;
    int _i_time, _i_cancel;
    // Multi-mode only: aggregate-bytes bar + percent. -1 in single mode.
    int _i_total_bar = -1, _i_total_pct = -1;

    // Cached values for change detection
    uint64_t _last_complete = 0, _last_total = 0, _last_count = 0;
    int _last_file_percent = -1;  // Track file percentage changes
    int _last_total_percent = -1; // Track aggregate percentage changes
    std::wstring _last_from, _last_to;

    // Speed calculation
    uint64_t _prev_bytes = 0, _speed = 0;
    std::chrono::steady_clock::time_point _prev_ts;

    void InitLayout(const std::wstring &title);
    void UpdateDialog();
    bool ShowAbortConfirmation();
};

// Mirrors far2l's native shell-delete UI: small centered modal with
// "Deleting the file or folder" + the current item name, debounced.
class DeleteProgressDialog : protected BaseDialog {
public:
    DeleteProgressDialog(ProgressState& state);
    void Show();
protected:
    LONG_PTR DlgProc(int msg, int param1, LONG_PTR param2) override;
private:
    ProgressState& _state;
    bool _finished = false;
    int _i_filename = -1;
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

// ============================================================================
// ADBDialogs - Dialog utility class
// ============================================================================

class ADBDialogs
{
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
        for (auto& s : storage)
            ptrs.push_back(s.c_str());
        return g_Info.Message(g_Info.ModuleNumber, flags, nullptr, ptrs.data(), (int)ptrs.size(), 0);
    }

    // Title + body where body is wrapped to fit. Long shell-error
    // messages can exceed the screen width otherwise.
    static int MessageWrapped(unsigned int flags,
                              const std::wstring& title,
                              const std::wstring& body,
                              size_t wrap = 70);
};

// ============================================================================
// ProgressOperation - Helper for running operations with progress
// ============================================================================

class ProgressOperation
{
public:
    using WorkFunc = std::function<void(ProgressState&)>;

    // is_multi=true adds an aggregate-bytes bar under the per-file bar
    // (matching MTP / native far2l layout). For single-item ops both
    // bars would just mirror each other so the second one is omitted.
    ProgressOperation(const std::wstring& title, bool is_multi = false);
    ~ProgressOperation();

    void Run(WorkFunc work_func);
    bool WasAborted() const { return _state->IsAborting(); }
    ProgressState& GetState() { return *_state; }

private:
    std::shared_ptr<ProgressState> _state;
    std::wstring _title;
    bool _is_multi;
    std::thread _worker_thread;
    static constexpr int JOIN_TIMEOUT_MS = 5000; // 5 second timeout for thread join
};
