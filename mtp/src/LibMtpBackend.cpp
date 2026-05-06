#include "LibMtpBackend.h"

#include <cerrno>
#include <chrono>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <mutex>
#include <optional>
#include <signal.h>
#include <string>
#include <sys/stat.h>
#include <unistd.h>
#include <unordered_map>
#include <vector>

#include <libmtp.h>
#include <libusb.h>
#include "ptp.h"

#if defined(__APPLE__)
#include <CoreFoundation/CoreFoundation.h>
#include <sys/sysctl.h>
#endif

#include "MTPLog.h"

namespace proto {

namespace {

// ASCII-only fold for filename/extension compares — Unicode case folding would shift byte counts, unneeded here.
std::string ToLowerASCII(std::string s) {
    for (auto& c : s) c = (char)tolower((unsigned char)c);
    return s;
}

// PTP datetime "YYYYMMDDTHHmmSS" → time_t. Duplicates static ptp_unpack_PTPTIME in ptp-pack.c (text-included by ptp.c, not linkable from here).
time_t ParsePTPTime(const char* str) {
    if (!str || strlen(str) < 15) return 0;
    struct tm tm = {};
    if (sscanf(str, "%4d%2d%2dT%2d%2d%2d",
               &tm.tm_year, &tm.tm_mon, &tm.tm_mday,
               &tm.tm_hour, &tm.tm_min, &tm.tm_sec) != 6) return 0;
    tm.tm_year -= 1900;
    tm.tm_mon  -= 1;
    tm.tm_isdst = -1;
    return mktime(&tm);
}

// Samsung quirk: in-session-created objects come back with empty PTP_OPC_ObjectFileName until reconnect; we cache names ourselves at create time.
void PatchKnownName(ObjectEntry& e, const std::unordered_map<uint32_t, std::string>& known_names) {
    if (!e.name.empty()) return;
    auto it = known_names.find(e.handle);
    if (it != known_names.end()) e.name = it->second;
}

// One-shot bulk GetObjectPropList for all children of `parent` (MTP 0x9805 depth=1). Returns nullopt on unsupported/error so the caller falls back.
std::optional<std::vector<ObjectEntry>> ListChildrenFast(
    LIBMTP_mtpdevice_t* device,
    uint32_t storage_id,
    uint32_t parent,
    const std::unordered_map<uint32_t, std::string>& known_names)
{
    PTPParams* params = static_cast<PTPParams*>(device->params);

    if ((params->device_flags & DEVICE_FLAG_BROKEN_MTPGETOBJPROPLIST) ||
        !ptp_operation_issupported(params, PTP_OC_MTP_GetObjPropList)) {
        DBG("[ListChildrenFast] fallback: GetObjPropList unsupported or flagged broken\n");
        return std::nullopt;
    }

    constexpr uint32_t kAllFormats = 0;
    constexpr uint32_t kAllProperties = 0xFFFFFFFFU;
    constexpr uint32_t kNoGroup = 0;
    constexpr uint32_t kImmediateChildren = 1;

    const uint32_t handle = (parent == LIBMTP_FILES_AND_FOLDERS_ROOT) ? 0 : parent;

    struct PropBuffer { MTPProperties* raw; int n; };
    std::vector<PropBuffer> bufs;
    auto release_bufs = [&]() { for (auto& b : bufs) free(b.raw); bufs.clear(); };

    // Storage root: Samsung accepts wildcard properties at handle=0 and returns the whole storage in one round-trip (we filter to direct children below). Specific parent handles reject the same form with 0xA801 after a long device-side delay — don't probe.
    if (handle == 0) {
        PropBuffer b{nullptr, 0};
        uint16_t rc = ptp_mtp_getobjectproplist_generic(
            params, handle, kAllFormats, kAllProperties, kNoGroup, kImmediateChildren, &b.raw, &b.n);
        if (rc == PTP_RC_OK && b.n > 0) {
            bufs.push_back(b);
        } else {
            free(b.raw);
            DBG("[ListChildrenFast] storage-root wildcard rc=0x%x, per-property fallback\n", rc);
        }
    }

    // Per-property fallback: one depth=1 call per ObjectPropCode (constant calls vs slow path's 1+N). Skipped: StorageID/ParentObject (knowns from caller), DateCreated (plugin falls back to mtime).
    if (bufs.empty()) {
        static const uint32_t kWantedProps[] = {
            PTP_OPC_ObjectFileName,
            PTP_OPC_ObjectFormat,
            PTP_OPC_ProtectionStatus,
            PTP_OPC_ObjectSize,
            PTP_OPC_DateModified,
        };
        bufs.reserve(sizeof(kWantedProps) / sizeof(kWantedProps[0]));
        for (uint32_t opc : kWantedProps) {
            PropBuffer b{nullptr, 0};
            uint16_t prc = ptp_mtp_getobjectproplist_generic(
                params, handle, kAllFormats, opc, kNoGroup, kImmediateChildren, &b.raw, &b.n);
            if (prc != PTP_RC_OK) {
                DBG("[ListChildrenFast] opc=0x%x rc=0x%x → fallback\n", opc, prc);
                free(b.raw);
                release_bufs();
                return std::nullopt;
            }
            bufs.push_back(b);
        }
    }

    // Switch handles the union of all OPCs (wildcard storage-root response carries every property); per-property path triggers only the 5 cases in kWantedProps.
    std::unordered_map<uint32_t, ObjectEntry> by_handle;
    if (!bufs.empty()) by_handle.reserve(static_cast<size_t>(bufs.front().n));
    for (const auto& buf : bufs) {
        for (int i = 0; i < buf.n; ++i) {
            const MTPProperties& p = buf.raw[i];
            const uint32_t h = p.ObjectHandle;
            if (h == parent) continue;  // depth=1 includes the parent itself
            auto [it, inserted] = by_handle.try_emplace(h);
            ObjectEntry& e = it->second;
            if (inserted) {
                e.handle = h;
                e.storage_id = storage_id;
                e.parent = parent;
            }
            switch (p.property) {
                case PTP_OPC_StorageID:        e.storage_id  = p.propval.u32; break;
                case PTP_OPC_ObjectFormat:
                    e.format = p.propval.u16;
                    e.is_dir = (p.propval.u16 == PTP_OFC_Association);
                    break;
                case PTP_OPC_ProtectionStatus: e.is_readonly = (p.propval.u16 != 0); break;
                case PTP_OPC_ObjectSize:
                    e.size = (p.datatype == PTP_DTC_UINT64) ? p.propval.u64 : p.propval.u32;
                    break;
                case PTP_OPC_ObjectFileName:
                    if (p.propval.str) e.name = p.propval.str;
                    break;
                case PTP_OPC_DateCreated:
                    if (p.propval.str) e.ctime_epoch = static_cast<uint64_t>(ParsePTPTime(p.propval.str));
                    break;
                case PTP_OPC_DateModified:
                    if (p.propval.str) e.mtime_epoch = static_cast<uint64_t>(ParsePTPTime(p.propval.str));
                    break;
                case PTP_OPC_ParentObject:
                    e.parent = (p.propval.u32 == 0) ? kRootParent : p.propval.u32;
                    break;
            }
        }
    }
    release_bufs();

    std::vector<ObjectEntry> out;
    out.reserve(by_handle.size());
    for (auto& kv : by_handle) {
        ObjectEntry& e = kv.second;
        if (e.parent != parent) continue;  // storage-root wildcard returns deeper descendants too
        PatchKnownName(e, known_names);
        out.push_back(std::move(e));
    }
    return out;
}

void EnsureLibmtpInit() {
    static std::once_flag once;
    std::call_once(once, []() {
        LIBMTP_Init();

        // Default mask: Debug → USB+PTP, Release → off. MTP_LIBMTP_DEBUG=all|data|usb|ptp overrides and forces the stderr redirect.
        int mask = 0;
#if defined(DEBUG) || defined(_DEBUG)
        mask = LIBMTP_DEBUG_USB | LIBMTP_DEBUG_PTP;
#endif
        if (const char* lvl = std::getenv("MTP_LIBMTP_DEBUG")) {
            mask = 0;
            if (std::strstr(lvl, "usb")) mask |= LIBMTP_DEBUG_USB;
            if (std::strstr(lvl, "ptp")) mask |= LIBMTP_DEBUG_PTP;
            if (std::strstr(lvl, "data")) mask |= LIBMTP_DEBUG_DATA;
            if (std::strstr(lvl, "all") || mask == 0) mask = LIBMTP_DEBUG_ALL;
        }
        LIBMTP_Set_Debug(mask);
        if (mask == 0) return;

        // libmtp prints to stderr — redirect alongside mtp.log so traces don't pollute the host TTY.
        const char *plugdir = MTPLog_GetPluginDir();
        std::string libmtp_log = (*plugdir ? std::string(plugdir) + "/mtp_libmtp.log"
                                           : "/tmp/mtp_libmtp.log");
        freopen(libmtp_log.c_str(), "a", stderr);
        std::fprintf(stderr, "\n=== mtp plugin init at pid=%d ===\n", getpid());
        std::fflush(stderr);
        DBG("LIBMTP_Set_Debug mask=0x%x stderr -> %s\n", mask, libmtp_log.c_str());
    });
}

ErrorCode MapLibmtpError(LIBMTP_error_number_t e) {
    switch (e) {
        case LIBMTP_ERROR_NO_DEVICE_ATTACHED:
        case LIBMTP_ERROR_USB_LAYER:
            return ErrorCode::Disconnected;
        case LIBMTP_ERROR_CANCELLED:
            return ErrorCode::Cancelled;
        case LIBMTP_ERROR_MEMORY_ALLOCATION:
            return ErrorCode::Io;
        case LIBMTP_ERROR_NONE:
            return ErrorCode::Ok;
        default:
            return ErrorCode::Io;
    }
}

bool IsFolder(LIBMTP_filetype_t t) {
    return t == LIBMTP_FILETYPE_FOLDER;
}

ObjectEntry FromMtpFile(const LIBMTP_file_t* f) {
    ObjectEntry e;
    e.handle = f->item_id;
    e.storage_id = f->storage_id;
    e.parent = (f->parent_id == 0) ? kRootParent : f->parent_id;
    e.name = f->filename ? f->filename : "";
    e.is_dir = IsFolder(f->filetype);
    e.size = f->filesize;
    e.mtime_epoch = static_cast<uint64_t>(f->modificationdate);
    // f->filetype is libmtp's enum, not the PTP ObjectFormatCode ListChildrenFast/FormatCodeName speak — leave format=0 ("unknown") instead of feeding mismatched values.
    e.format = 0;
    return e;
}

struct ProgressCtx {
    std::function<void(uint64_t, uint64_t)> cb;
    const CancellationToken* cancel;
};

int ProgressTrampoline(uint64_t sent, uint64_t total, void const* data) {
    auto* ctx = static_cast<const ProgressCtx*>(data);
    if (ctx->cancel && ctx->cancel->IsCancelled()) {
        return -1;
    }
    if (ctx->cb) {
        ctx->cb(sent, total);
    }
    return 0;
}

bool StatFileSize(const std::string& path, uint64_t& out) {
    struct stat st{};
    if (::stat(path.c_str(), &st) != 0) return false;
    if (st.st_size < 0) return false;
    out = static_cast<uint64_t>(st.st_size);
    return true;
}

// USB SET_CONFIGURATION resets kernel-side claims (icdd/gvfs-mtp drop exclusive); ~300ms settle. MTP_NO_KICK=1.
#if defined(__APPLE__)

// macOS: iSerialNumber via device-level libusb_open — icdd holds the interface, not the device (ep 0 still openable).
std::string ReadDeviceSerialViaLibusb(libusb_context* ctx, int bus, int address) {
    libusb_device** list = nullptr;
    ssize_t cnt = libusb_get_device_list(ctx, &list);
    if (cnt < 0) return {};
    std::string out;
    for (ssize_t i = 0; i < cnt; ++i) {
        libusb_device* d = list[i];
        if (libusb_get_bus_number(d) != bus) continue;
        if (libusb_get_device_address(d) != address) continue;
        libusb_device_descriptor dd{};
        if (libusb_get_device_descriptor(d, &dd) != 0) break;
        if (dd.iSerialNumber == 0) break;
        libusb_device_handle* h = nullptr;
        if (libusb_open(d, &h) != 0 || !h) break;
        unsigned char buf[256] = {0};
        int n = libusb_get_string_descriptor_ascii(h, dd.iSerialNumber, buf, sizeof(buf));
        if (n > 0) out.assign(reinterpret_cast<char*>(buf), static_cast<size_t>(n));
        libusb_close(h);
        break;
    }
    libusb_free_device_list(list, 1);
    return out;
}

// Image Capture per-device pref key: "Device-VVVV-PPPP-SERIAL" (uppercase hex); no serial → "Device-VVVV-PPPP".
std::string MakeImageCapturePrefKey(uint16_t vid, uint16_t pid, const std::string& serial) {
    char buf[96];
    if (!serial.empty()) {
        std::snprintf(buf, sizeof(buf), "Device-%04X-%04X-%s", vid, pid, serial.c_str());
    } else {
        std::snprintf(buf, sizeof(buf), "Device-%04X-%04X", vid, pid);
    }
    return std::string(buf);
}

// Equivalent to Image Capture.app's "Connecting this device opens: No application" — icdd reads at match-time, skips claim.
void WriteImageCaptureNoLaunchPref(const std::string& deviceKey) {
    CFStringRef domain = CFSTR("com.apple.ImageCaptureCore");
    CFStringRef key = CFStringCreateWithCString(nullptr, deviceKey.c_str(), kCFStringEncodingUTF8);
    if (!key) return;

    CFMutableDictionaryRef dev = CFDictionaryCreateMutable(nullptr, 0,
        &kCFTypeDictionaryKeyCallBacks, &kCFTypeDictionaryValueCallBacks);
    CFDictionarySetValue(dev, CFSTR("autolaunchPath"), CFSTR(""));

    CFPreferencesSetValue(key, dev, domain,
                          kCFPreferencesCurrentUser, kCFPreferencesAnyHost);
    CFPreferencesSynchronize(domain,
                             kCFPreferencesCurrentUser, kCFPreferencesAnyHost);

    CFRelease(dev);
    CFRelease(key);
}

// Per-user MTP-claimer pids (icdd, mscamerad-xpc, ptpcamerad) via sysctl; adb fork-server untouched (USB-debugging conflict).
struct ClaimerPids {
    pid_t icdd = 0;
    pid_t mscamerad = 0;
    pid_t ptpcamerad = 0;
};

ClaimerPids FindMtpClaimers() {
    ClaimerPids out;
    int mib[4] = { CTL_KERN, KERN_PROC, KERN_PROC_ALL, 0 };
    size_t bufsz = 0;
    if (sysctl(mib, 4, nullptr, &bufsz, nullptr, 0) != 0 || bufsz == 0) {
        DBG("FindMtpClaimers: sysctl size probe failed errno=%d\n", errno);
        return out;
    }
    bufsz += sizeof(struct kinfo_proc) * 32;
    std::vector<unsigned char> buf(bufsz);
    if (sysctl(mib, 4, buf.data(), &bufsz, nullptr, 0) != 0) {
        DBG("FindMtpClaimers: sysctl fetch failed errno=%d\n", errno);
        return out;
    }
    const struct kinfo_proc* kp = reinterpret_cast<const struct kinfo_proc*>(buf.data());
    size_t count = bufsz / sizeof(struct kinfo_proc);
    uid_t our_uid = getuid();
    for (size_t i = 0; i < count; ++i) {
        if (kp[i].kp_eproc.e_ucred.cr_uid != our_uid) continue;
        const char* name = kp[i].kp_proc.p_comm;
        if (out.icdd == 0 && std::strcmp(name, "icdd") == 0) {
            out.icdd = kp[i].kp_proc.p_pid;
        } else if (out.mscamerad == 0 && std::strcmp(name, "mscamerad-xpc") == 0) {
            out.mscamerad = kp[i].kp_proc.p_pid;
        } else if (out.ptpcamerad == 0 && std::strcmp(name, "ptpcamerad") == 0) {
            out.ptpcamerad = kp[i].kp_proc.p_pid;
        }
    }
    return out;
}

// Write "No app" pref → SIGKILL icdd/* → 50ms settle (respawned icdd reads pref, skips claim); MTP_NO_ICDD_BYPASS=1 disables.
void DislodgeIcdd(libusb_context* ctx, int bus, int address,
                  uint16_t vid, uint16_t pid) {
    if (const char* off = std::getenv("MTP_NO_ICDD_BYPASS")) {
        if (off[0] && off[0] != '0') {
            DBG("Dislodge: skipped (MTP_NO_ICDD_BYPASS=%s)\n", off);
            return;
        }
    }

    std::string serial = ReadDeviceSerialViaLibusb(ctx, bus, address);
    std::string key = MakeImageCapturePrefKey(vid, pid, serial);
    DBG("Dislodge: pref key=%s serial_len=%zu\n", key.c_str(), serial.size());

    WriteImageCaptureNoLaunchPref(key);

    ClaimerPids pids = FindMtpClaimers();
    DBG("Dislodge: icdd=%d mscamerad=%d ptpcamerad=%d\n",
        pids.icdd, pids.mscamerad, pids.ptpcamerad);

    bool killed = false;
    auto try_kill = [&](pid_t p, const char* name) {
        if (p <= 0) return;
        if (kill(p, SIGKILL) == 0) {
            DBG("Dislodge: SIGKILL %s pid=%d ok\n", name, p);
            killed = true;
        } else {
            DBG("Dislodge: SIGKILL %s pid=%d failed errno=%d (%s)\n",
                name, p, errno, std::strerror(errno));
        }
    };
    // ptpcamerad killed LAST — race-critical claimer for Galaxy on Tahoe.
    try_kill(pids.mscamerad, "mscamerad-xpc");
    try_kill(pids.icdd, "icdd");
    try_kill(pids.ptpcamerad, "ptpcamerad");

    if (killed) {
        usleep(50 * 1000);  // IOKit-cleanup window before respawned icdd re-claims.
    }
}

#endif // __APPLE__

} // namespace

libusb_context* SharedLibusbContext() {
    struct CtxHolder {
        libusb_context* ctx = nullptr;
        CtxHolder() { libusb_init(&ctx); }
        // Never libusb_exit; keeping hotplug thread alive avoids races.
    };
    static CtxHolder holder;
    return holder.ctx;
}

UsbDescriptorStrings ReadUsbDescriptorStrings(int bus, int address) {
    UsbDescriptorStrings out;
    libusb_context* ctx = SharedLibusbContext();
    if (!ctx) return out;
    libusb_device** list = nullptr;
    ssize_t cnt = libusb_get_device_list(ctx, &list);
    if (cnt < 0) return out;
    for (ssize_t i = 0; i < cnt; ++i) {
        libusb_device* d = list[i];
        if (libusb_get_bus_number(d) != bus) continue;
        if (libusb_get_device_address(d) != address) continue;
        libusb_device_descriptor dd{};
        if (libusb_get_device_descriptor(d, &dd) != 0) break;
        libusb_device_handle* h = nullptr;
        if (libusb_open(d, &h) != 0 || !h) break;
        unsigned char buf[256];
        auto read_str = [&](uint8_t idx, std::string& dst) {
            if (idx == 0) return;
            buf[0] = 0;
            int n = libusb_get_string_descriptor_ascii(h, idx, buf, sizeof(buf));
            if (n > 0) dst.assign(reinterpret_cast<char*>(buf), static_cast<size_t>(n));
        };
        read_str(dd.iManufacturer, out.manufacturer);
        read_str(dd.iProduct, out.product);
        read_str(dd.iSerialNumber, out.serial);
        libusb_close(h);
        break;
    }
    libusb_free_device_list(list, 1);
    return out;
}

namespace {

void KickPriorInterfaceClaim(int bus, int address) {
    if (const char* off = std::getenv("MTP_NO_KICK")) {
        if (off[0] && off[0] != '0') {
            DBG("Kick: skipped (MTP_NO_KICK=%s)\n", off);
            return;
        }
    }
    libusb_context* ctx = SharedLibusbContext();
    if (!ctx) {
        DBG("Kick: no libusb context\n");
        return;
    }
    libusb_device** list = nullptr;
    ssize_t cnt = libusb_get_device_list(ctx, &list);
    DBG("Kick: get_device_list cnt=%zd target_bus=%d target_addr=%d\n", cnt, bus, address);
    if (cnt < 0) {
        return;
    }
    bool kicked = false;
    bool found = false;
    for (ssize_t i = 0; i < cnt; ++i) {
        libusb_device* d = list[i];
        int dbus = libusb_get_bus_number(d);
        int daddr = libusb_get_device_address(d);
        if (dbus != bus || daddr != address) continue;
        found = true;
        libusb_device_handle* h = nullptr;
        int orc = libusb_open(d, &h);
        DBG("Kick: libusb_open rc=%d (%s) h=%p\n", orc, libusb_error_name(orc), (void*)h);
        if (orc != 0 || !h) {
            // Likely icdd holds device w/ kIOUSBExclusiveAccess at IOKit level — can't open, let alone kick.
            break;
        }
        int cfg = -1;
        int grc = libusb_get_configuration(h, &cfg);
        DBG("Kick: get_configuration rc=%d cfg=%d\n", grc, cfg);
        if (grc == 0 && cfg > 0) {
            int src = libusb_set_configuration(h, cfg);
            DBG("Kick: set_configuration(%d) rc=%d (%s)\n", cfg, src, libusb_error_name(src));
            if (src == 0) {
                kicked = true;
            }
        }
        libusb_close(h);
        break;
    }
    libusb_free_device_list(list, 1);
    DBG("Kick: result found=%d kicked=%d\n", found, kicked);
    if (kicked) {
        usleep(300 * 1000);  // Settle: altsetting/endpoint reshuffle post-SetConfiguration before libmtp open + PTP-OpenSessions.
    }
}

} // namespace

LibMtpBackend::LibMtpBackend(UsbDeviceCandidate candidate)
    : _candidate(std::move(candidate)) {}

LibMtpBackend::~LibMtpBackend() {
    Disconnect();
}

namespace {
struct ClaimsRegistry {
    std::mutex mtx;
    std::unordered_map<std::string, std::weak_ptr<LibMtpBackend>> claims;
};

ClaimsRegistry& Registry() {
    static ClaimsRegistry inst;
    return inst;
}
} // namespace

Result<std::vector<UsbDeviceCandidate>> LibMtpBackend::Enumerate() {
    EnsureLibmtpInit();

    std::vector<UsbDeviceCandidate> out;
    LIBMTP_raw_device_t* rawdevs = nullptr;
    int n = 0;
    LIBMTP_error_number_t e = LIBMTP_Detect_Raw_Devices(&rawdevs, &n);
    if (e != LIBMTP_ERROR_NONE) {
        if (rawdevs) free(rawdevs);
        DBG("libmtp Detect_Raw_Devices err=%d n=%d\n", static_cast<int>(e), n);
        if (e == LIBMTP_ERROR_NO_DEVICE_ATTACHED) {
            return Result<std::vector<UsbDeviceCandidate>>::Success({});
        }
        return Result<std::vector<UsbDeviceCandidate>>::Failure(ErrorCode::Io,
            "LIBMTP_Detect_Raw_Devices failed", true);
    }

    for (int i = 0; i < n; ++i) {
        UsbDeviceCandidate c;
        c.id.bus = static_cast<int>(rawdevs[i].bus_location);
        c.id.address = static_cast<int>(rawdevs[i].devnum);
        c.id.interface_number = 0;
        c.vendor_id = rawdevs[i].device_entry.vendor_id;
        c.product_id = rawdevs[i].device_entry.product_id;

        // Concrete USB descriptors beat libmtp's catchall ("Galaxy models (MTP)") — device-level open works through icdd's grip.
        UsbDescriptorStrings ud = ReadUsbDescriptorStrings(c.id.bus, c.id.address);
        if (!ud.manufacturer.empty()) c.manufacturer = ud.manufacturer;
        else if (rawdevs[i].device_entry.vendor) c.manufacturer = rawdevs[i].device_entry.vendor;
        if (!ud.product.empty()) c.product = ud.product;
        else if (rawdevs[i].device_entry.product) c.product = rawdevs[i].device_entry.product;
        c.serial = ud.serial;

        DBG("libmtp detected key=%s vid=%04x pid=%04x mfr=%s product=%s serial=%s\n",
            c.id.Key().c_str(), c.vendor_id, c.product_id,
            c.manufacturer.c_str(), c.product.c_str(), c.serial.c_str());
        out.push_back(std::move(c));
    }
    free(rawdevs);
    return Result<std::vector<UsbDeviceCandidate>>::Success(std::move(out));
}

std::shared_ptr<LibMtpBackend> LibMtpBackend::AcquireShared(const UsbDeviceCandidate& candidate) {
    auto& reg = Registry();
    const std::string key = candidate.id.Key();
    std::lock_guard<std::mutex> lk(reg.mtx);
    auto it = reg.claims.find(key);
    if (it != reg.claims.end()) {
        if (auto live = it->second.lock()) {
            DBG("AcquireShared reuse key=%s\n", key.c_str());
            return live;
        }
    }
    auto backend = std::make_shared<LibMtpBackend>(candidate);
    reg.claims[key] = backend;
    DBG("AcquireShared new key=%s\n", key.c_str());
    return backend;
}

void LibMtpBackend::ReleaseShared(const std::string& device_key) {
    auto& reg = Registry();
    std::lock_guard<std::mutex> lk(reg.mtx);
    auto it = reg.claims.find(device_key);
    if (it == reg.claims.end()) return;
    // Keep weak_ptr alive while another panel holds the backend (2nd libmtp session = "device busy").
    if (!it->second.lock()) reg.claims.erase(it);
}

bool LibMtpBackend::IsReady() const {
    std::lock_guard<std::recursive_mutex> lk(_mtx);
    return _device != nullptr;
}

void LibMtpBackend::ClearErrors() {
    if (_device) {
        LIBMTP_Clear_Errorstack(_device);
    }
}

Status LibMtpBackend::DrainErrors(const char* what) {
    if (!_device) {
        return Status::Failure(ErrorCode::Disconnected, std::string(what) + ": device closed");
    }
    LIBMTP_error_t* head = LIBMTP_Get_Errorstack(_device);
    ErrorCode mapped = ErrorCode::Io;
    std::string msg = what;
    bool any = false;
    for (LIBMTP_error_t* e = head; e; e = e->next) {
        any = true;
        mapped = MapLibmtpError(e->errornumber);
        if (e->error_text) {
            msg += ": ";
            msg += e->error_text;
        }
    }
    LIBMTP_Clear_Errorstack(_device);
    if (!any) {
        msg += ": unknown failure";
    }
    // libmtp drops PTP RCs — pull from message; treat "device refused" RCs as Unsupported so host fallback covers them.
    if (msg.find("Error 2005") != std::string::npos
            || msg.find("Error 2002") != std::string::npos
            || msg.find("Error 2006") != std::string::npos
            || msg.find("Error 2009") != std::string::npos
            || msg.find("Error 200B") != std::string::npos
            || msg.find("Error 200b") != std::string::npos
            || msg.find("Error 201A") != std::string::npos
            || msg.find("Error 201a") != std::string::npos
            || msg.find("Error 201D") != std::string::npos
            || msg.find("Error 201d") != std::string::npos
            || msg.find("Invalid Parent") != std::string::npos
            || msg.find("Invalid Object Handle") != std::string::npos
            || msg.find("Operation Not Supported") != std::string::npos
            || msg.find("OperationNotSupported") != std::string::npos) {
        mapped = ErrorCode::Unsupported;
    }
    return Status::Failure(mapped, msg);
}

Status LibMtpBackend::Connect() {
    std::lock_guard<std::recursive_mutex> lk(_mtx);
    EnsureLibmtpInit();
    if (_device) {
        return OkStatus();
    }

    // Single open attempt — libmtp leaks claim state on later-step failure; MTP_LIBMTP_DEBUG=usb+ptp surfaces real cause.
#if defined(__APPLE__)
    DislodgeIcdd(SharedLibusbContext(),
                 _candidate.id.bus, _candidate.id.address,
                 _candidate.vendor_id, _candidate.product_id);
#else
    KickPriorInterfaceClaim(_candidate.id.bus, _candidate.id.address);
#endif

    LIBMTP_raw_device_t* rawdevs = nullptr;
    int n = 0;
    LIBMTP_error_number_t e = LIBMTP_Detect_Raw_Devices(&rawdevs, &n);
    if (e != LIBMTP_ERROR_NONE) {
        if (rawdevs) free(rawdevs);
        return Status::Failure(MapLibmtpError(e), "LIBMTP_Detect_Raw_Devices failed");
    }

    LIBMTP_raw_device_t* match = nullptr;
    for (int i = 0; i < n; ++i) {
        if (rawdevs[i].device_entry.vendor_id == _candidate.vendor_id
                && rawdevs[i].device_entry.product_id == _candidate.product_id) {
            match = &rawdevs[i];
            break;
        }
    }

    if (!match) {
        free(rawdevs);
        return Status::Failure(ErrorCode::NotFound,
            "device not seen by libmtp raw probe");
    }

    // Uncached: cached mode's initial flush_handles takes 5+ minutes on a Galaxy with tens of thousands of photos.
    DBG("Connect: calling LIBMTP_Open_Raw_Device_Uncached\n");
    _device = LIBMTP_Open_Raw_Device_Uncached(match);
    DBG("Connect: LIBMTP_Open_Raw_Device_Uncached -> %p\n", (void*)_device);
    free(rawdevs);

    if (!_device) {
        return Status::Failure(ErrorCode::Busy,
            "Device refused MTP session (busy or unauthorized).");
    }

    // 6×250ms retry: some devices don't publish storage tree immediately after Open; non-fatal on giveup.
    ClearErrors();
    bool got_storage = false;
    for (int attempt = 0; attempt < 6; ++attempt) {
        if (LIBMTP_Get_Storage(_device, LIBMTP_STORAGE_SORTBY_NOTSORTED) == 0) {
            got_storage = true;
            break;
        }
        DBG("Connect Get_Storage attempt %d failed — retrying\n", attempt);
        LIBMTP_Clear_Errorstack(_device);
        usleep(250 * 1000);
    }
    if (!got_storage) {
        DBG("Connect Get_Storage gave up — non-fatal, proceeding without storage list\n");
    }

    // Friendly name: user label preferred, model name fallback.
    if (char* fn = LIBMTP_Get_Friendlyname(_device)) {
        if (*fn) _friendly_name = fn;
        free(fn);
    }
    if (_friendly_name.empty()) {
        if (char* mn = LIBMTP_Get_Modelname(_device)) {
            if (*mn) _friendly_name = mn;
            free(mn);
        }
    }
    return OkStatus();
}

std::string LibMtpBackend::FriendlyName() const {
    std::lock_guard<std::recursive_mutex> lk(_mtx);
    return _friendly_name;
}

// No PollEvents: libmtp has no cheap presence ping — disconnect surfaces lazily at next op (NO_DEVICE_ATTACHED → Disconnected).

void LibMtpBackend::Disconnect() {
    std::lock_guard<std::recursive_mutex> lk(_mtx);
    if (_device) {
        LIBMTP_Release_Device(_device);
        _device = nullptr;
    }
    ClearSessionCachesLocked();
}

void LibMtpBackend::ClearSessionCachesLocked() {
    // Session-scoped (caps drift; handles die with libmtp session).
    _caps_probed = false;
    _cap_copy_object = false;
    _cap_move_object = false;
    _cap_edit_objects = false;
    _cap_partial_object = false;
    _supported_filetypes.clear();
    _known_names.clear();
    _storage_case_insens.clear();
}

void LibMtpBackend::ReopenAfterCancelLocked() {
    // Release only — eager reopen blocks _mtx and didn't recover Sony's post-CancelRequest desync; next op triggers Connect().
    if (_device) {
        DBG("ReopenAfterCancel: releasing desynced libmtp session\n");
        const auto t0 = std::chrono::steady_clock::now();
        LIBMTP_Release_Device(_device);
        _device = nullptr;
        const auto t1 = std::chrono::steady_clock::now();
        const auto release_ms = std::chrono::duration_cast<std::chrono::milliseconds>(t1 - t0).count();
        DBG("ReopenAfterCancel: released in %lldms, leaving session closed\n",
            static_cast<long long>(release_ms));
    }
}

void LibMtpBackend::RefreshCache() {
    std::lock_guard<std::recursive_mutex> lk(_mtx);
    if (!_device) return;
    DBG("RefreshCache: closing libmtp device to drop cached_files\n");
    LIBMTP_Release_Device(_device);
    _device = nullptr;

    LIBMTP_raw_device_t* rawdevs = nullptr;
    int n = 0;
    LIBMTP_error_number_t e = LIBMTP_Detect_Raw_Devices(&rawdevs, &n);
    if (e != LIBMTP_ERROR_NONE || !rawdevs) {
        if (rawdevs) free(rawdevs);
        DBG("RefreshCache: Detect_Raw_Devices failed err=%d\n", static_cast<int>(e));
        return;
    }
    LIBMTP_raw_device_t* match = nullptr;
    for (int i = 0; i < n; ++i) {
        if (rawdevs[i].device_entry.vendor_id == _candidate.vendor_id
                && rawdevs[i].device_entry.product_id == _candidate.product_id) {
            match = &rawdevs[i];
            break;
        }
    }
    if (match) {
        _device = LIBMTP_Open_Raw_Device_Uncached(match);
        DBG("RefreshCache: reopened device=%p\n", (void*)_device);
        if (_device) {
            ClearErrors();
            LIBMTP_Get_Storage(_device, LIBMTP_STORAGE_SORTBY_NOTSORTED);
        }
    } else {
        DBG("RefreshCache: device disappeared from raw probe\n");
    }
    free(rawdevs);
}

Result<std::vector<StorageEntry>> LibMtpBackend::ListStorages() {
    std::lock_guard<std::recursive_mutex> lk(_mtx);
    if (!_device) {
        return Result<std::vector<StorageEntry>>::Failure(ErrorCode::Disconnected, "no device");
    }
    ClearErrors();
    std::vector<StorageEntry> out;
    for (LIBMTP_devicestorage_t* s = _device->storage; s; s = s->next) {
        StorageEntry e;
        e.id = s->id;
        e.description = s->StorageDescription ? s->StorageDescription : "";
        e.volume = s->VolumeIdentifier ? s->VolumeIdentifier : "";
        e.free_bytes = s->FreeSpaceInBytes;
        e.max_capacity = s->MaxCapacity;
        // 0x0003 FixedRAM=Internal, 0x0004 RemovableRAM=External; ROM variants (0x0001/0x0002) → Unknown.
        switch (s->StorageType) {
            case 0x0003: e.kind = StorageKind::Internal; break;
            case 0x0004: e.kind = StorageKind::External; break;
            default:     e.kind = StorageKind::Unknown;  break;
        }
        out.push_back(std::move(e));
    }
    return Result<std::vector<StorageEntry>>::Success(std::move(out));
}

Result<std::vector<ObjectEntry>> LibMtpBackend::ListChildrenLocked(StorageId storageId, ObjectHandle parent, bool include_tombstones) {
    (void)include_tombstones;
    if (!_device) {
        return Result<std::vector<ObjectEntry>>::Failure(ErrorCode::Disconnected, "no device");
    }
    ClearErrors();
    uint32_t pid = (parent == 0 || parent == kRootParent) ? LIBMTP_FILES_AND_FOLDERS_ROOT : parent;

    if (auto fast = ListChildrenFast(_device, storageId, pid, _known_names)) {
        return Result<std::vector<ObjectEntry>>::Success(std::move(*fast));
    }

    // Slow path fallback: per-object LIBMTP_Get_Files_And_Folders.
    LIBMTP_file_t* head = LIBMTP_Get_Files_And_Folders(_device, storageId, pid);
    if (!head) {
        if (LIBMTP_Get_Errorstack(_device)) {
            auto st = DrainErrors("Get_Files_And_Folders");
            return Result<std::vector<ObjectEntry>>::Failure(st.code, st.message);
        }
        return Result<std::vector<ObjectEntry>>::Success({});
    }

    std::vector<ObjectEntry> out;
    while (head) {
        ObjectEntry e = FromMtpFile(head);
        PatchKnownName(e, _known_names);
        out.push_back(std::move(e));
        LIBMTP_file_t* next = head->next;
        LIBMTP_destroy_file_t(head);
        head = next;
    }
    return Result<std::vector<ObjectEntry>>::Success(std::move(out));
}

Result<std::vector<ObjectEntry>> LibMtpBackend::ListChildren(StorageId storageId, ObjectHandle parent) {
    std::lock_guard<std::recursive_mutex> lk(_mtx);
    return ListChildrenLocked(storageId, parent);
}

Result<ObjectEntry> LibMtpBackend::StatLocked(ObjectHandle handle) {
    if (!_device) {
        return Result<ObjectEntry>::Failure(ErrorCode::Disconnected, "no device");
    }
    ClearErrors();
    LIBMTP_file_t* f = LIBMTP_Get_Filemetadata(_device, handle);
    if (!f) {
        auto st = DrainErrors("Get_Filemetadata");
        return Result<ObjectEntry>::Failure(st.code == ErrorCode::Ok ? ErrorCode::NotFound : st.code, st.message);
    }
    ObjectEntry e = FromMtpFile(f);
    LIBMTP_destroy_file_t(f);
    PatchKnownName(e, _known_names);
    return Result<ObjectEntry>::Success(std::move(e));
}

Result<ObjectEntry> LibMtpBackend::Stat(ObjectHandle handle) {
    std::lock_guard<std::recursive_mutex> lk(_mtx);
    return StatLocked(handle);
}

Status LibMtpBackend::Download(ObjectHandle handle,
                               const std::string& local_path,
                               std::function<void(uint64_t, uint64_t)> progress,
                               const CancellationToken& cancel) {
    std::lock_guard<std::recursive_mutex> lk(_mtx);
    if (!_device) {
        return Status::Failure(ErrorCode::Disconnected, "no device");
    }
    ClearErrors();
    ProgressCtx ctx{std::move(progress), &cancel};
    int rc = LIBMTP_Get_File_To_File(_device, handle, local_path.c_str(), ProgressTrampoline, &ctx);
    if (rc != 0) {
        if (cancel.IsCancelled()) {
            ReopenAfterCancelLocked();
            return Status::Failure(ErrorCode::Cancelled, "download cancelled");
        }
        return DrainErrors("Get_File_To_File");
    }
    return OkStatus();
}

Status LibMtpBackend::Upload(const std::string& local_path,
                             const std::string& remote_name,
                             StorageId storageId,
                             ObjectHandle parent,
                             std::function<void(uint64_t, uint64_t)> progress,
                             const CancellationToken& cancel) {
    std::lock_guard<std::recursive_mutex> lk(_mtx);
    if (!_device) {
        return Status::Failure(ErrorCode::Disconnected, "no device");
    }

    uint64_t sz = 0;
    if (!StatFileSize(local_path, sz)) {
        return Status::Failure(ErrorCode::Io, "stat source file failed");
    }

    LIBMTP_file_t* meta = LIBMTP_new_file_t();
    if (!meta) {
        return Status::Failure(ErrorCode::Io, "LIBMTP_new_file_t failed");
    }
    meta->filename = strdup(remote_name.c_str());
    if (!meta->filename) {
        LIBMTP_destroy_file_t(meta);
        return Status::Failure(ErrorCode::Io, "strdup failed");
    }
    meta->parent_id = (parent == kRootParent) ? 0 : parent;
    meta->storage_id = storageId;
    // PickUploadFiletype: Sony rejects ObjectFormat=Undefined — always pass a type the device advertises in SupportedFiletypes.
    meta->filetype = static_cast<LIBMTP_filetype_t>(PickUploadFiletype(remote_name));
    meta->filesize = sz;

    ClearErrors();
    ProgressCtx ctx{std::move(progress), &cancel};
    int rc = LIBMTP_Send_File_From_File(_device, local_path.c_str(), meta, ProgressTrampoline, &ctx);
    uint32_t new_handle = (rc == 0) ? meta->item_id : 0;
    LIBMTP_destroy_file_t(meta);
    if (rc == 0 && new_handle) {
        // Cache filename — Galaxy returns blank from Get_Files_And_Folders for newly-created handles in this session.
        _known_names[new_handle] = remote_name;
    }

    if (rc != 0) {
        if (cancel.IsCancelled()) {
            ReopenAfterCancelLocked();
            return Status::Failure(ErrorCode::Cancelled, "upload cancelled");
        }
        return DrainErrors("Send_File_From_File");
    }
    return OkStatus();
}

Status LibMtpBackend::Rename(ObjectHandle handle, const std::string& new_name) {
    std::lock_guard<std::recursive_mutex> lk(_mtx);
    if (!_device) return Status::Failure(ErrorCode::Disconnected, "no device");
    ClearErrors();
    LIBMTP_file_t* f = LIBMTP_Get_Filemetadata(_device, handle);
    if (!f) {
        return DrainErrors("Get_Filemetadata for rename");
    }
    int rc = LIBMTP_Set_File_Name(_device, f, new_name.c_str());
    LIBMTP_destroy_file_t(f);
    if (rc != 0) {
        return DrainErrors("Set_File_Name");
    }
    // Update cached name so subsequent listings show the new value (replace if cached, else add).
    _known_names[handle] = new_name;
    return OkStatus();
}

Status LibMtpBackend::DeleteRecursiveLocked(ObjectHandle handle,
    const std::function<void(const std::string&)>& on_progress) {
    auto stat = StatLocked(handle);
    if (!stat.ok) return Status::Failure(stat.code, stat.message);

    if (stat.value.is_dir) {
        // MTP spec: Delete_Object on folder recursively deletes all descendants in one command — try fast path first.
        ClearErrors();
        DBG("LibMtp Delete_Object (direct) folder=%u name=%s\n",
            handle, stat.value.name.c_str());
        if (LIBMTP_Delete_Object(_device, handle) == 0) {
            if (on_progress) on_progress(stat.value.name);
            return OkStatus();
        }
        ClearErrors();
        // Fallback: device refuses recursive-delete OR Galaxy tombstones (ghost children from prior deletes block folder).
        DBG("Direct folder delete failed, walking children for handle=%u\n", handle);
        auto kids = ListChildrenLocked(stat.value.storage_id, handle, /*include_tombstones=*/true);
        if (!kids.ok) return Status::Failure(kids.code, kids.message);
        DBG("DeleteRecursive folder=%u children=%zu (incl tombstones)\n",
            handle, kids.value.size());
        for (const auto& k : kids.value) {
            auto st = DeleteRecursiveLocked(k.handle, on_progress);
            if (!st.ok) return st;
        }
        // Re-list to flush libmtp/device cache before parent Delete_Object so it sees the folder as actually empty.
        (void)ListChildrenLocked(stat.value.storage_id, handle, /*include_tombstones=*/true);
    }

    if (on_progress) on_progress(stat.value.name);
    ClearErrors();
    DBG("LibMtp Delete_Object handle=%u is_dir=%d name=%s\n",
        handle, stat.value.is_dir ? 1 : 0, stat.value.name.c_str());
    int rc = LIBMTP_Delete_Object(_device, handle);
    DBG("LibMtp Delete_Object rc=%d\n", rc);
    if (rc != 0) {
        auto err = DrainErrors("Delete_Object");
        DBG("Delete_Object failed handle=%u: %s\n", handle, err.message.c_str());
        return err;
    }
    _known_names.erase(handle);
    return OkStatus();
}

Status LibMtpBackend::Delete(ObjectHandle handle, bool recursive,
    std::function<void(const std::string&)> on_progress) {
    std::lock_guard<std::recursive_mutex> lk(_mtx);
    if (!_device) return Status::Failure(ErrorCode::Disconnected, "no device");
    if (recursive) return DeleteRecursiveLocked(handle, on_progress);
    if (on_progress) {
        auto stat = StatLocked(handle);
        if (stat.ok) on_progress(stat.value.name);
    }
    ClearErrors();
    if (LIBMTP_Delete_Object(_device, handle) != 0) {
        return DrainErrors("Delete_Object");
    }
    _known_names.erase(handle);
    return OkStatus();
}

Result<ObjectHandle> LibMtpBackend::MakeDirectory(const std::string& name, StorageId storageId, ObjectHandle parent) {
    std::lock_guard<std::recursive_mutex> lk(_mtx);
    if (!_device) return Result<ObjectHandle>::Failure(ErrorCode::Disconnected, "no device");
    // Samsung quirk: SendObjectInfo parent=0 → InvalidObjectHandle; wants 0xFFFFFFFF for "create at storage root".
    uint32_t pid = (parent == kRootParent || parent == 0) ? 0xFFFFFFFFu : parent;
    char* mut = strdup(name.c_str());
    if (!mut) return Result<ObjectHandle>::Failure(ErrorCode::Io, "strdup failed");
    ClearErrors();
    uint32_t fid = LIBMTP_Create_Folder(_device, mut, pid, storageId);
    free(mut);
    if (fid == 0) {
        auto st = DrainErrors("Create_Folder");
        return Result<ObjectHandle>::Failure(st.code, st.message);
    }
    // Cache for Galaxy's blank-name quirk on newly-created handles.
    _known_names[fid] = name;
    return Result<ObjectHandle>::Success(fid);
}

Result<ObjectHandle> LibMtpBackend::CopyObject(ObjectHandle handle, StorageId storageId, ObjectHandle parent) {
    std::lock_guard<std::recursive_mutex> lk(_mtx);
    if (!_device) return Result<ObjectHandle>::Failure(ErrorCode::Disconnected, "no device");
    // Same Samsung quirk as MakeDirectory: device root parent is 0xFFFFFFFF, not 0.
    const uint32_t pid = (parent == kRootParent || parent == 0) ? 0xFFFFFFFFu : parent;
    auto src = StatLocked(handle);
    if (!src.ok) return Result<ObjectHandle>::Failure(src.code, src.message);
    ClearErrors();
    if (LIBMTP_Copy_Object(_device, handle, storageId, pid) != 0) {
        auto st = DrainErrors("Copy_Object");
        return Result<ObjectHandle>::Failure(st.code, st.message);
    }
    // libmtp's wrapper drops new handle (PTP returns it in param[0]) — recover by listing target, matching name ≠ source handle.
    auto kids = ListChildrenLocked(storageId, parent, /*include_tombstones=*/false);
    if (!kids.ok) return Result<ObjectHandle>::Failure(kids.code, kids.message);
    for (const auto& k : kids.value) {
        if (k.name == src.value.name && k.handle != handle) {
            _known_names[k.handle] = src.value.name;
            return Result<ObjectHandle>::Success(k.handle);
        }
    }
    // Copy ok but couldn't relocate new handle — caller's subsequent Rename (Shift+F5) will fail and surface it.
    return Result<ObjectHandle>::Failure(ErrorCode::Io, "Copy_Object: new handle not found");
}

Status LibMtpBackend::MoveObject(ObjectHandle handle, StorageId storageId, ObjectHandle parent) {
    std::lock_guard<std::recursive_mutex> lk(_mtx);
    if (!_device) return Status::Failure(ErrorCode::Disconnected, "no device");
    const bool to_root = (parent == kRootParent || parent == 0);
    // libmtp wire convention: 0xFFFFFFFF = root for Move_Object. MTP spec §5.5.6.1: ParentObject property = 0 for root. Some firmwares are quirky and accept the "wrong" sentinel — try canonical first, then the alternate, before host fallback.
    const uint32_t move_canon = to_root ? 0xFFFFFFFFu : parent;
    const uint32_t move_alt   = to_root ? 0u          : parent;
    const uint32_t prop_canon = to_root ? 0u          : parent;
    const uint32_t prop_alt   = to_root ? 0xFFFFFFFFu : parent;
    // Track the most-recent error: later attempts hit deeper into the device's responder, so their failures carry more specific PTP codes than the canonical first attempt's "Operation not supported" / libusb-timeout shape.
    Status last_err;
    ClearErrors();
    if (LIBMTP_Move_Object(_device, handle, storageId, move_canon) == 0) return OkStatus();
    last_err = DrainErrors("Move_Object");
    if (to_root) {
        ClearErrors();
        if (LIBMTP_Move_Object(_device, handle, storageId, move_alt) == 0) return OkStatus();
        last_err = DrainErrors("Move_Object(alt-root)");
    }
    ClearErrors();
    if (LIBMTP_Set_Object_u32(_device, handle, LIBMTP_PROPERTY_ParentObject, prop_canon) == 0) return OkStatus();
    last_err = DrainErrors("Set_Object_u32(ParentObject)");
    if (to_root) {
        ClearErrors();
        if (LIBMTP_Set_Object_u32(_device, handle, LIBMTP_PROPERTY_ParentObject, prop_alt) == 0) return OkStatus();
        last_err = DrainErrors("Set_Object_u32(ParentObject,alt-root)");
    }
    return last_err;
}

// --- capability probes ------------------------------------------------------

void LibMtpBackend::EnsureCapsProbedLocked() {
    if (_caps_probed) return;
    _caps_probed = true;
    if (!_device) return;
    // Capability-driven (no vendor/product table) — ask libmtp, which queries the device's published OperationsSupported list.
    _cap_copy_object   = LIBMTP_Check_Capability(_device, LIBMTP_DEVICECAP_CopyObject)   != 0;
    _cap_move_object   = LIBMTP_Check_Capability(_device, LIBMTP_DEVICECAP_MoveObject)   != 0;
    _cap_edit_objects  = LIBMTP_Check_Capability(_device, LIBMTP_DEVICECAP_EditObjects)  != 0;
    _cap_partial_object= LIBMTP_Check_Capability(_device, LIBMTP_DEVICECAP_GetPartialObject) != 0;
    uint16_t* ft = nullptr;
    uint16_t  fn = 0;
    if (LIBMTP_Get_Supported_Filetypes(_device, &ft, &fn) == 0 && ft) {
        _supported_filetypes.assign(ft, ft + fn);
        free(ft);
    }
    DBG("Capabilities: copy=%d move=%d edit=%d partial=%d filetypes=%zu\n",
        _cap_copy_object ? 1 : 0, _cap_move_object ? 1 : 0,
        _cap_edit_objects ? 1 : 0, _cap_partial_object ? 1 : 0,
        _supported_filetypes.size());
}

bool LibMtpBackend::SupportsCopyObject() {
    std::lock_guard<std::recursive_mutex> lk(_mtx);
    EnsureCapsProbedLocked();
    return _cap_copy_object;
}

bool LibMtpBackend::SupportsMoveObject() {
    std::lock_guard<std::recursive_mutex> lk(_mtx);
    EnsureCapsProbedLocked();
    return _cap_move_object;
}

bool LibMtpBackend::SupportsEditObjects() {
    std::lock_guard<std::recursive_mutex> lk(_mtx);
    EnsureCapsProbedLocked();
    return _cap_edit_objects;
}

bool LibMtpBackend::SupportsPartialObject() {
    std::lock_guard<std::recursive_mutex> lk(_mtx);
    EnsureCapsProbedLocked();
    return _cap_partial_object;
}

std::vector<uint16_t> LibMtpBackend::SupportedFiletypes() {
    std::lock_guard<std::recursive_mutex> lk(_mtx);
    EnsureCapsProbedLocked();
    return _supported_filetypes;
}

bool LibMtpBackend::StorageIsCaseInsensitive(StorageId storageId) {
    std::lock_guard<std::recursive_mutex> lk(_mtx);
    if (auto it = _storage_case_insens.find(storageId);
            it != _storage_case_insens.end()) {
        return it->second;
    }
    bool ci = false;
    if (_device) {
        for (LIBMTP_devicestorage_t* s = _device->storage; s; s = s->next) {
            if (s->id != storageId) continue;
            // StorageType is the reliable discriminator (FilesystemType varies); 0x0004 RemovableRAM = SD card = FAT-class.
            ci = (s->StorageType == 0x0004);
            break;
        }
    }
    _storage_case_insens.emplace(storageId, ci);
    return ci;
}

bool LibMtpBackend::NamesEqual(StorageId storageId,
                                const std::string& a, const std::string& b) {
    if (a.size() != b.size()) return false;
    if (!StorageIsCaseInsensitive(storageId)) return a == b;
    return ToLowerASCII(a) == ToLowerASCII(b);
}

uint16_t LibMtpBackend::PickUploadFiletype(const std::string& remote_name) {
    std::lock_guard<std::recursive_mutex> lk(_mtx);
    EnsureCapsProbedLocked();
    // Generic ext → standard MTP filetype map; not per-device.
    std::string ext;
    if (auto dot = remote_name.rfind('.'); dot != std::string::npos) {
        ext = ToLowerASCII(remote_name.substr(dot + 1));
    }
    struct Map { const char* ext; uint16_t ft; };
    static const Map kMap[] = {
        {"mp3",  LIBMTP_FILETYPE_MP3},  {"wav",  LIBMTP_FILETYPE_WAV},
        {"ogg",  LIBMTP_FILETYPE_OGG},  {"flac", LIBMTP_FILETYPE_FLAC},
        {"m4a",  LIBMTP_FILETYPE_M4A},  {"wma",  LIBMTP_FILETYPE_WMA},
        {"aac",  LIBMTP_FILETYPE_AAC},
        {"mp4",  LIBMTP_FILETYPE_MP4},  {"m4v",  LIBMTP_FILETYPE_MP4},
        {"avi",  LIBMTP_FILETYPE_AVI},  {"mpg",  LIBMTP_FILETYPE_MPEG},
        {"mpeg", LIBMTP_FILETYPE_MPEG}, {"mov",  LIBMTP_FILETYPE_QT},
        {"wmv",  LIBMTP_FILETYPE_WMV},  {"asf",  LIBMTP_FILETYPE_ASF},
        {"jpg",  LIBMTP_FILETYPE_JPEG}, {"jpeg", LIBMTP_FILETYPE_JPEG},
        {"png",  LIBMTP_FILETYPE_PNG},  {"gif",  LIBMTP_FILETYPE_GIF},
        {"bmp",  LIBMTP_FILETYPE_BMP},  {"tif",  LIBMTP_FILETYPE_TIFF},
        {"tiff", LIBMTP_FILETYPE_TIFF},
        {"txt",  LIBMTP_FILETYPE_TEXT}, {"log",  LIBMTP_FILETYPE_TEXT},
        {"html", LIBMTP_FILETYPE_HTML}, {"htm",  LIBMTP_FILETYPE_HTML},
        {"xml",  LIBMTP_FILETYPE_XML},
        {"doc",  LIBMTP_FILETYPE_DOC},  {"docx", LIBMTP_FILETYPE_DOC},
        // libmtp 1.1.x has no PDF enum — map to DOC, which all tested responders accept as generic document.
        {"pdf",  LIBMTP_FILETYPE_DOC},
    };
    uint16_t proposed = LIBMTP_FILETYPE_UNKNOWN;
    for (const auto& m : kMap) {
        if (ext == m.ext) { proposed = m.ft; break; }
    }

    auto in_supported = [this](uint16_t ft) -> bool {
        if (_supported_filetypes.empty()) return true;  // unknown ⇒ allow
        for (uint16_t s : _supported_filetypes) if (s == ft) return true;
        return false;
    };

    if (proposed != LIBMTP_FILETYPE_UNKNOWN && in_supported(proposed)) {
        DBG("PickUploadFiletype name=%s ext=%s → natural ft=%u\n",
            remote_name.c_str(), ext.c_str(), proposed);
        return proposed;
    }
    // Either ext didn't map OR device rejects this format — try Undefined (most permissive; Galaxy etc. accept).
    if (in_supported(LIBMTP_FILETYPE_UNKNOWN)) {
        DBG("PickUploadFiletype name=%s ext=%s → UNKNOWN (Undefined)\n",
            remote_name.c_str(), ext.c_str());
        return LIBMTP_FILETYPE_UNKNOWN;
    }
    // Sony rejects Undefined too — pick a "neutral" advertised type; bytes still land, filename extension drives on-device apps.
    static const uint16_t kFallbacks[] = {
        LIBMTP_FILETYPE_TEXT, LIBMTP_FILETYPE_HTML, LIBMTP_FILETYPE_XML,
        LIBMTP_FILETYPE_AAC,  LIBMTP_FILETYPE_WAV,  LIBMTP_FILETYPE_MP3,
    };
    for (uint16_t ft : kFallbacks) {
        if (in_supported(ft)) {
            DBG("PickUploadFiletype name=%s ext=%s → neutral fallback ft=%u "
                "(natural %u + UNKNOWN both rejected by device)\n",
                remote_name.c_str(), ext.c_str(), ft, proposed);
            return ft;
        }
    }
    // Last resort: whatever the device claims to support first.
    if (!_supported_filetypes.empty()) {
        uint16_t ft = _supported_filetypes.front();
        DBG("PickUploadFiletype name=%s ext=%s → last-resort ft=%u "
            "(no neutral fallback in device's supported list)\n",
            remote_name.c_str(), ext.c_str(), ft);
        return ft;
    }
    DBG("PickUploadFiletype name=%s ext=%s → UNKNOWN (device has no caps yet)\n",
        remote_name.c_str(), ext.c_str());
    return LIBMTP_FILETYPE_UNKNOWN;
}

} // namespace proto
