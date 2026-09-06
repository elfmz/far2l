#pragma once

#include <functional>
#include <memory>
#include <mutex>
#include <string>
#include <unordered_map>
#include <vector>

#include "Types.h"

struct LIBMTP_mtpdevice_struct;
struct libusb_context;

namespace proto {

// Process-lifetime libusb ctx (darwin re-init races on hotplug thread).
libusb_context* SharedLibusbContext();

// USB device-descriptor strings; readable while icdd holds interface 0.
struct UsbDescriptorStrings {
    std::string manufacturer;  // iManufacturer (e.g. "SAMSUNG")
    std::string product;       // iProduct (e.g. "SAMSUNG_Android")
    std::string serial;        // iSerialNumber (e.g. "RFCR822QD1N")
};
UsbDescriptorStrings ReadUsbDescriptorStrings(int bus, int address);

class LibMtpBackend {
public:
    explicit LibMtpBackend(UsbDeviceCandidate candidate);
    ~LibMtpBackend();

    LibMtpBackend(const LibMtpBackend&) = delete;
    LibMtpBackend& operator=(const LibMtpBackend&) = delete;

    // Refcounted session: Acquire reuses existing per device-key; Release drops a panel's claim (handle dies w/ last ref).
    static Result<std::vector<UsbDeviceCandidate>> Enumerate();
    static std::shared_ptr<LibMtpBackend> AcquireShared(const UsbDeviceCandidate& candidate);
    static void ReleaseShared(const std::string& device_key);

    std::string DeviceKey() const { return _candidate.id.Key(); }
    std::string FriendlyName() const;

    Status Connect();
    void Disconnect();
    void RefreshCache();
    bool IsReady() const;

    Result<std::vector<StorageEntry>> ListStorages();
    Result<std::vector<ObjectEntry>> ListChildren(StorageId storageId, ObjectHandle parent);
    Result<ObjectEntry> Stat(ObjectHandle handle);

    Status Download(ObjectHandle handle,
                    const std::string& local_path,
                    std::function<void(uint64_t, uint64_t)> progress,
                    const CancellationToken& cancel);

    Status Upload(const std::string& local_path,
                  const std::string& remote_name,
                  StorageId storageId,
                  ObjectHandle parent,
                  std::function<void(uint64_t, uint64_t)> progress,
                  const CancellationToken& cancel);

    // Capability accessors — driven by device's OperationsSupported/SupportedFiletypes, never vendor/product tables.
    bool SupportsCopyObject();
    bool SupportsMoveObject();
    bool SupportsEditObjects();
    bool SupportsPartialObject();
    // LIBMTP_filetype_t values the device accepts in upload calls.
    std::vector<uint16_t> SupportedFiletypes();
    // Pick filetype device accepts for `remote_name`; falls back to "neutral" type (Text/HTML/etc.) on devices rejecting UNKNOWN.
    uint16_t PickUploadFiletype(const std::string& remote_name);

    // True for FAT-class storages — collision checks must use this so "Test.txt"/"test.txt" collide on SD cards.
    bool StorageIsCaseInsensitive(StorageId storageId);
    bool NamesEqual(StorageId storageId, const std::string& a,
                    const std::string& b);

    Status Rename(ObjectHandle handle, const std::string& new_name);
    Status Delete(ObjectHandle handle, bool recursive,
                  std::function<void(const std::string&)> on_progress = nullptr);
    Result<ObjectHandle> MakeDirectory(const std::string& name, StorageId storageId, ObjectHandle parent);
    Result<ObjectHandle> CopyObject(ObjectHandle handle, StorageId storageId, ObjectHandle parent);
    Status MoveObject(ObjectHandle handle, StorageId storageId, ObjectHandle parent);

private:
    UsbDeviceCandidate _candidate;
    LIBMTP_mtpdevice_struct* _device = nullptr;
    mutable std::recursive_mutex _mtx;
    std::string _friendly_name;

    Status DrainErrors(const char* what);
    void ClearErrors();
    // After cancelled USB transfer libmtp's PTP session is desynced (half-committed abort) — tear down + reopen for next call.
    void ReopenAfterCancelLocked();
    Status DeleteRecursiveLocked(ObjectHandle handle,
                                 const std::function<void(const std::string&)>& on_progress);
    Result<std::vector<ObjectEntry>> ListChildrenLocked(StorageId storageId, ObjectHandle parent,
                                                        bool include_tombstones = false);
    Result<ObjectEntry> StatLocked(ObjectHandle handle);

    // Capability cache; populated lazily, cleared on Disconnect.
    bool _caps_probed = false;
    bool _cap_copy_object = false;
    bool _cap_move_object = false;
    bool _cap_edit_objects = false;
    bool _cap_partial_object = false;
    std::vector<uint16_t> _supported_filetypes;
    void EnsureCapsProbedLocked();
    void ClearSessionCachesLocked();

    // Samsung-quirk patch: some firmwares return blank filename for session-created handles — track names client-side.
    std::unordered_map<uint32_t, std::string> _known_names;

    // Per-storage case-insens cache; StorageType stable for session but NamesEqual is hot — without this every call walks storage list.
    std::unordered_map<uint32_t, bool> _storage_case_insens;
};

} // namespace proto
