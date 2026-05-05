#pragma once

#include <atomic>
#include <cstdint>
#include <memory>
#include <string>

namespace proto {

class CancellationToken {
public:
    explicit CancellationToken(std::shared_ptr<std::atomic<bool>> state)
        : _state(std::move(state)) {}
    bool IsCancelled() const { return _state && _state->load(); }
private:
    std::shared_ptr<std::atomic<bool>> _state;
};

class CancellationSource {
public:
    CancellationSource() : _state(std::make_shared<std::atomic<bool>>(false)) {}
    CancellationToken Token() const { return CancellationToken(_state); }
    void Cancel() { _state->store(true); }
private:
    std::shared_ptr<std::atomic<bool>> _state;
};

enum class ErrorCode {
    Ok,
    Busy,
    LockedOrUnauthorized,
    Disconnected,
    Timeout,
    ProtocolDesync,
    Unsupported,
    Io,
    Cancelled,
    NotFound,
    AccessDenied,
    InvalidArgument,
};

template <typename T>
struct Result {
    bool ok = false;
    T value{};
    ErrorCode code = ErrorCode::Io;
    std::string message;
    bool retryable = false;

    static Result<T> Success(T v) {
        Result<T> r;
        r.ok = true;
        r.code = ErrorCode::Ok;
        r.value = std::move(v);
        return r;
    }

    static Result<T> Failure(ErrorCode c, std::string msg, bool canRetry = false) {
        Result<T> r;
        r.ok = false;
        r.code = c;
        r.message = std::move(msg);
        r.retryable = canRetry;
        return r;
    }
};

using Status = Result<bool>;

inline Status OkStatus() {
    return Status::Success(true);
}

struct DeviceId {
    int bus = -1;
    int address = -1;
    int interface_number = -1;

    bool IsValid() const {
        return bus >= 0 && address >= 0 && interface_number >= 0;
    }

    std::string Key() const {
        return std::to_string(bus) + ":" + std::to_string(address) + ":" + std::to_string(interface_number);
    }
};

using StorageId = uint32_t;
using ObjectHandle = uint32_t;

// Storage-root parent sentinel; Samsung wants 0xFFFFFFFF, not 0.
constexpr uint32_t kRootParent = 0xFFFFFFFFu;

struct UsbDeviceCandidate {
    DeviceId id;
    uint16_t vendor_id = 0;
    uint16_t product_id = 0;
    std::string serial;
    std::string manufacturer;
    std::string product;
};

// PTP StorageType (0x0003 FixedRAM / 0x0004 RemovableRAM) → 3-way enum.
enum class StorageKind { Internal, External, Unknown };

struct StorageEntry {
    StorageId id = 0;
    std::string description;  // device-supplied label, e.g. "Internal storage"
    std::string volume;       // device-supplied volume id, often empty
    uint64_t free_bytes = 0;
    uint64_t max_capacity = 0;
    StorageKind kind = StorageKind::Unknown;
};

struct ObjectEntry {
    ObjectHandle handle = 0;
    StorageId storage_id = 0;
    ObjectHandle parent = 0;
    std::string name;
    bool is_dir = false;
    bool is_hidden = false;
    bool is_readonly = false;
    uint64_t size = 0;
    uint64_t mtime_epoch = 0;
    uint64_t ctime_epoch = 0;
    uint16_t format = 0;
    uint16_t protection = 0;
    uint32_t image_width = 0;
    uint32_t image_height = 0;
    std::string summary;
};

} // namespace proto
