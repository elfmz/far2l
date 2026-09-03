#pragma once

#include <map>
#include <memory>
#include <mutex>
#include <set>
#include <string>
#include <unordered_map>
#include <vector>

#include "farplug-wide.h"

#include "LibMtpBackend.h"
#include "ProgressBatch.h"

struct ProgressState;

extern PluginStartupInfo g_Info;
extern FarStandardFunctions g_FSF;

class MTPPlugin {
public:
    MTPPlugin(const wchar_t* path = nullptr, bool path_is_standalone_config = false, int op_mode = 0);
    ~MTPPlugin();

    int GetFindData(PluginPanelItem** panel_items, int* items_number, int op_mode);
    void FreeFindData(PluginPanelItem* panel_items, int items_number);
    void GetOpenPluginInfo(OpenPluginInfo* info);
    int SetDirectory(const wchar_t* dir, int op_mode);
    int ProcessKey(int key, unsigned int control_state);
    int ProcessEvent(int event, void* param);

    int MakeDirectory(const wchar_t** name, int op_mode);
    int DeleteFiles(PluginPanelItem* panel_item, int items_number, int op_mode);
    int GetFiles(PluginPanelItem* panel_item, int items_number, int move, const wchar_t** dest_path, int op_mode);
    int PutFiles(PluginPanelItem* panel_item, int items_number, int move, const wchar_t* src_path, int op_mode);
    int Execute(PluginPanelItem* panel_item, int items_number, int op_mode);

    static PluginStartupInfo* GetInfo();

private:
    // Live-instance registry — used to type-check cross-panel handles before casting (Drives-menu "Other panel" path adoption + cross-panel copy/move). std::set instead of unordered_set: pointer hash isn't worth the larger node size for this small set.
    static std::set<MTPPlugin*> s_live_instances;
    static std::mutex s_live_mtx;
    // Returns the MTPPlugin instance running on `panel` (PANEL_ACTIVE / PANEL_PASSIVE), or nullptr if that panel isn't an MTP plugin (or no plugin at all).
    static MTPPlugin* PluginFromPanel(HANDLE panel);

    enum class ViewMode {
        Devices,
        Storages,
        Objects,
    };

    struct DeviceBind {
        proto::UsbDeviceCandidate candidate;
    };

    PluginPanelItem MakePanelItem(const std::string& name,
                                  bool is_dir,
                                  uint64_t size,
                                  uint64_t mtime_epoch,
                                  uint32_t object_id,
                                  uint32_t storage_id,
                                  uint32_t packed_device_id,
                                  const std::string& description = std::string(),
                                  uint64_t ctime_epoch = 0,
                                  uint32_t file_attributes = 0,
                                  uint32_t unix_mode = 0) const;
    PluginPanelItem MakeObjectPanelItem(const proto::ObjectEntry& entry,
                                        uint32_t packed_device_id) const;
    static std::string BuildObjectDescription(const proto::ObjectEntry& entry);
    void UpdateObjectsPanelTitle();

    bool GetSelectedPanelUserData(std::string& out) const;
    bool GetSelectedPanelFileName(std::string& out) const;
    bool EnsureConnected(const std::string& device_key);

    int ListDevices(PluginPanelItem** panel_items, int* items_number);
    int ListStorages(PluginPanelItem** panel_items, int* items_number);
    int ListObjects(PluginPanelItem** panel_items, int* items_number);

    bool ParseStorageToken(const std::string& token, uint32_t& storage_id) const;
    bool ParseObjectToken(const std::string& token, uint32_t& handle) const;
    bool ResolvePanelToken(const PluginPanelItem& item, std::string& token) const;
    bool EnterByToken(const std::string& token);

    // Collects handles for active panel's selection, falling back to current item; skips "."/".."; first_name receives 1st resolved item name (UI hint).
    std::vector<uint32_t> CollectActivePanelHandles(std::string* first_name = nullptr);

    int MapErrorToErrno(const proto::Status& st) const;
    int MapErrorToErrno(proto::ErrorCode code) const;
    void SetErrorFromStatus(const proto::Status& st) const;
    bool PromptInput(const wchar_t* title,
                     const wchar_t* prompt,
                     const wchar_t* history_name,
                     const std::string& initial_value,
                     std::string& out) const;
    void RefreshPanel() const;
    bool RenameSelectedItem();
    bool EnterSelectedItem();
    bool CrossPanelCopyMoveSameDevice(bool move);
    // Drives-menu "Other panel" support: a fresh plugin instance receives SetDirectory("/Storage/Path") with no device context. Inherits the device from the passive panel (when it's another live MTPPlugin) and walks the path. Returns true on success, false if any segment doesn't resolve.
    bool AdoptSiblingAndWalkPath(const std::string& abs_path);
    // Shift+F5 in-place copy: single-select prompts new name, multi auto-suffixes ".copy"; falls back host-mediated on device refusal.
    bool ShiftF5CopyInPlace();

    // Recursive size for Overwrite-dialog folder column (libmtp returns 0); depth-bounded, returns partial sum on early out.
    uint64_t ComputeRecursiveSize(uint32_t storage_id, uint32_t handle,
                                  int depth = 0);

    // Same walk + file-count, used by RunBatch unit construction so dialog's "X of Y" counter advances per recursive child.
    struct RecursiveStats { uint64_t size = 0; uint64_t count = 0; };
    RecursiveStats ComputeRecursiveStats(uint32_t storage_id, uint32_t handle,
                                         int depth = 0);

    // Resolve user-typed path ("../foo", "Internal/Temp/x") into (storage_id, parent, basename); ok=false on unresolvable.
    struct PathResolution {
        bool ok = false;
        uint32_t storage_id = 0;
        uint32_t parent = 0;
        std::string basename;
    };
    // Resolution base — storage + parent + storage_name; defaults to active panel, cross-panel F5/F6 supplies the dst panel's values.
    struct ResolveBase {
        uint32_t storage_id;
        uint32_t parent;
        std::string storage_name;
        ResolveBase() : storage_id(0), parent(0) {}
        ResolveBase(uint32_t s, uint32_t p, std::string n)
            : storage_id(s), parent(p), storage_name(std::move(n)) {}
    };
    PathResolution ResolveNewNamePath(const std::string& input,
                                      const std::string& src_name = std::string(),
                                      bool auto_create_dirs = true,
                                      const ResolveBase& base = ResolveBase());

    // Case-fold scan on FAT-class storages (catches "Test.txt"/"test.txt"); strict find on ext4.
    const proto::ObjectEntry* FindExistingByName(
        const std::map<std::string, proto::ObjectEntry>& m,
        const std::string& name,
        uint32_t storage_id) const;

    // Lowest-N suffix not present in `taken` (collision-aware via NamesEqual on FAT). "ucl" → "ucl(2)" → "ucl(3)" ... preserving extension: "a.txt" → "a(2).txt".
    std::string FindFreeName(const std::map<std::string, proto::ObjectEntry>& taken,
                             const std::string& base, uint32_t storage_id) const;

    // True if user-typed F5/F6 destination should route to the in-device copy/move engine instead of host xfer. Catches dot-prefixed paths, storage-rooted paths (with optional leading slash), and bare names (treated as device targets to be auto-created).
    bool IsInDevicePathSyntax(const std::string& s) const;

    // OverwriteDialog click-to-view callbacks. Host: open the path with far2l Viewer. Device: synchronously download to a per-pid temp, chmod 0644, open with VF_DELETEONCLOSE so the temp self-deletes when the viewer exits. Files of any size — caller accepts the wait when they click.
    std::function<void()> ViewHostFileCallback(const std::string& path) const;
    std::function<void()> ViewDeviceFileCallback(uint32_t handle, const std::string& display_name);

    // "<storage>/<dir>/<dir>/" prefix for ShiftF5/F6 prompt prefills.
    std::string BuildPanelRelativePath() const;

    // Folder-only variant of ResolveNewNamePath (no basename) for F5/F6 destination prompts.
    struct DirResolution {
        bool ok = false;
        uint32_t storage_id = 0;
        uint32_t parent = 0;
    };
    DirResolution ResolveDestinationFolder(const std::string& input);

    // Device-side primitives + host fallback for items targeting in-device folder; reached when F5/F6 destination resolves on-device.
    int InDeviceCopyMoveTo(PluginPanelItem* panel_item, int items_number,
                           bool move, uint32_t dst_storage, uint32_t dst_parent,
                           const std::string& dst_label = std::string());

    // Recursive download; tr (non-null) gets per-file Tick + final CompleteFile for RunBatch accounting; nullptr = fire-and-forget (silent OPM_VIEW).
    proto::Status DownloadRecursive(uint32_t storage_id,
                                    uint32_t handle,
                                    const std::string& local_root,
                                    proto::CancellationToken token,
                                    class ProgressTracker* tr);

    proto::Status UploadRecursive(const std::string& local_dir,
                                  const std::string& remote_name,
                                  uint32_t storage_id,
                                  uint32_t parent,
                                  proto::CancellationToken token,
                                  proto::CancellationSource* cancel_src,
                                  class ProgressTracker* tr);

    // Host-staged copy fallback (download → upload) for devices refusing Copy_Object; owns its progress dialog.
    proto::Status ManualCopyViaHost(uint32_t src_handle,
                                    const proto::ObjectEntry& src_stat,
                                    uint32_t dst_storage_id,
                                    uint32_t dst_parent,
                                    const std::string& new_name);

    // Inline variant: caller-supplied tracker/cancel so batch shares one progress dialog instead of one-modal-per-item.
    proto::Status ManualCopyViaHostInline(uint32_t src_handle,
                                          const proto::ObjectEntry& src_stat,
                                          uint32_t dst_storage_id,
                                          uint32_t dst_parent,
                                          const std::string& new_name,
                                          class ProgressTracker& tr,
                                          proto::CancellationToken token,
                                          proto::CancellationSource* cancel_src);

    // Cross-panel host-fallback batch when CopyObject/MoveObject refused; move=true deletes source after each successful host-staged copy.
    struct HostFallbackItem {
        uint32_t src_handle;
        proto::ObjectEntry src_stat;
        uint32_t dst_storage_id;
        uint32_t dst_parent;
        std::string dst_name;
        // Pre-computed recursive size + file count from caller's walk; 0/0 = "not computed yet", RunHostFallbackBatch fills on demand.
        uint64_t total_bytes = 0;
        uint64_t total_files = 0;
    };
    proto::Status RunHostFallbackBatch(const std::vector<HostFallbackItem>& items,
                                       bool move,
                                       size_t* out_ok_count = nullptr);

    // Per-item state for device-side CopyObject/MoveObject batch (InDeviceCopyMoveTo + CrossPanelCopyMoveSameDevice); rstats computed once in pre-scan, reused for collision dialog + unit-build.
    struct DeviceCopyItem {
        uint32_t handle;
        proto::ObjectEntry stat;
        RecursiveStats rstats;  // pre-computed
        std::string target_name;  // empty = use stat.name; populated when user picked Rename to disambiguate collision.
        bool skip = false;
        bool overwrite = false;
    };
    // Cross-call mutable state captured by device-batch lambda — pointers only, no &local refs, so a future async RunBatch can't UAF.
    struct DeviceCopyCtx {
        MTPPlugin* dst;  // dst panel; equals this for in-device flow
        std::vector<HostFallbackItem>* fallback;
        proto::Status* hard_err;
        std::map<std::string, proto::ObjectEntry>* dst_existing;
        bool move;
        uint32_t dst_storage;
        uint32_t dst_parent;
    };
    WorkUnit MakeDeviceCopyMoveUnit(const DeviceCopyItem& it,
                                    const RecursiveStats& stats,
                                    const DeviceCopyCtx& ctx);

    std::wstring _standalone_config;
    std::wstring _panel_title;
    // Backing for OpenPluginInfo::CurDir; pointer must outlive the call.
    mutable std::wstring _cur_dir_buf;

    ViewMode _view_mode = ViewMode::Devices;
    std::string _current_device_key;
    uint32_t _current_storage_id = 0;
    uint32_t _current_parent = 0;
    std::string _current_device_name;     // friendly: libmtp description, fallback product
    std::string _current_device_serial;   // USB iSerialNumber — primary device ID
    std::string _current_storage_name;    // panel label: "Internal" / "External" / "Storage"
    std::vector<std::string> _dir_stack;

    std::unordered_map<std::string, DeviceBind> _device_binds;
    std::unordered_map<std::string, std::string> _name_token_index;

    // Shared registry-managed backend — second panel reuses the libmtp session instead of re-claiming the USB interface.
    std::shared_ptr<proto::LibMtpBackend> _backend;

    mutable std::string _last_error_message;
    std::wstring _last_made_dir;
};
