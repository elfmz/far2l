#include "MTPPlugin.h"
#include "MTPDialogs.h"
#include "MTPLog.h"
#include "ProgressBatch.h"
#include "lng.h"

#include <IntStrConv.h>
#include <WideMB.h>
#include <utils.h>

#include <atomic>
#include <cerrno>
#include <climits>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <dirent.h>
#include <functional>
#include <sys/stat.h>
#include <unistd.h>
#include <array>

PluginStartupInfo g_Info = {};
FarStandardFunctions g_FSF = {};

namespace {
using proto::kRootParent;

// Aside-rename suffix for atomic-overwrite — PID-scoped to avoid clashes across plugin instances.
std::string AsideName(const std::string& base) {
    return base + ".far2l-tmp." + std::to_string(getpid());
}

// Symmetric with ParseObjectToken — single source of truth for the OBJ token form.
std::string MakeObjectToken(uint32_t handle) {
    return std::string("OBJ|") + std::to_string(handle);
}

// Strip libmtp's noisy "LIBMTP_*(): " / "PTP: " / "USB: " prefixes from user-facing text.
static std::string CleanErrorMessage(const std::string& s) {
    auto trim_prefix = [&](std::string m) {
        for (const char* p : {"LIBMTP_", "PTP:", "USB:"}) {
            if (m.compare(0, std::strlen(p), p) == 0) {
                auto colon = m.find(':');
                if (colon != std::string::npos) {
                    size_t start = colon + 1;
                    while (start < m.size() && (m[start] == ' ' || m[start] == '\t')) ++start;
                    m = m.substr(start);
                }
            }
        }
        return m;
    };
    std::string out = trim_prefix(s);
    while (!out.empty() && (out.back() == '\n' || out.back() == ' ')) out.pop_back();
    return out;
}

std::string JoinPath(const std::string& base, const std::string& name) {
    if (base.empty()) return name;
    if (base.back() == '/' || base.back() == '\\') {
        return base + name;
    }
    return base + "/" + name;
}

struct LocalStats { uint64_t size = 0; uint64_t count = 0; };

LocalStats LocalRecursiveStats(const std::string& path) {
    LocalStats s;
    struct stat st{};
    if (::lstat(path.c_str(), &st) != 0) return s;
    if (S_ISREG(st.st_mode)) { s.size = static_cast<uint64_t>(st.st_size); s.count = 1; return s; }
    if (!S_ISDIR(st.st_mode)) return s;
    DIR* d = opendir(path.c_str());
    if (!d) return s;
    while (dirent* ent = readdir(d)) {
        if (!strcmp(ent->d_name, ".") || !strcmp(ent->d_name, "..")) continue;
        auto sub = LocalRecursiveStats(path + "/" + ent->d_name);
        s.size += sub.size;
        s.count += sub.count;
    }
    closedir(d);
    return s;
}

uint64_t LocalRecursiveSize(const std::string& path) {
    return LocalRecursiveStats(path).size;
}

bool RemoveLocalPathRecursively(const std::string& path) {
    struct stat st = {};
    if (lstat(path.c_str(), &st) != 0) {
        return false;
    }
    if (S_ISDIR(st.st_mode)) {
        DIR* dir = opendir(path.c_str());
        if (!dir) {
            return false;
        }
        bool ok = true;
        while (true) {
            dirent* ent = readdir(dir);
            if (!ent) {
                break;
            }
            if (strcmp(ent->d_name, ".") == 0 || strcmp(ent->d_name, "..") == 0) {
                continue;
            }
            if (!RemoveLocalPathRecursively(path + "/" + ent->d_name)) {
                ok = false;
                break;
            }
        }
        closedir(dir);
        if (!ok) {
            return false;
        }
        return rmdir(path.c_str()) == 0;
    }
    return unlink(path.c_str()) == 0;
}

FILETIME EpochToFileTime(uint64_t unix_epoch) {
    FILETIME ft{};
    if (unix_epoch == 0) {
        return ft;
    }
    constexpr uint64_t kEpochDiff = 11644473600ULL;
    uint64_t t = (unix_epoch + kEpochDiff) * 10000000ULL;
    ft.dwLowDateTime = static_cast<DWORD>(t & 0xffffffffULL);
    ft.dwHighDateTime = static_cast<DWORD>((t >> 32) & 0xffffffffULL);
    return ft;
}

const char* FormatCodeName(uint16_t format) {
    struct Entry {
        uint16_t code;
        const char* name;
    };
    static constexpr Entry kTable[] = {
        {0x3000, "Undefined Object"},
        {0x3001, "Association (Folder)"},
        {0x3002, "Script"},
        {0x3003, "Executable"},
        {0x3004, "Text"},
        {0x3005, "HTML"},
        {0x3006, "DPOF"},
        {0x3007, "AIFF"},
        {0x3008, "WAV"},
        {0x3009, "MP3"},
        {0x300A, "AVI"},
        {0x300B, "MPEG"},
        {0x300C, "ASF"},
        {0x300D, "Undefined Video"},
        {0x300E, "Undefined Audio"},
        {0x300F, "Undefined Collection"},
        {0x3800, "Undefined Image"},
        {0x3801, "EXIF/JPEG"},
        {0x3802, "TIFF/EP"},
        {0x3803, "FlashPix"},
        {0x3804, "BMP"},
        {0x3805, "CIFF"},
        {0x3807, "GIF"},
        {0x3808, "JFIF"},
        {0x3809, "PCD"},
        {0x380A, "PICT"},
        {0x380B, "PNG"},
        {0x380D, "TIFF"},
        {0x380E, "TIFF/IT"},
        {0x380F, "JP2"},
        {0x3810, "JPX"},
        {0xBA10, "Abstract Audio Album"},
        {0xBA11, "Abstract Audio Video Playlist"},
        {0xBA12, "Abstract Mediastream"},
        {0xBA13, "Abstract Audio Playlist"},
        {0xBA14, "Abstract Video Playlist"},
        {0xBA15, "Abstract Audio Album Playlist"},
        {0xBA16, "Abstract Image Album"},
        {0xBA17, "Abstract Image Album Playlist"},
        {0xBA18, "Abstract Video Album"},
        {0xBA19, "Abstract Video Album Playlist"},
        {0xBA1A, "Abstract Audio Video Album"},
        {0xBA1B, "Abstract Audio Video Album Playlist"},
        {0xBA1C, "Abstract Contact Group"},
        {0xBA1D, "Abstract Message Folder"},
        {0xBA1E, "Abstract Chaptered Production"},
        {0xBA1F, "Abstract Audio Podcast"},
        {0xBA20, "Abstract Video Podcast"},
        {0xBA21, "Abstract Audio Video Podcast"},
        {0xBA22, "Abstract Chaptered Production Playlist"},
        {0xBA81, "XML Document"},
        {0xBA82, "MS Word Document"},
        {0xBA83, "MHT Document"},
        {0xBA84, "MS Excel Spreadsheet"},
        {0xBA85, "MS PowerPoint Presentation"},
        {0xBB00, "vCard 2"},
        {0xBB01, "vCard 3"},
        {0xBB02, "vCalendar 1"},
        {0xBB03, "vCalendar 2"},
        {0xBB04, "vMessage"},
        {0xBB05, "Contact Database"},
        {0xBB06, "Message Database"},
        {0xB901, "WMA"},
        {0xB902, "OGG"},
        {0xB903, "AAC"},
        {0xB905, "FLAC"},
        {0xB906, "FLAC"},
        {0xB981, "WMV"},
        {0xB982, "MP4"},
        {0xB983, "3GP"},
        {0xB984, "3G2"},
    };
    for (const auto& e : kTable) {
        if (e.code == format) {
            return e.name;
        }
    }
    return nullptr;
}

bool NoControls(unsigned int control_state) {
    return (control_state & (PKF_CONTROL | PKF_ALT | PKF_SHIFT)) == 0;
}

uint32_t PackDeviceTriplet(uint8_t bus, uint8_t addr, uint8_t iface) {
    return (static_cast<uint32_t>(bus) << 16) |
           (static_cast<uint32_t>(addr) << 8) |
           static_cast<uint32_t>(iface);
}

void UnpackDeviceTriplet(uint32_t packed, uint8_t& bus, uint8_t& addr, uint8_t& iface) {
    bus = static_cast<uint8_t>((packed >> 16) & 0xFFu);
    addr = static_cast<uint8_t>((packed >> 8) & 0xFFu);
    iface = static_cast<uint8_t>(packed & 0xFFu);
}

uint32_t PackDeviceFromKey(const std::string& key) {
    int bus = -1;
    int addr = -1;
    int iface = -1;
    if (sscanf(key.c_str(), "%d:%d:%d", &bus, &addr, &iface) == 3 &&
        bus >= 0 && bus <= 255 &&
        addr >= 0 && addr <= 255 &&
        iface >= 0 && iface <= 255) {
        return PackDeviceTriplet(static_cast<uint8_t>(bus),
                                 static_cast<uint8_t>(addr),
                                 static_cast<uint8_t>(iface));
    }
    return 0;
}

std::string DeviceKeyFromPacked(uint32_t packed) {
    uint8_t bus = 0;
    uint8_t addr = 0;
    uint8_t iface = 0;
    UnpackDeviceTriplet(packed, bus, addr, iface);
    return std::to_string(static_cast<int>(bus)) + ":" +
           std::to_string(static_cast<int>(addr)) + ":" +
           std::to_string(static_cast<int>(iface));
}
}

std::set<MTPPlugin*> MTPPlugin::s_live_instances;
std::mutex MTPPlugin::s_live_mtx;

MTPPlugin* MTPPlugin::PluginFromPanel(HANDLE panel) {
    HANDLE h = INVALID_HANDLE_VALUE;
    g_Info.Control(panel, FCTL_GETPANELPLUGINHANDLE, 0, reinterpret_cast<LONG_PTR>(&h));
    if (h == INVALID_HANDLE_VALUE || h == nullptr) return nullptr;
    auto* candidate = reinterpret_cast<MTPPlugin*>(h);
    std::lock_guard<std::mutex> g(s_live_mtx);
    return s_live_instances.count(candidate) ? candidate : nullptr;
}

MTPPlugin::MTPPlugin(const wchar_t* path, bool path_is_standalone_config, int) {
    if (path && path_is_standalone_config) {
        _standalone_config = path;
    }
    _panel_title = Lng(MDevicesTitle);
    {
        std::lock_guard<std::mutex> g(s_live_mtx);
        s_live_instances.insert(this);
    }
    DBG("MTPPlugin constructed\n");
}

MTPPlugin::~MTPPlugin() {
    DBG("MTPPlugin destructed, current_device_key=%s\n", _current_device_key.c_str());
    {
        std::lock_guard<std::mutex> g(s_live_mtx);
        s_live_instances.erase(this);
    }
    if (_backend) {
        // Drop ref only — backend's dtor disconnects on last shared_ptr; explicit Disconnect would kill another panel's session.
        _backend.reset();
        proto::LibMtpBackend::ReleaseShared(_current_device_key);
    }
}

int MTPPlugin::GetFindData(PluginPanelItem** panel_items, int* items_number, int) {
    DBG("GetFindData mode=%d\n", static_cast<int>(_view_mode));
    if (!panel_items || !items_number) {
        return FALSE;
    }

    switch (_view_mode) {
        case ViewMode::Devices:
            return ListDevices(panel_items, items_number);
        case ViewMode::Storages:
            return ListStorages(panel_items, items_number);
        case ViewMode::Objects:
            return ListObjects(panel_items, items_number);
    }
    return FALSE;
}

void MTPPlugin::FreeFindData(PluginPanelItem* panel_items, int items_number) {
    if (!panel_items || items_number <= 0) {
        return;
    }

    for (int i = 0; i < items_number; ++i) {
        free((void*)panel_items[i].FindData.lpwszFileName);
        free((void*)panel_items[i].Description);
    }
    free(panel_items);
}

void MTPPlugin::GetOpenPluginInfo(OpenPluginInfo* info) {
    if (!info) {
        return;
    }

    info->StructSize = sizeof(OpenPluginInfo);
    info->Flags = OPIF_SHOWPRESERVECASE | OPIF_USEHIGHLIGHTING;
    info->HostFile = nullptr;
    // CurDir feeds Ctrl+F + Copy/Move dialog; "/Internal/Temp/foo" or "/".
    if (_view_mode == ViewMode::Objects) {
        std::string rel = BuildPanelRelativePath();
        if (!rel.empty() && rel.back() == '/') rel.pop_back();
        _cur_dir_buf = L"/" + StrMB2Wide(rel);
    } else {
        _cur_dir_buf = L"/";
    }
    info->CurDir = _cur_dir_buf.c_str();
    info->Format = L"mtp";
    info->PanelTitle = _panel_title.c_str();
    info->InfoLines = nullptr;
    info->DescrFiles = nullptr;
    info->StartPanelMode = 0;
    info->StartSortMode = SM_NAME;
    info->StartSortOrder = 0;
    info->KeyBar = nullptr;
    info->ShortcutData = nullptr;

    // Custom Ctrl+0 panel modes (adb-style) — one mode per view-state; Ctrl+1..9 stay FAR's built-in default.
    static PanelMode devicesMode = {
        .ColumnTypes        = (wchar_t*)L"N,C0,C1,C2",
        .ColumnWidths       = (wchar_t*)L"0,0,18,9",
        .ColumnTitles       = nullptr,
        .FullScreen         = 0,
        .DetailedStatus     = 1,
        .AlignExtensions    = 0,
        .CaseConversion     = 0,
        .StatusColumnTypes  = (wchar_t*)L"N,C0,C1,C2",
        .StatusColumnWidths = (wchar_t*)L"0,0,18,9",
        .Reserved           = {0, 0}
    };
    static const wchar_t* devicesTitles[4];
    devicesTitles[0] = Lng(MColDevice);
    devicesTitles[1] = Lng(MColManufacturer);
    devicesTitles[2] = Lng(MColSerial);
    devicesTitles[3] = Lng(MColVidPid);
    devicesMode.ColumnTitles = (wchar_t**)devicesTitles;

    // Storages and Objects share a "files-like" mode (N + Size).
    static PanelMode filesMode = {
        .ColumnTypes        = (wchar_t*)L"N,S",
        .ColumnWidths       = (wchar_t*)L"0,9",
        .ColumnTitles       = nullptr,
        .FullScreen         = 0,
        .DetailedStatus     = 1,
        .AlignExtensions    = 0,
        .CaseConversion     = 0,
        .StatusColumnTypes  = (wchar_t*)L"N,S",
        .StatusColumnWidths = (wchar_t*)L"0,9",
        .Reserved           = {0, 0}
    };
    static const wchar_t* filesTitles[2];
    filesTitles[0] = Lng(MColName);
    filesTitles[1] = Lng(MColSize);
    filesMode.ColumnTitles = (wchar_t**)filesTitles;

    info->PanelModesNumber = 1;
    info->PanelModesArray = (_view_mode == ViewMode::Devices) ? &devicesMode : &filesMode;
}

void MTPPlugin::UpdateObjectsPanelTitle() {
    // "<serial>:<storage>/<path>"; trailing "/" at storage root so it doesn't read like a filename.
    std::string title;
    if (!_current_device_serial.empty()) {
        title = _current_device_serial + ":";
    }
    title += _current_storage_name.empty() ? "Storage" : _current_storage_name;
    if (_dir_stack.empty()) {
        title += "/";
    } else {
        for (const auto& part : _dir_stack) {
            if (part.empty()) continue;
            title += "/";
            title += part;
        }
    }
    _panel_title = StrMB2Wide(title);
}

int MTPPlugin::SetDirectory(const wchar_t* dir, int) {
    DBG("SetDirectory dir=%s view_mode=%d cur_dev=%s cur_storage=%u cur_parent=%u\n",
        dir ? StrWide2MB(dir).c_str() : "(null)",
        static_cast<int>(_view_mode),
        _current_device_key.c_str(),
        _current_storage_id,
        _current_parent);
    if (!dir) {
        return FALSE;
    }

    std::string d = StrWide2MB(dir);
    if (d == "/") {
        _view_mode = ViewMode::Devices;
        _current_storage_id = 0;
        _current_parent = 0;
        _current_storage_name.clear();
        _current_device_name.clear();
        _current_device_serial.clear();
        _dir_stack.clear();
        _name_token_index.clear();
        if (_backend) {
            _backend.reset();  // last-ref drop → backend's dtor disconnects
            proto::LibMtpBackend::ReleaseShared(_current_device_key);
        }
        _current_device_key.clear();
        _panel_title = Lng(MDevicesTitle);
        return TRUE;
    }

    // Drives-menu "Other panel" hand-off: fresh plugin in Devices mode receives an absolute storage-relative path ("/Internal/Temp"); inherit device from passive panel and walk.
    if (_view_mode == ViewMode::Devices && d.size() > 1 && d.front() == '/') {
        return AdoptSiblingAndWalkPath(d) ? TRUE : FALSE;
    }

    if (d == "..") {
        // 0 and kRootParent both mean "at storage root" — fall through to Storages on next "..", avoiding two-press.
        bool at_root = (_current_parent == 0 || _current_parent == kRootParent);
        if (_view_mode == ViewMode::Objects && !at_root) {
            const uint32_t prev = _current_parent;
            auto st = _backend->Stat(_current_parent);
            if (st.ok) {
                _current_parent = st.value.parent;
            } else {
                _current_parent = 0;
            }
            if (!_dir_stack.empty()) {
                _dir_stack.pop_back();
            }
            UpdateObjectsPanelTitle();
            DBG("SetDirectory back object=%u new_parent=%u\n", prev, _current_parent);
            return TRUE;
        }
        if (_view_mode == ViewMode::Objects && _current_storage_id != 0) {
            _view_mode = ViewMode::Storages;
            _current_parent = 0;
            _current_storage_id = 0;
            _current_storage_name.clear();
            _dir_stack.clear();
            _panel_title = StrMB2Wide(_current_device_name.empty() ? "Storages" : _current_device_name);
            return TRUE;
        }
        if (_view_mode == ViewMode::Storages) {
            _view_mode = ViewMode::Devices;
            _current_storage_id = 0;
            _current_parent = 0;
            _name_token_index.clear();
            _current_storage_name.clear();
            _current_device_name.clear();
            _current_device_serial.clear();
            _dir_stack.clear();
            if (_backend) {
                _backend.reset();  // last-ref drop → backend's dtor disconnects
                proto::LibMtpBackend::ReleaseShared(_current_device_key);
            }
            _current_device_key.clear();
            _panel_title = Lng(MDevicesTitle);
            return TRUE;
        }
        // ".." at root (Devices view) → close plugin (Esc-equivalent).
        if (_view_mode == ViewMode::Devices) {
            DBG("SetDirectory '..' at root → closing plugin\n");
            g_Info.Control(this, FCTL_CLOSEPLUGIN, 0, 0);
            return TRUE;
        }
        return TRUE;
    }

    // Try cached index first (fast path for the common Enter-on-panel-row case). On miss — possible during far2l's silent walks for F3-on-folder size compute, where _name_token_index has been overwritten by intermediate GetFindData calls — fall back to a live ListChildren of the current parent.
    auto it = _name_token_index.find(d);
    if (it != _name_token_index.end() && !it->second.empty()) {
        DBG("SetDirectory dir=%s token=%s (cached)\n", d.c_str(), it->second.c_str());
        return EnterByToken(it->second) ? TRUE : FALSE;
    }
    if (_view_mode == ViewMode::Objects && _backend && _backend->IsReady()) {
        auto kids = _backend->ListChildren(_current_storage_id, _current_parent);
        if (kids.ok) {
            // Re-warm the index from this listing so the next SetDirectory sibling hits the cache (matters for far2l's nested silent walk during F3-on-folder size compute — would otherwise issue a fresh ListChildren per sibling).
            _name_token_index.clear();
            for (const auto& k : kids.value) {
                _name_token_index.emplace(k.name, MakeObjectToken(k.handle));
            }
            for (const auto& k : kids.value) {
                if (k.is_dir && _backend->NamesEqual(_current_storage_id, k.name, d)) {
                    const std::string token = MakeObjectToken(k.handle);
                    DBG("SetDirectory dir=%s token=%s (live lookup)\n", d.c_str(), token.c_str());
                    return EnterByToken(token) ? TRUE : FALSE;
                }
            }
        }
    }
    DBG("SetDirectory unresolved dir=%s\n", d.c_str());
    return FALSE;
}

bool MTPPlugin::AdoptSiblingAndWalkPath(const std::string& abs_path) {
    DBG("AdoptSiblingAndWalkPath path=%s\n", abs_path.c_str());
    // Prefer PANEL_PASSIVE if far2l reports it. Otherwise fall back to scanning the live-instance registry — covers the Drives-menu race where the new plugin is asked SetDirectory before far2l publishes the passive panel's handle to FCTL_GETPANELPLUGINHANDLE.
    MTPPlugin* sibling = PluginFromPanel(PANEL_PASSIVE);
    auto usable = [this](MTPPlugin* p) {
        return p && p != this && !p->_current_device_key.empty()
               && p->_backend && p->_backend->IsReady();
    };
    if (!usable(sibling)) {
        std::lock_guard<std::mutex> g(s_live_mtx);
        sibling = nullptr;
        for (MTPPlugin* p : s_live_instances) {
            if (usable(p)) { sibling = p; break; }
        }
    }
    if (!sibling) {
        DBG("AdoptSiblingAndWalkPath no usable sibling (registry size=%zu)\n",
            s_live_instances.size());
        return false;
    }
    DBG("AdoptSiblingAndWalkPath sibling key=%s name=%s\n",
        sibling->_current_device_key.c_str(), sibling->_current_device_name.c_str());

    // Mirror sibling's device bind so EnsureConnected() can find it; sibling stays untouched.
    auto bind_it = sibling->_device_binds.find(sibling->_current_device_key);
    if (bind_it == sibling->_device_binds.end()) {
        DBG("AdoptSiblingAndWalkPath sibling has no device_binds entry for key=%s\n",
            sibling->_current_device_key.c_str());
        return false;
    }
    _device_binds[bind_it->first] = bind_it->second;

    if (!EnsureConnected(sibling->_current_device_key)) {
        DBG("AdoptSiblingAndWalkPath EnsureConnected failed\n");
        return false;
    }
    _current_device_name = sibling->_current_device_name;
    _current_device_serial = sibling->_current_device_serial;

    // Fast path: requested path matches sibling's CurDir 1:1 (Drives-menu "Other panel" hands us exactly what far2l displayed for the sibling — no per-segment PTP walk needed). Compare modulo leading "/" and trailing "/" — sibling's BuildPanelRelativePath ends with "/", abs_path starts with "/".
    if (sibling->_view_mode == ViewMode::Objects) {
        std::string sibling_cur = "/" + sibling->BuildPanelRelativePath();
        if (!sibling_cur.empty() && sibling_cur.back() == '/') sibling_cur.pop_back();
        std::string requested = abs_path;
        if (requested.size() > 1 && requested.back() == '/') requested.pop_back();
        if (sibling_cur == requested) {
            _current_storage_id   = sibling->_current_storage_id;
            _current_storage_name = sibling->_current_storage_name;
            _current_parent       = sibling->_current_parent;
            _dir_stack            = sibling->_dir_stack;
            _view_mode            = ViewMode::Objects;
            UpdateObjectsPanelTitle();
            DBG("AdoptSiblingAndWalkPath fast-path: cloned sibling state (storage=%u parent=%u depth=%zu)\n",
                _current_storage_id, _current_parent, _dir_stack.size());
            return true;
        }
        DBG("AdoptSiblingAndWalkPath fast-path miss: sibling_cur='%s' requested='%s'\n",
            sibling_cur.c_str(), requested.c_str());
    }

    _view_mode = ViewMode::Storages;
    _panel_title = StrMB2Wide(_current_device_name.empty() ? "Storages" : _current_device_name);
    _current_storage_id = 0;
    _current_parent = 0;
    _current_storage_name.clear();
    _dir_stack.clear();

    // Split "/A/B/C" into ["A","B","C"]
    std::vector<std::string> parts;
    std::string seg;
    for (char c : abs_path) {
        if (c == '/') {
            if (!seg.empty()) { parts.push_back(std::move(seg)); seg.clear(); }
        } else {
            seg.push_back(c);
        }
    }
    if (!seg.empty()) parts.push_back(std::move(seg));
    if (parts.empty()) {
        return true;  // landed at Storages view
    }

    // First segment = storage label ("Internal" / "External" / "Storage" with optional "<n>" suffix from ListStorages).
    auto sl = _backend->ListStorages();
    if (!sl.ok) {
        DBG("AdoptSiblingAndWalkPath ListStorages failed code=%d\n", static_cast<int>(sl.code));
        return false;
    }
    int internal_seq = 0, external_seq = 0, unknown_seq = 0;
    uint32_t storage_id = 0;
    std::string storage_label;
    for (const auto& s : sl.value) {
        std::string label;
        switch (s.kind) {
            case proto::StorageKind::Internal:
                label = (++internal_seq == 1) ? "Internal" : "Internal " + std::to_string(internal_seq);
                break;
            case proto::StorageKind::External:
                label = (++external_seq == 1) ? "External" : "External " + std::to_string(external_seq);
                break;
            default:
                label = (++unknown_seq == 1) ? "Storage" : "Storage " + std::to_string(unknown_seq);
                break;
        }
        if (label == parts[0] || s.description == parts[0] || s.volume == parts[0]) {
            storage_id = s.id;
            storage_label = label;
            break;
        }
    }
    if (storage_id == 0) {
        DBG("AdoptSiblingAndWalkPath storage '%s' not found\n", parts[0].c_str());
        return false;
    }
    _current_storage_id = storage_id;
    _current_storage_name = storage_label;
    _current_parent = 0;
    _view_mode = ViewMode::Objects;
    UpdateObjectsPanelTitle();

    // Remaining segments = folder path under the storage.
    for (size_t i = 1; i < parts.size(); ++i) {
        const std::string& part = parts[i];
        auto kids = _backend->ListChildren(_current_storage_id, _current_parent);
        if (!kids.ok) {
            DBG("AdoptSiblingAndWalkPath ListChildren failed at parent=%u code=%d\n",
                _current_parent, static_cast<int>(kids.code));
            return false;
        }
        bool found = false;
        for (const auto& k : kids.value) {
            if (k.is_dir && _backend->NamesEqual(_current_storage_id, k.name, part)) {
                _current_parent = k.handle;
                _dir_stack.push_back(k.name);
                found = true;
                break;
            }
        }
        if (!found) {
            DBG("AdoptSiblingAndWalkPath segment '%s' not found at parent=%u\n",
                part.c_str(), _current_parent);
            return false;
        }
    }
    UpdateObjectsPanelTitle();
    DBG("AdoptSiblingAndWalkPath ok: storage=%u parent=%u\n", _current_storage_id, _current_parent);
    return true;
}

int MTPPlugin::ProcessKey(int key, unsigned int control_state) {
    DBG("ProcessKey key=%d ctrl=0x%x mode=%d\n", key, control_state, static_cast<int>(_view_mode));
    // F3/F4 not intercepted — far2l routes them through GetFiles(OPM_VIEW)/GetFiles(OPM_EDIT) and manages the temp dir + viewer/editor lifecycle, including F4 write-back via PutFiles.
    if ((key == VK_F5 || key == VK_F6) && NoControls(control_state)) {
        if (CrossPanelCopyMoveSameDevice(key == VK_F6)) {
            return TRUE;
        }
    }
    if (key == VK_F6 && control_state == PKF_SHIFT) {
        // Always claim — falling through to far2l ShellCopy on cancel would show a second dialog user already dismissed.
        RenameSelectedItem();
        return TRUE;
    }
    if (key == VK_F5 && control_state == PKF_SHIFT) {
        // Same rule as Shift+F6 — claim regardless of user-cancel.
        ShiftF5CopyInPlace();
        return TRUE;
    }
    return FALSE;
}

int MTPPlugin::ProcessEvent(int /*event*/, void* /*param*/) {
    return FALSE;
}

int MTPPlugin::MakeDirectory(const wchar_t** name, int op_mode) {
    if (!_backend || !_backend->IsReady()) {
        return FALSE;
    }
    if (_view_mode != ViewMode::Objects) {
        // F7 outside the Objects view doesn't have a meaningful target.
        return FALSE;
    }

    std::string dir = (name && *name) ? StrWide2MB(*name) : std::string();
    if (!(op_mode & OPM_SILENT)) {
        if (!MTPDialogs::AskCreateDirectory(dir)) {
            return -1;
        }
    } else if (dir.empty()) {
        return FALSE;
    }

    DBG("MakeDirectory call: name=%s storage_id=%u parent=%u\n",
        dir.c_str(), _current_storage_id, _current_parent);
    auto st = _backend->MakeDirectory(dir, _current_storage_id, _current_parent);
    if (!st.ok) {
        DBG("MakeDirectory failed code=%d msg=%s\n",
            static_cast<int>(st.code), st.message.c_str());
        if (!(op_mode & OPM_SILENT)) {
            MTPDialogs::MessageWrapped(FMSG_WARNING | FMSG_MB_OK,
                Lng(MMkdirFailed), StrMB2Wide(CleanErrorMessage(st.message)));
        }
        // -1: error already shown; FAR skips its generic dialog.
        return -1;
    }
    // No RefreshCache: Galaxy's device-side index lags 50-200ms after Create_Folder — reopening now would briefly hide it.
    DBG("MakeDirectory ok name=%s new_handle=%u\n", dir.c_str(), st.value);

    // Member buffer keeps the wchar_t* valid until next call.
    _last_made_dir = StrMB2Wide(dir);
    if (name) *name = _last_made_dir.c_str();
    return TRUE;
}

int MTPPlugin::DeleteFiles(PluginPanelItem* panel_item, int items_number, int op_mode) {
    if (!_backend || !_backend->IsReady() || !panel_item || items_number <= 0) {
        return FALSE;
    }

    // Skip synthetic ".."/"." nav rows — F8 on ".." was offering to delete it.
    std::vector<int> deletable;
    deletable.reserve(items_number);
    for (int i = 0; i < items_number; ++i) {
        const wchar_t* n = panel_item[i].FindData.lpwszFileName;
        if (!n) continue;
        if (n[0] == L'.' && (n[1] == 0 || (n[1] == L'.' && n[2] == 0))) continue;
        deletable.push_back(i);
    }
    if (deletable.empty()) return -1;

    int folderCount = 0;
    for (int idx : deletable) {
        if (panel_item[idx].FindData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) ++folderCount;
    }
    const int total = static_cast<int>(deletable.size());

    if (!(op_mode & OPM_SILENT)) {
        // Mirror native ShellConfirmDeletion (delete.cpp).
        const wchar_t* title = Lng(MDeleteBoxTitle);
        std::wstring prompt, target;
        if (total == 1) {
            const bool isFolder = (folderCount == 1);
            prompt = isFolder ? Lng(MDeleteFolder)
                              : Lng(MDeleteFile);
            target = panel_item[deletable[0]].FindData.lpwszFileName;
        } else {
            prompt = Lng(MDeleteItems);
            target = std::to_wstring(total) + L" " + Lng(MPromptItems);
        }
        if (MTPDialogs::Message(FMSG_MB_YESNO, title, prompt, target) != 0) return -1;
    }

    int okCount = 0;
    int lastErr = 0;

    auto worker = [&](ProgressState& st) {
        st.count_total = static_cast<uint64_t>(total);
        uint64_t k = 0;
        for (int idx : deletable) {
            if (st.ShouldAbort()) break;
            std::string token;
            uint32_t handle = 0;
            if (!ResolvePanelToken(panel_item[idx], token)
                || !ParseObjectToken(token, handle)) {
                ++k;
                continue;
            }
            {
                std::lock_guard<std::mutex> lk(st.mtx_strings);
                st.current_file = panel_item[idx].FindData.lpwszFileName
                    ? std::wstring(panel_item[idx].FindData.lpwszFileName)
                    : std::wstring();
            }
            st.is_directory = (panel_item[idx].FindData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) != 0;
            st.count_complete = k++;

            auto on_progress = [&](const std::string& name) {
                std::lock_guard<std::mutex> lk(st.mtx_strings);
                st.current_file = StrMB2Wide(name);
            };
            auto status = _backend->Delete(handle, true, on_progress);
            if (status.ok) ++okCount;
            else lastErr = MapErrorToErrno(status);
        }
        st.count_complete = static_cast<uint64_t>(total);
    };

    if (!(op_mode & OPM_SILENT)) {
        DeleteOperation pop;
        pop.Run(worker);
        if (pop.WasAborted() && okCount == 0) return -1;
    } else {
        ProgressState dummy;
        dummy.Reset();
        worker(dummy);
    }

    if (okCount == 0 && lastErr != 0) {
        WINPORT(SetLastError)(lastErr);
    }
    // No RefreshCache: Galaxy reindex after delete renumbers handles, invalidating _current_parent → breaks next F7/F5/F8.
    return okCount > 0 ? TRUE : FALSE;
}

int MTPPlugin::GetFiles(PluginPanelItem* panel_item, int items_number, int move,
                        const wchar_t** dest_path, int op_mode) {
    if (!_backend || !_backend->IsReady() || !panel_item || items_number <= 0
            || !dest_path || !dest_path[0]) {
        return FALSE;
    }

    std::string baseDest = StrWide2MB(dest_path[0]);

    // F3/F4/QuickView fast path: single-file pull to far2l-managed temp; chmod 0644 so the editor can write back over device files that landed 0640. Far2l manages the temp dir + viewer/editor lifecycle; F4 write-back loops via PutFiles.
    if (op_mode & (OPM_VIEW | OPM_EDIT | OPM_QUICKVIEW)) {
        proto::CancellationSource cancelSrc;
        if (panel_item[0].FindData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) {
            return FALSE;
        }
        std::string token;
        if (!ResolvePanelToken(panel_item[0], token)) return FALSE;
        uint32_t handle = 0;
        if (!ParseObjectToken(token, handle)) return FALSE;
        auto st = _backend->Stat(handle);
        if (!st.ok) {
            SetErrorFromStatus(proto::Status::Failure(st.code, st.message));
            return FALSE;
        }
        std::string localPath = JoinPath(baseDest, st.value.name);
        auto op = _backend->Download(st.value.handle, localPath,
                                     [](uint64_t, uint64_t) {}, cancelSrc.Token());
        if (!op.ok) { SetErrorFromStatus(op); return FALSE; }
        ::chmod(localPath.c_str(), 0644);
        return TRUE;
    }

    if (!(op_mode & OPM_SILENT)) {
        std::string firstName;
        if (items_number > 0 && panel_item[0].FindData.lpwszFileName) {
            firstName = StrWide2MB(panel_item[0].FindData.lpwszFileName);
        }
        if (!MTPDialogs::AskCopyMove(move != 0, /*is_upload=*/false,
                                     baseDest, firstName, items_number)) {
            return -1;
        }
    }

    // Reroute device-path inputs (".."/"./"/storage-rooted/existing-name/bare) to in-device copy/move; else falls through to host xfer.
    {
        const std::string& s = baseDest;
        if (IsInDevicePathSyntax(s)) {
            // Single-item rename intent: F6 (or F5) on one item with a bare name that doesn't already exist → rename (F6) or copy+rename (F5) in current parent. Without this branch ResolveDestinationFolder would auto-create a folder named "newname" and move ucl into .../newname/ucl, which is not what the user wanted.
            const bool trailing_sep = !s.empty() && (s.back() == '/' || s.back() == '\\');
            const bool bare = (s.find_first_of("/\\") == std::string::npos);
            // Exclude dot-navigation tokens — "." / "..": these are device-path syntax (move-to-parent etc.), not rename targets. libmtp rejects Rename(h, "..") with PTP 2002.
            if (items_number == 1 && bare && !trailing_sep
                    && !s.empty() && s != "." && s != ".."
                    && _name_token_index.count(s) == 0) {
                std::string token;
                uint32_t handle = 0;
                if (ResolvePanelToken(panel_item[0], token)
                        && ParseObjectToken(token, handle)) {
                    auto sst = _backend->Stat(handle);
                    if (sst.ok && sst.value.name != s
                            && sst.value.name != "." && sst.value.name != "..") {
                        if (move) {
                            auto rn = _backend->Rename(handle, s);
                            if (!rn.ok) {
                                SetErrorFromStatus(rn);
                                MTPDialogs::MessageWrapped(FMSG_WARNING | FMSG_MB_OK,
                                    Lng(MRenameFailed), StrMB2Wide(CleanErrorMessage(rn.message)));
                                return FALSE;
                            }
                        } else {
                            auto cp = _backend->CopyObject(handle,
                                _current_storage_id, _current_parent);
                            if (!cp.ok) {
                                SetErrorFromStatus(proto::Status::Failure(cp.code, cp.message));
                                MTPDialogs::MessageWrapped(FMSG_WARNING | FMSG_MB_OK,
                                    Lng(MCopyFailed), StrMB2Wide(CleanErrorMessage(cp.message)));
                                return FALSE;
                            }
                            auto rn = _backend->Rename(cp.value, s);
                            if (!rn.ok) {
                                DBG("rename after copy failed: %s — copy remains under src name\n",
                                    rn.message.c_str());
                            }
                        }
                        g_Info.Control(PANEL_ACTIVE, FCTL_CLEARSELECTION, 0, 0);
                        g_Info.Control(PANEL_ACTIVE, FCTL_UPDATEPANEL, 0, 0);
                        g_Info.Control(PANEL_ACTIVE, FCTL_REDRAWPANEL, 0, 0);
                        return TRUE;
                    }
                }
            }

            // ResolveDestinationFolder treats input as folder-only; each queued item keeps its own basename in the loop.
            auto resolved = ResolveDestinationFolder(baseDest);
            if (!resolved.ok) {
                MTPDialogs::MessageWrapped(FMSG_WARNING | FMSG_MB_OK,
                    move ? Lng(MMoveFailed) : Lng(MCopyFailed),
                    Lng(MErrResolveDstDevice));
                return FALSE;
            }
            return InDeviceCopyMoveTo(panel_item, items_number, move != 0,
                                      resolved.storage_id, resolved.parent,
                                      baseDest);
        }
    }

    // Single pre-scan: resolve, Stat once, walk recursive stats once — collision dialog + unit-build loop reuse this.
    enum DLCollisionAct : uint8_t { DL_PROCEED, DL_OVERWRITE, DL_SKIP, DL_RENAME };
    struct Pre {
        proto::ObjectEntry sv;
        RecursiveStats stats;
        DLCollisionAct decision;
        std::string rename_to;  // populated when decision==DL_RENAME
    };
    int lastErr = 0;
    std::vector<Pre> pre;
    pre.reserve(items_number);
    for (int i = 0; i < items_number; ++i) {
        std::string token;
        if (!ResolvePanelToken(panel_item[i], token)) continue;
        uint32_t handle = 0;
        if (!ParseObjectToken(token, handle)) continue;
        auto stat_r = _backend->Stat(handle);
        if (!stat_r.ok) { lastErr = MapErrorToErrno(stat_r.code); continue; }
        Pre pi;
        pi.sv = stat_r.value;
        pi.stats = pi.sv.is_dir
            ? ComputeRecursiveStats(pi.sv.storage_id, pi.sv.handle)
            : RecursiveStats{pi.sv.size, 1};
        pi.decision = DL_PROCEED;
        pre.push_back(std::move(pi));
    }

    // Lowest-N suffix not already on disk under baseDest. "a.txt" → "a(2).txt".
    auto find_free_local = [&baseDest](const std::string& base) -> std::string {
        std::string stem = base, ext;
        auto dot = base.rfind('.');
        if (dot != std::string::npos && dot != 0) {
            stem = base.substr(0, dot);
            ext = base.substr(dot);
        }
        for (int n = 2; n < 1000; ++n) {
            std::string candidate = stem + "(" + std::to_string(n) + ")" + ext;
            struct stat fs{};
            if (::stat(JoinPath(baseDest, candidate).c_str(), &fs) != 0) return candidate;
        }
        return base + "_renamed";
    };

    // Host-fs pre-flight collision (mirrors native WarnCopyDlg).
    if (!(op_mode & OPM_SILENT)) {
        bool overwrite_all = false, skip_all = false, newer_all = false, rename_all = false;
        for (auto& pi : pre) {
            std::string localPath = JoinPath(baseDest, pi.sv.name);
            struct stat fs{};
            if (::stat(localPath.c_str(), &fs) != 0) continue;  // no collision
            uint64_t dst_show = S_ISDIR(fs.st_mode)
                ? LocalRecursiveSize(localPath)
                : static_cast<uint64_t>(fs.st_size);
            auto view_new = pi.sv.is_dir ? OverwriteDialog::ViewFn{}
                : ViewDeviceFileCallback(pi.sv.handle, pi.sv.name);
            auto view_existing = S_ISDIR(fs.st_mode) ? OverwriteDialog::ViewFn{}
                : ViewHostFileCallback(localPath);
            auto d = OverwriteDialog::AskSticky(StrMB2Wide(localPath),
                pi.stats.size, static_cast<int64_t>(pi.sv.mtime_epoch),
                dst_show, static_cast<int64_t>(fs.st_mtime),
                overwrite_all, skip_all, newer_all, rename_all,
                std::move(view_new), std::move(view_existing));
            if (d == OverwriteDialog::D_CANCEL) return -1;
            if (d == OverwriteDialog::D_RENAME) {
                pi.decision = DL_RENAME;
                pi.rename_to = find_free_local(pi.sv.name);
            } else {
                pi.decision = (d == OverwriteDialog::D_OVERWRITE) ? DL_OVERWRITE : DL_SKIP;
            }
        }
    }

    proto::CancellationSource cancelSrc;
    auto cancel_tok = cancelSrc.Token();
    auto p_lastErr = &lastErr;

    // WorkUnit per top-level item with accurate total_bytes/total_files — dialog totals stable from start (no growing-during-walk).
    std::vector<WorkUnit> units;
    units.reserve(pre.size());
    for (auto& pi : pre) {
        if (pi.decision == DL_SKIP) continue;
        const auto& sv = pi.sv;
        const auto& stats = pi.stats;

        WorkUnit u;
        u.display_name = StrMB2Wide(sv.name);
        u.is_directory = sv.is_dir;
        u.total_bytes = stats.size;
        u.total_files = std::max<uint64_t>(stats.count, 1);
        const bool needWipe = (pi.decision == DL_OVERWRITE) && sv.is_dir;
        const std::string& effective_name = pi.rename_to.empty() ? sv.name : pi.rename_to;
        const std::string localPath = JoinPath(baseDest, effective_name);
        auto* cs = &cancelSrc;
        u.execute = [this, sv, localPath, needWipe, move, cancel_tok, cs, p_lastErr]
                    (ProgressTracker& tr) -> int {
            if (needWipe) {
                struct stat probe{};
                if (::stat(localPath.c_str(), &probe) == 0
                        && S_ISDIR(probe.st_mode)
                        && !RemoveLocalPathRecursively(localPath)) {
                    DBG("Folder Overwrite: wipe of %s failed (errno=%d)\n",
                        localPath.c_str(), errno);
                    return errno ? errno : EIO;
                }
            }
            proto::Status op;
            if (sv.is_dir) {
                op = DownloadRecursive(sv.storage_id, sv.handle, localPath, cancel_tok, &tr);
            } else {
                op = _backend->Download(sv.handle, localPath,
                    [&tr, cs, name = sv.name](uint64_t sent, uint64_t total) {
                        if (tr.Aborted()) cs->Cancel();
                        tr.Tick(sent, total, name);
                    }, cancel_tok);
                if (op.ok) tr.CompleteFile(sv.size);
            }
            if (!op.ok) {
                if (op.code != proto::ErrorCode::Cancelled) *p_lastErr = MapErrorToErrno(op);
                return op.code == proto::ErrorCode::Cancelled ? ECANCELED : MapErrorToErrno(op);
            }
            if (move) {
                auto del = _backend->Delete(sv.handle, true);
                if (!del.ok) DBG("F6 move: delete-after-copy failed: %s\n", del.message.c_str());
            }
            return 0;
        };
        units.push_back(std::move(u));
    }

    auto* cs = &cancelSrc;
    auto br = (op_mode & OPM_SILENT)
        ? RunBatchSilent(std::move(units))
        : RunBatch(move != 0, /*is_upload=*/false,
                   _panel_title, StrMB2Wide(baseDest), std::move(units),
                   [cs]() { cs->Cancel(); });
    if (br.aborted && br.success_count == 0) return -1;
    if (br.success_count == 0 && lastErr != 0) WINPORT(SetLastError)(lastErr);
    return br.success_count > 0 ? TRUE : FALSE;
}

int MTPPlugin::PutFiles(PluginPanelItem* panel_item, int items_number, int move,
                        const wchar_t* src_path, int op_mode) {
    if (!_backend || !_backend->IsReady() || !panel_item || items_number <= 0 || !src_path) {
        return FALSE;
    }
    if (_view_mode != ViewMode::Objects || _current_storage_id == 0) {
        WINPORT(SetLastError)(EINVAL);
        return FALSE;
    }

    std::string baseSrc = StrWide2MB(src_path);
    if (baseSrc.empty()) {
        WINPORT(SetLastError)(EINVAL);
        return FALSE;
    }
    // Dest fixed at _current_storage_id/_current_parent; synthetic path string only for collision-dialog display.
    std::string deviceDestDisplay = StrWide2MB(_panel_title);

    // We own collision UI (far2l skips WarnCopyDlg for plugin dst); delete-then-upload on OVERWRITE — libmtp would otherwise duplicate.
    enum CollisionAct : uint8_t { ACT_PROCEED, ACT_OVERWRITE, ACT_SKIP };
    std::map<std::string, proto::ObjectEntry> existing_by_name;
    {
        auto kids = _backend->ListChildren(_current_storage_id, _current_parent);
        if (kids.ok) {
            for (const auto& k : kids.value) existing_by_name[k.name] = k;
        }
    }

    // Single pre-scan: filter, lstat, recursive-walk dirs once — collision dialog + unit-build loop reuse these stats.
    struct Pre {
        std::string fileName;
        std::string localPath;
        bool isDir;
        LocalStats stats;
        int64_t mtime;
        CollisionAct decision;
        std::string upload_as;  // populated when user picked Rename — uploaded under this name instead of fileName.
    };
    std::vector<Pre> pre;
    pre.reserve(items_number);
    for (int i = 0; i < items_number; ++i) {
        if (!panel_item[i].FindData.lpwszFileName) continue;
        std::string fileName = StrWide2MB(panel_item[i].FindData.lpwszFileName);
        if (fileName.empty() || fileName == "." || fileName == "..") continue;
        std::string localPath = JoinPath(baseSrc, fileName);
        struct stat fs{};
        if (::stat(localPath.c_str(), &fs) != 0) continue;
        Pre pi;
        pi.fileName = std::move(fileName);
        pi.localPath = std::move(localPath);
        pi.isDir = S_ISDIR(fs.st_mode);
        pi.stats = pi.isDir
            ? LocalRecursiveStats(pi.localPath)
            : LocalStats{(fs.st_size > 0 ? static_cast<uint64_t>(fs.st_size) : 0), 1};
        pi.mtime = static_cast<int64_t>(fs.st_mtime);
        pi.decision = ACT_PROCEED;
        pre.push_back(std::move(pi));
    }

    if (!(op_mode & OPM_SILENT)) {
        bool overwrite_all = false, skip_all = false, newer_all = false, rename_all = false;
        for (auto& pi : pre) {
            // Case-fold on FAT-class storages; strict on ext4.
            const proto::ObjectEntry* existing = FindExistingByName(
                existing_by_name, pi.fileName, _current_storage_id);
            if (!existing) continue;
            std::string fullDevicePath = deviceDestDisplay;
            auto colon = fullDevicePath.find(':');
            if (colon != std::string::npos) fullDevicePath = fullDevicePath.substr(colon + 1);
            if (!fullDevicePath.empty() && fullDevicePath.back() != '/') fullDevicePath += '/';
            fullDevicePath += pi.fileName;
            uint64_t dst_show = existing->is_dir
                ? ComputeRecursiveSize(_current_storage_id, existing->handle)
                : existing->size;
            auto view_new = pi.isDir ? OverwriteDialog::ViewFn{}
                : ViewHostFileCallback(pi.localPath);
            auto view_existing = existing->is_dir ? OverwriteDialog::ViewFn{}
                : ViewDeviceFileCallback(existing->handle, existing->name);
            auto d = OverwriteDialog::AskSticky(StrMB2Wide(fullDevicePath),
                pi.stats.size, pi.mtime,
                dst_show, static_cast<int64_t>(existing->mtime_epoch),
                overwrite_all, skip_all, newer_all, rename_all,
                std::move(view_new), std::move(view_existing));
            if (d == OverwriteDialog::D_CANCEL) return -1;
            if (d == OverwriteDialog::D_RENAME) {
                pi.decision = ACT_PROCEED;
                pi.upload_as = FindFreeName(existing_by_name, pi.fileName, _current_storage_id);
            } else {
                pi.decision = (d == OverwriteDialog::D_OVERWRITE) ? ACT_OVERWRITE : ACT_SKIP;
            }
        }
    }

    proto::CancellationSource cancelSrc;
    auto cancel_tok = cancelSrc.Token();
    int lastErr = 0;
    auto p_lastErr = &lastErr;

    std::vector<WorkUnit> units;
    units.reserve(pre.size());
    for (auto& pi : pre) {
        if (pi.decision == ACT_SKIP) continue;
        const std::string fileName = pi.upload_as.empty() ? pi.fileName : pi.upload_as;
        const std::string localPath = pi.localPath;
        const bool isDir = pi.isDir;
        const uint64_t fsize = pi.stats.size;

        WorkUnit u;
        u.display_name = StrMB2Wide(fileName);
        u.is_directory = isDir;
        u.total_bytes = pi.stats.size;
        u.total_files = std::max<uint64_t>(pi.stats.count, 1);
        const bool needOverwriteDelete = (pi.decision == ACT_OVERWRITE);
        auto* cs = &cancelSrc;
        auto* ebn = &existing_by_name;
        u.execute = [this, fileName, localPath, isDir, fsize,
                     needOverwriteDelete, move,
                     storage_id = _current_storage_id, parent = _current_parent,
                     cancel_tok, cs, ebn, p_lastErr]
                    (ProgressTracker& tr) -> int {
            if (needOverwriteDelete) {
                const proto::ObjectEntry* existing = FindExistingByName(
                    *ebn, fileName, storage_id);
                if (existing) {
                    auto del = _backend->Delete(existing->handle, /*recursive=*/true);
                    if (!del.ok) DBG("upload-overwrite delete failed: %s — duplicate '%s' may appear\n",
                                     del.message.c_str(), fileName.c_str());
                    ebn->erase(existing->name);
                }
            }
            proto::Status up;
            if (isDir) {
                up = UploadRecursive(localPath, fileName, storage_id, parent,
                                     cancel_tok, cs, &tr);
            } else {
                up = _backend->Upload(localPath, fileName, storage_id, parent,
                    [&tr, cs, name = fileName](uint64_t sent, uint64_t total) {
                        if (tr.Aborted()) cs->Cancel();
                        tr.Tick(sent, total, name);
                    }, cancel_tok);
                if (up.ok) tr.CompleteFile(fsize);
            }
            if (!up.ok) {
                if (up.code != proto::ErrorCode::Cancelled) *p_lastErr = MapErrorToErrno(up);
                return up.code == proto::ErrorCode::Cancelled ? ECANCELED : MapErrorToErrno(up);
            }
            if (move) (void)RemoveLocalPathRecursively(localPath);
            return 0;
        };
        units.push_back(std::move(u));
    }

    auto* cs = &cancelSrc;
    auto br = (op_mode & OPM_SILENT)
        ? RunBatchSilent(std::move(units))
        : RunBatch(move != 0, /*is_upload=*/true,
                   StrMB2Wide(baseSrc), StrMB2Wide(deviceDestDisplay),
                   std::move(units),
                   [cs]() { cs->Cancel(); });
    if (br.aborted && br.success_count == 0) return -1;
    if (br.success_count == 0 && lastErr != 0) WINPORT(SetLastError)(lastErr);
    return br.success_count > 0 ? TRUE : FALSE;
}

// Far2l Execute callback. F3/F4/QuickView are routed via GetFiles (handled in its OPM_VIEW/OPM_EDIT/OPM_QUICKVIEW fast path); nothing else triggers Execute for this plugin.
int MTPPlugin::Execute(PluginPanelItem*, int, int) {
    return FALSE;
}

PluginStartupInfo* MTPPlugin::GetInfo() {
    return &g_Info;
}

PluginPanelItem MTPPlugin::MakePanelItem(const std::string& name,
                                         bool is_dir,
                                         uint64_t size,
                                         uint64_t mtime_epoch,
                                         uint32_t object_id,
                                         uint32_t storage_id,
                                         uint32_t packed_device_id,
                                         const std::string& description,
                                         uint64_t ctime_epoch,
                                         uint32_t file_attributes,
                                         uint32_t unix_mode) const {
    PluginPanelItem item = {{{0}}};

    std::wstring wname = StrMB2Wide(name);
    item.FindData.lpwszFileName = static_cast<wchar_t*>(calloc(wname.size() + 1, sizeof(wchar_t)));
    if (item.FindData.lpwszFileName) {
        wcscpy(const_cast<wchar_t*>(item.FindData.lpwszFileName), wname.c_str());
    }

    if (file_attributes == 0) {
        file_attributes = is_dir ? FILE_ATTRIBUTE_DIRECTORY : FILE_ATTRIBUTE_NORMAL;
    }
    if (unix_mode == 0) {
        unix_mode = is_dir ? (S_IFDIR | 0755) : (S_IFREG | 0644);
    }
    item.FindData.dwFileAttributes = file_attributes;
    item.FindData.dwUnixMode = unix_mode;
    item.FindData.nFileSize = size;
    item.FindData.nPhysicalSize = size;
    if (ctime_epoch == 0) {
        ctime_epoch = mtime_epoch;
    }
    item.FindData.ftCreationTime = EpochToFileTime(ctime_epoch);
    item.FindData.ftLastAccessTime = item.FindData.ftCreationTime;
    item.FindData.ftLastWriteTime = item.FindData.ftCreationTime;
    if (mtime_epoch != 0) {
        item.FindData.ftLastWriteTime = EpochToFileTime(mtime_epoch);
        item.FindData.ftLastAccessTime = item.FindData.ftLastWriteTime;
    }

    item.UserData = static_cast<DWORD_PTR>(object_id);

    if (!description.empty()) {
        std::wstring wd = StrMB2Wide(description);
        item.Description = static_cast<wchar_t*>(calloc(wd.size() + 1, sizeof(wchar_t)));
        if (item.Description) {
            wcscpy(const_cast<wchar_t*>(item.Description), wd.c_str());
        }
    }
    item.Reserved[0] = static_cast<DWORD_PTR>(storage_id);
    item.Reserved[1] = static_cast<DWORD_PTR>(packed_device_id);

    return item;
}

std::string MTPPlugin::BuildObjectDescription(const proto::ObjectEntry& entry) {
    if (entry.format != 0) {
        const char* name = FormatCodeName(entry.format);
        if (name) {
            return std::string(name);
        }
        char fmt[32] = {};
        snprintf(fmt, sizeof(fmt), "Unknown (0x%04X)", entry.format);
        return std::string(fmt);
    }
    return std::string();
}

PluginPanelItem MTPPlugin::MakeObjectPanelItem(const proto::ObjectEntry& entry,
                                               uint32_t packed_device_id) const {
    uint32_t attrs = entry.is_dir ? FILE_ATTRIBUTE_DIRECTORY : FILE_ATTRIBUTE_NORMAL;
    if (entry.is_hidden || (!entry.name.empty() && entry.name[0] == '.')) {
        attrs |= FILE_ATTRIBUTE_HIDDEN;
    }
    if (entry.is_readonly) {
        attrs |= FILE_ATTRIBUTE_READONLY;
    }

    uint32_t mode = entry.is_dir ? (S_IFDIR | 0755) : (S_IFREG | 0644);
    if (entry.is_readonly) {
        mode = entry.is_dir ? (S_IFDIR | 0555) : (S_IFREG | 0444);
    }

    return MakePanelItem(entry.name,
                         entry.is_dir,
                         entry.size,
                         entry.mtime_epoch,
                         entry.handle,
                         entry.storage_id,
                         packed_device_id,
                         BuildObjectDescription(entry),
                         entry.ctime_epoch,
                         attrs,
                         mode);
}

bool MTPPlugin::GetSelectedPanelUserData(std::string& out) const {
    out.clear();
    intptr_t size = g_Info.Control(PANEL_ACTIVE, FCTL_GETSELECTEDPANELITEM, 0, 0);
    DBG("GetSelectedPanelUserData size=%ld\n", static_cast<long>(size));
    if (size < static_cast<intptr_t>(sizeof(PluginPanelItem))) {
        return false;
    }

    PluginPanelItem* item = static_cast<PluginPanelItem*>(malloc(size + 0x100));
    if (!item) {
        return false;
    }

    memset(item, 0, size + 0x100);
    intptr_t ok = g_Info.Control(PANEL_ACTIVE, FCTL_GETSELECTEDPANELITEM, 0, reinterpret_cast<LONG_PTR>(item));
    DBG("GetSelectedPanelUserData fetch_ok=%ld user_data=%llu storage=%llu dev=%llu name=%s\n",
        static_cast<long>(ok),
        static_cast<unsigned long long>(item->UserData),
        static_cast<unsigned long long>(item->Reserved[0]),
        static_cast<unsigned long long>(item->Reserved[1]),
        item->FindData.lpwszFileName ? StrWide2MB(item->FindData.lpwszFileName).c_str() : "(null)");
    if (ok) {
        (void)ResolvePanelToken(*item, out);
    }
    DBG("GetSelectedPanelUserData token=%s\n", out.c_str());
    free(item);
    return !out.empty();
}

bool MTPPlugin::GetSelectedPanelFileName(std::string& out) const {
    out.clear();
    intptr_t size = g_Info.Control(PANEL_ACTIVE, FCTL_GETSELECTEDPANELITEM, 0, 0);
    if (size < static_cast<intptr_t>(sizeof(PluginPanelItem))) {
        return false;
    }

    PluginPanelItem* item = static_cast<PluginPanelItem*>(malloc(size + 0x100));
    if (!item) {
        return false;
    }
    memset(item, 0, size + 0x100);

    intptr_t ok = g_Info.Control(PANEL_ACTIVE, FCTL_GETSELECTEDPANELITEM, 0, reinterpret_cast<LONG_PTR>(item));
    if (!ok) {
        DBG("GetSelectedPanelFileName fetch failed\n");
        free(item);
        return false;
    }

    if (item->FindData.lpwszFileName) {
        out = StrWide2MB(item->FindData.lpwszFileName);
    }
    DBG("GetSelectedPanelFileName ok name=%s\n", out.c_str());
    free(item);
    return !out.empty();
}

bool MTPPlugin::EnsureConnected(const std::string& device_key) {
    DBG("EnsureConnected device_key=%s\n", device_key.c_str());
    auto it = _device_binds.find(device_key);
    if (it == _device_binds.end()) {
        DBG("EnsureConnected missing device bind key=%s\n", device_key.c_str());
        return false;
    }

    if (_backend && _current_device_key == device_key && _backend->IsReady()) {
        return true;
    }

    if (_backend) {
        DBG("EnsureConnected drop previous key=%s\n", _current_device_key.c_str());
        _backend.reset();  // last-ref drop → backend's dtor disconnects
        proto::LibMtpBackend::ReleaseShared(_current_device_key);
    }

    DBG("EnsureConnected acquire backend vid=%04x pid=%04x bus=%d addr=%d\n",
        it->second.candidate.vendor_id,
        it->second.candidate.product_id,
        it->second.candidate.id.bus,
        it->second.candidate.id.address);
    _backend = proto::LibMtpBackend::AcquireShared(it->second.candidate);
    if (!_backend) {
        DBG("EnsureConnected acquire returned null\n");
        return false;
    }

    auto st = _backend->Connect();
    if (!st.ok) {
        DBG("Connect failed code=%d msg=%s\n", static_cast<int>(st.code), st.message.c_str());
        SetErrorFromStatus(st);
        _backend.reset();
        return false;
    }

    DBG("Connect success device_key=%s\n", device_key.c_str());
    _current_device_key = device_key;
    return true;
}

int MTPPlugin::ListDevices(PluginPanelItem** panel_items, int* items_number) {
    auto devices = proto::LibMtpBackend::Enumerate();
    if (!devices.ok) {
        DBG("Enumerate failed code=%d msg=%s\n", static_cast<int>(devices.code), devices.message.c_str());
        WINPORT(SetLastError)(MapErrorToErrno(devices.code));
        return FALSE;
    }
    DBG("Enumerate found=%zu\n", devices.value.size());

    _device_binds.clear();

    std::vector<PluginPanelItem> out;
    out.reserve(devices.value.size() + 1);
    _name_token_index.clear();

    // ".." up-link → SetDirectory("..") → FCTL_CLOSEPLUGIN.
    out.push_back(MakePanelItem("..", true, 0, 0, 0, 0, 0));

    for (const auto& c : devices.value) {
        const std::string key = c.id.Key();
        DeviceBind bind;
        bind.candidate = c;
        _device_binds[key] = bind;

        std::string name = c.product.empty() ? "USB Device" : c.product;
        if (!c.serial.empty()) {
            name += " [" + c.serial + "]";
        }
        const std::string desc = std::string("MTP ") +
                                 (c.manufacturer.empty() ? "" : c.manufacturer);

        std::string token = "DEV|" + key;
        const uint32_t packedDev = PackDeviceTriplet(static_cast<uint8_t>(c.id.bus),
                                                     static_cast<uint8_t>(c.id.address),
                                                     static_cast<uint8_t>(c.id.interface_number));
        DBG("ListDevices item name=%s token=%s vid=%04x pid=%04x serial=%s\n",
            name.c_str(),
            token.c_str(),
            c.vendor_id,
            c.product_id,
            c.serial.c_str());
        out.push_back(MakePanelItem(name, true, 0, 0, 0, 0, packedDev, desc));
        auto em = _name_token_index.emplace(name, token);
        if (!em.second) {
            em.first->second.clear();
        }
    }

    // Mirror ADB plugin: when no devices are present, show a "<Not found>" placeholder
    // row with a "<Connect device>" hint, so the panel doesn't look empty/broken.
    // out always contains "..", so test devices.value not out.
    if (devices.value.empty()) {
        _panel_title = Lng(MNoDevicesPanelTitle);
        out.push_back(MakePanelItem("<Not found>", false, 0, 0, 0, 0, 0, "<Connect device>"));
    } else {
        _panel_title = Lng(MDevicesTitle);
    }

    *items_number = static_cast<int>(out.size());
    *panel_items = static_cast<PluginPanelItem*>(calloc(out.size(), sizeof(PluginPanelItem)));
    if (!*panel_items) {
        return FALSE;
    }

    for (size_t i = 0; i < out.size(); ++i) {
        (*panel_items)[i] = out[i];
    }
    return TRUE;
}

int MTPPlugin::ListStorages(PluginPanelItem** panel_items, int* items_number) {
    if (!_backend || !_backend->IsReady()) {
        DBG("ListStorages backend not ready backend=%p\n", _backend.get());
        return FALSE;
    }

    auto storages = _backend->ListStorages();
    if (!storages.ok) {
        DBG("ListStorages failed code=%d msg=%s\n", static_cast<int>(storages.code), storages.message.c_str());
        WINPORT(SetLastError)(MapErrorToErrno(storages.code));
        return FALSE;
    }
    DBG("ListStorages count=%zu\n", storages.value.size());

    std::vector<PluginPanelItem> out;
    out.reserve(storages.value.size() + 1);
    const uint32_t packedDev = PackDeviceFromKey(_current_device_key);
    _name_token_index.clear();
    out.push_back(MakePanelItem("..", true, 0, 0, 0, 0, packedDev));
    // Compact "Internal"/"External" labels (verbose StorageDescription → description column); duplicates get "...2", "...3".
    int internal_seq = 0, external_seq = 0, unknown_seq = 0;
    for (const auto& s : storages.value) {
        std::string name;
        switch (s.kind) {
            case proto::StorageKind::Internal:
                name = (++internal_seq == 1) ? "Internal"
                                             : "Internal " + std::to_string(internal_seq);
                break;
            case proto::StorageKind::External:
                name = (++external_seq == 1) ? "External"
                                             : "External " + std::to_string(external_seq);
                break;
            default:
                name = (++unknown_seq == 1) ? "Storage"
                                            : "Storage " + std::to_string(unknown_seq);
                break;
        }
        std::string token = "STO|" + std::to_string(s.id);
        DBG("ListStorages item id=%u name=%s desc=%s volume=%s kind=%d free=%llu cap=%llu token=%s\n",
            s.id, name.c_str(), s.description.c_str(), s.volume.c_str(),
            static_cast<int>(s.kind),
            static_cast<unsigned long long>(s.free_bytes),
            static_cast<unsigned long long>(s.max_capacity),
            token.c_str());
        // Description column carries StorageDescription; primary label is name.
        out.push_back(MakePanelItem(name, true, s.max_capacity, 0, 0, s.id, packedDev, s.description));
        auto em = _name_token_index.emplace(name, token);
        if (!em.second) {
            em.first->second.clear();
        }
    }

    *items_number = static_cast<int>(out.size());
    *panel_items = static_cast<PluginPanelItem*>(calloc(out.size(), sizeof(PluginPanelItem)));
    if (!*panel_items) {
        return FALSE;
    }

    for (size_t i = 0; i < out.size(); ++i) {
        (*panel_items)[i] = out[i];
    }
    return TRUE;
}

int MTPPlugin::ListObjects(PluginPanelItem** panel_items, int* items_number) {
    if (!_backend || !_backend->IsReady() || _current_storage_id == 0) {
        DBG("ListObjects precondition failed backend=%p ready=%d storage=%u parent=%u\n",
            _backend.get(),
            (_backend && _backend->IsReady()) ? 1 : 0,
            _current_storage_id,
            _current_parent);
        return FALSE;
    }

    auto children = _backend->ListChildren(_current_storage_id, _current_parent);
    if (!children.ok) {
        DBG("ListChildren failed storage=%u parent=%u code=%d msg=%s\n",
            _current_storage_id, _current_parent, static_cast<int>(children.code), children.message.c_str());
        WINPORT(SetLastError)(MapErrorToErrno(children.code));
        return FALSE;
    }
    DBG("ListChildren storage=%u parent=%u count=%zu\n", _current_storage_id, _current_parent, children.value.size());

    std::vector<PluginPanelItem> out;
    out.reserve(children.value.size() + 1);
    const uint32_t packedDev = PackDeviceFromKey(_current_device_key);
    _name_token_index.clear();
    out.push_back(MakePanelItem("..", true, 0, 0, 0, 0, packedDev));
    for (const auto& e : children.value) {
        std::string token = MakeObjectToken(e.handle);
        DBG("ListObjects item handle=%u parent=%u storage=%u dir=%d size=%llu name=%s token=%s\n",
            e.handle,
            e.parent,
            e.storage_id,
            e.is_dir ? 1 : 0,
            static_cast<unsigned long long>(e.size),
            e.name.c_str(),
            token.c_str());
        out.push_back(MakeObjectPanelItem(e, packedDev));
        auto em = _name_token_index.emplace(e.name, token);
        if (!em.second) {
            em.first->second.clear();
        }
    }

    *items_number = static_cast<int>(out.size());
    *panel_items = static_cast<PluginPanelItem*>(calloc(out.size(), sizeof(PluginPanelItem)));
    if (!*panel_items) {
        return FALSE;
    }

    for (size_t i = 0; i < out.size(); ++i) {
        (*panel_items)[i] = out[i];
    }
    return TRUE;
}

bool MTPPlugin::ParseStorageToken(const std::string& token, uint32_t& storage_id) const {
    if (!StrStartsFrom(token, "STO|")) {
        return false;
    }
    try {
        storage_id = static_cast<uint32_t>(std::stoul(token.substr(4)));
        return true;
    } catch (...) {
        return false;
    }
}

bool MTPPlugin::ParseObjectToken(const std::string& token, uint32_t& handle) const {
    if (!StrStartsFrom(token, "OBJ|")) {
        return false;
    }
    try {
        handle = static_cast<uint32_t>(std::stoul(token.substr(4)));
        return true;
    } catch (...) {
        return false;
    }
}

bool MTPPlugin::ResolvePanelToken(const PluginPanelItem& item, std::string& token) const {
    token.clear();
    const uint32_t object_id = static_cast<uint32_t>(item.UserData);
    const uint32_t storage_id = static_cast<uint32_t>(item.Reserved[0]);
    const uint32_t packed_dev = static_cast<uint32_t>(item.Reserved[1]);

    if (_view_mode == ViewMode::Objects) {
        if (object_id != 0) {
            token = MakeObjectToken(object_id);
            return true;
        }
    } else if (_view_mode == ViewMode::Storages) {
        if (storage_id != 0) {
            token = "STO|" + std::to_string(storage_id);
            return true;
        }
    } else if (_view_mode == ViewMode::Devices) {
        if (packed_dev != 0) {
            const std::string key = DeviceKeyFromPacked(packed_dev);
            if (!key.empty()) {
                token = "DEV|" + key;
                return true;
            }
        }
    }

    if (object_id != 0) {
        token = MakeObjectToken(object_id);
        return true;
    }
    if (storage_id != 0) {
        token = "STO|" + std::to_string(storage_id);
        return true;
    }
    if (packed_dev != 0) {
        const std::string key = DeviceKeyFromPacked(packed_dev);
        if (!key.empty()) {
            token = "DEV|" + key;
            return true;
        }
    }

    if (item.FindData.lpwszFileName) {
        std::string name = StrWide2MB(item.FindData.lpwszFileName);
        auto it = _name_token_index.find(name);
        if (it != _name_token_index.end() && !it->second.empty()) {
            token = it->second;
            return true;
        }
    }
    return false;
}

std::vector<uint32_t> MTPPlugin::CollectActivePanelHandles(std::string* first_name) {
    auto fetch = [](int cmd, int idx, PluginPanelItem& out, std::vector<uint8_t>& buf) -> bool {
        intptr_t sz = g_Info.Control(PANEL_ACTIVE, cmd, idx, 0);
        if (sz < static_cast<intptr_t>(sizeof(PluginPanelItem))) return false;
        buf.assign(static_cast<size_t>(sz + 0x100), 0);
        auto* item = reinterpret_cast<PluginPanelItem*>(buf.data());
        if (!g_Info.Control(PANEL_ACTIVE, cmd, idx, reinterpret_cast<LONG_PTR>(item))) return false;
        out = *item;
        return true;
    };

    auto try_resolve = [&](const PluginPanelItem& item) -> uint32_t {
        std::string token;
        uint32_t h = 0;
        if (!ResolvePanelToken(item, token)) return 0;
        if (!ParseObjectToken(token, h)) return 0;
        auto st = _backend->Stat(h);
        if (!st.ok || st.value.name == "." || st.value.name == "..") return 0;
        if (first_name && first_name->empty()) *first_name = st.value.name;
        return h;
    };

    std::vector<uint32_t> handles;
    PanelInfo pi = {};
    g_Info.Control(PANEL_ACTIVE, FCTL_GETPANELINFO, 0, reinterpret_cast<LONG_PTR>(&pi));

    if (pi.SelectedItemsNumber > 0) {
        for (int i = 0; i < pi.SelectedItemsNumber; ++i) {
            PluginPanelItem item = {};
            std::vector<uint8_t> buf;
            if (!fetch(FCTL_GETSELECTEDPANELITEM, i, item, buf)) continue;
            if (uint32_t h = try_resolve(item)) handles.push_back(h);
        }
    } else {
        PluginPanelItem item = {};
        std::vector<uint8_t> buf;
        if (fetch(FCTL_GETCURRENTPANELITEM, 0, item, buf)) {
            if (uint32_t h = try_resolve(item)) handles.push_back(h);
        }
    }
    return handles;
}

int MTPPlugin::MapErrorToErrno(const proto::Status& st) const {
    return MapErrorToErrno(st.code);
}

int MTPPlugin::MapErrorToErrno(proto::ErrorCode code) const {
    switch (code) {
        case proto::ErrorCode::NotFound: return ENOENT;
        case proto::ErrorCode::AccessDenied: return EACCES;
        case proto::ErrorCode::Busy: return EBUSY;
        case proto::ErrorCode::Timeout: return ETIMEDOUT;
        case proto::ErrorCode::InvalidArgument: return EINVAL;
        case proto::ErrorCode::Unsupported: return ENOTSUP;
        case proto::ErrorCode::Cancelled: return ECANCELED;
        case proto::ErrorCode::Disconnected: return ENODEV;
        default: return EIO;
    }
}

void MTPPlugin::SetErrorFromStatus(const proto::Status& st) const {
    WINPORT(SetLastError)(MapErrorToErrno(st));
    _last_error_message = CleanErrorMessage(st.message);
}

bool MTPPlugin::PromptInput(const wchar_t* title,
                            const wchar_t* prompt,
                            const wchar_t* history_name,
                            const std::string& initial_value,
                            std::string& out) const {
    out.clear();

    wchar_t input_buffer[1024] = {0};
    if (!initial_value.empty()) {
        std::wstring w = StrMB2Wide(initial_value);
        wcsncpy(input_buffer, w.c_str(), (sizeof(input_buffer) / sizeof(input_buffer[0])) - 1);
    }

    const bool ok = g_Info.InputBox(title,
                                    prompt,
                                    history_name,
                                    nullptr,
                                    input_buffer,
                                    (sizeof(input_buffer) / sizeof(input_buffer[0])) - 1,
                                    nullptr,
                                    FIB_BUTTONS | FIB_NOUSELASTHISTORY);
    if (!ok) {
        return false;
    }
    out = StrWide2MB(input_buffer);
    return !out.empty();
}

void MTPPlugin::RefreshPanel() const {
    g_Info.Control(PANEL_ACTIVE, FCTL_UPDATEPANEL, 0, 0);
    g_Info.Control(PANEL_ACTIVE, FCTL_REDRAWPANEL, 0, 0);
}

MTPPlugin::DirResolution MTPPlugin::ResolveDestinationFolder(const std::string& input) {
    DirResolution out;
    out.storage_id = _current_storage_id;
    out.parent = _current_parent;
    if (input.empty()) { out.ok = true; return out; }

    std::vector<std::string> parts;
    std::string cur;
    for (char c : input) {
        if (c == '/' || c == '\\') {
            if (!cur.empty()) parts.push_back(std::move(cur));
            cur.clear();
        } else {
            cur.push_back(c);
        }
    }
    if (!cur.empty()) parts.push_back(std::move(cur));

    // Storage-rooted: leading "Internal/..." walks from storage root.
    if (!parts.empty() && !_current_storage_name.empty()
            && parts.front() == _current_storage_name) {
        out.parent = kRootParent;
        parts.erase(parts.begin());
    }

    for (const auto& p : parts) {
        if (p == "..") {
            if (out.parent == 0 || out.parent == kRootParent) return out;
            auto st = _backend->Stat(out.parent);
            if (!st.ok) return out;
            out.parent = (st.value.parent == 0) ? kRootParent : st.value.parent;
        } else if (p == "." || p.empty()) {
            // skip
        } else {
            auto kids = _backend->ListChildren(out.storage_id, out.parent);
            if (!kids.ok) return out;
            bool found = false;
            for (const auto& k : kids.value) {
                if (k.is_dir && _backend->NamesEqual(out.storage_id, k.name, p)) {
                    out.parent = k.handle;
                    found = true;
                    break;
                }
            }
            if (!found) {
                // Auto-mkdir missing component (matches native far2l).
                auto md = _backend->MakeDirectory(p, out.storage_id, out.parent);
                if (!md.ok) return out;
                out.parent = md.value;
            }
        }
    }
    out.ok = true;
    return out;
}

std::string MTPPlugin::BuildPanelRelativePath() const {
    // "<storage>/<dir>/<dir>/" for ShiftF5/F6 prompt prefills.
    std::string out = _current_storage_name.empty()
        ? std::string("Storage") : _current_storage_name;
    for (const auto& part : _dir_stack) {
        if (part.empty()) continue;
        out += "/";
        out += part;
    }
    out += "/";
    return out;
}

std::function<void()> MTPPlugin::ViewHostFileCallback(const std::string& path) const {
    return [path]() {
        // VF_NONMODAL — plugin SDK pattern (matches farftp). Returns immediately; far2l's frame manager pushes the viewer above the dialog, Esc closes the viewer and pops back to the dialog.
        std::wstring w = StrMB2Wide(path);
        g_Info.Viewer(w.c_str(), w.c_str(), -1, -1, -1, -1,
                      VF_NONMODAL, CP_UTF8);
    };
}

std::function<void()> MTPPlugin::ViewDeviceFileCallback(uint32_t handle, const std::string& display_name) {
    auto backend = _backend;  // shared_ptr keeps backend alive for the closure
    return [backend, handle, display_name]() {
        if (!backend) return;
        static std::atomic<uint64_t> seq{0};
        std::string safe = display_name;
        for (char& c : safe) if (c == '/' || c == '\\') c = '_';
        char prefix[64];
        snprintf(prefix, sizeof(prefix), "mtp_overw_%d_%llu_",
                 static_cast<int>(getpid()),
                 static_cast<unsigned long long>(seq.fetch_add(1)));
        const std::string tmp = InMyTemp((std::string(prefix) + safe).c_str());
        proto::CancellationSource cs;
        auto dl = backend->Download(handle, tmp,
                                    [](uint64_t, uint64_t) {}, cs.Token());
        if (!dl.ok) return;
        ::chmod(tmp.c_str(), 0644);
        std::wstring w = StrMB2Wide(tmp);
        g_Info.Viewer(w.c_str(), w.c_str(), -1, -1, -1, -1,
                      VF_NONMODAL | VF_DELETEONCLOSE, CP_UTF8);
    };
}

bool MTPPlugin::IsInDevicePathSyntax(const std::string& s) const {
    // Dot-prefixed (./, ../, ..) or contains "/.." anywhere.
    if (s == "." || s == "./" || s == ".\\"
            || s == ".." || s == "../" || s == "..\\"
            || s.compare(0, 2, "./") == 0
            || s.compare(0, 2, ".\\") == 0
            || s.compare(0, 3, "../") == 0
            || s.compare(0, 3, "..\\") == 0
            || s.find("/..") != std::string::npos) {
        return true;
    }
    // Storage-rooted (optional leading slash + first non-empty component matches storage name).
    if (!_current_storage_name.empty()) {
        size_t start = (!s.empty() && (s[0] == '/' || s[0] == '\\')) ? 1 : 0;
        size_t slash = s.find_first_of("/\\", start);
        std::string head = (slash == std::string::npos)
            ? s.substr(start) : s.substr(start, slash - start);
        if (head == _current_storage_name) return true;
    }
    // Bare name (no slash): always route to device — ResolveDestinationFolder auto-creates.
    if (_backend && _backend->IsReady()
            && s.find_first_of("/\\") == std::string::npos
            && !s.empty() && s.size() <= 255) {
        return true;
    }
    return false;
}

std::string MTPPlugin::FindFreeName(
        const std::map<std::string, proto::ObjectEntry>& taken,
        const std::string& base, uint32_t storage_id) const {
    // Split off ".ext" so "a.txt" → "a(2).txt", not "a.txt(2)".
    std::string stem = base, ext;
    auto dot = base.rfind('.');
    if (dot != std::string::npos && dot != 0) {
        stem = base.substr(0, dot);
        ext = base.substr(dot);
    }
    for (int n = 2; n < 1000; ++n) {
        std::string candidate = stem + "(" + std::to_string(n) + ")" + ext;
        if (!FindExistingByName(taken, candidate, storage_id)) return candidate;
    }
    return base + "_renamed";  // pathological fallback
}

const proto::ObjectEntry* MTPPlugin::FindExistingByName(
        const std::map<std::string, proto::ObjectEntry>& m,
        const std::string& name, uint32_t storage_id) const {
    auto it = m.find(name);  // strict (works on both fs types)
    if (it != m.end()) return &it->second;
    if (!_backend) return nullptr;
    // Case-fold scan only on FAT-class storages.
    if (!_backend->StorageIsCaseInsensitive(storage_id)) return nullptr;
    for (const auto& kv : m) {
        if (_backend->NamesEqual(storage_id, kv.first, name)) {
            return &kv.second;
        }
    }
    return nullptr;
}

uint64_t MTPPlugin::ComputeRecursiveSize(uint32_t storage_id, uint32_t handle, int depth) {
    return ComputeRecursiveStats(storage_id, handle, depth).size;
}

MTPPlugin::RecursiveStats MTPPlugin::ComputeRecursiveStats(uint32_t storage_id,
                                                            uint32_t handle,
                                                            int depth) {
    RecursiveStats r;
    if (!_backend || depth > 8) return r;
    auto kids = _backend->ListChildren(storage_id, handle);
    if (!kids.ok) return r;
    int seen = 0;
    for (const auto& k : kids.value) {
        if (++seen > 2000) break;
        if (k.is_dir) {
            auto sub = ComputeRecursiveStats(k.storage_id, k.handle, depth + 1);
            r.size += sub.size;
            r.count += sub.count;
        } else {
            r.size += k.size;
            r.count += 1;
        }
    }
    return r;
}

int MTPPlugin::InDeviceCopyMoveTo(PluginPanelItem* panel_item, int items_number,
                                   bool move, uint32_t dst_storage, uint32_t dst_parent,
                                   const std::string& dst_label) {
    DBG("InDeviceCopyMoveTo: items=%d move=%d dst_storage=%u dst_parent=%u\n",
        items_number, move ? 1 : 0, dst_storage, dst_parent);

    // Pre-list destination folder for collision detection.
    std::map<std::string, proto::ObjectEntry> dst_existing;
    {
        auto kids = _backend->ListChildren(dst_storage, dst_parent);
        if (kids.ok) {
            for (const auto& k : kids.value) dst_existing[k.name] = k;
        }
    }

    std::vector<DeviceCopyItem> work;
    work.reserve(items_number);
    bool overwrite_all = false, skip_all = false, newer_all = false, rename_all = false;
    for (int i = 0; i < items_number; ++i) {
        std::string token;
        if (!ResolvePanelToken(panel_item[i], token)) continue;
        uint32_t h = 0;
        if (!ParseObjectToken(token, h)) continue;
        auto stat = _backend->Stat(h);
        if (!stat.ok || stat.value.name == "." || stat.value.name == "..") continue;

        DeviceCopyItem it;
        it.handle = h;
        it.stat = stat.value;
        it.rstats = stat.value.is_dir
            ? ComputeRecursiveStats(stat.value.storage_id, stat.value.handle)
            : RecursiveStats{stat.value.size, 1};
        // Case-aware on FAT; strict on ext4.
        const proto::ObjectEntry* existing = FindExistingByName(
            dst_existing, stat.value.name, dst_storage);
        if (existing && existing->handle != h) {
            uint64_t dst_show = existing->is_dir
                ? ComputeRecursiveSize(existing->storage_id, existing->handle)
                : existing->size;
            auto view_new = stat.value.is_dir ? OverwriteDialog::ViewFn{}
                : ViewDeviceFileCallback(stat.value.handle, stat.value.name);
            auto view_existing = existing->is_dir ? OverwriteDialog::ViewFn{}
                : ViewDeviceFileCallback(existing->handle, existing->name);
            auto d = OverwriteDialog::AskSticky(StrMB2Wide(stat.value.name),
                it.rstats.size, static_cast<int64_t>(stat.value.mtime_epoch),
                dst_show, static_cast<int64_t>(existing->mtime_epoch),
                overwrite_all, skip_all, newer_all, rename_all,
                std::move(view_new), std::move(view_existing));
            if (d == OverwriteDialog::D_CANCEL) return -1;
            if (d == OverwriteDialog::D_OVERWRITE) it.overwrite = true;
            else if (d == OverwriteDialog::D_RENAME)
                it.target_name = FindFreeName(dst_existing, stat.value.name, dst_storage);
            else it.skip = true;
        }
        work.push_back(std::move(it));
    }

    // First pass: device-side primitive with aside-rename overwrite — items returning Unsupported queue for host fallback.
    std::vector<HostFallbackItem> fallback;
    size_t first_pass_ok = 0;
    proto::Status hard_err = proto::OkStatus();
    {
        const DeviceCopyCtx ctx{this, &fallback, &hard_err, &dst_existing,
                                move, dst_storage, dst_parent};

        std::vector<WorkUnit> units;
        units.reserve(work.size());
        for (auto& it_ref : work) {
            units.push_back(MakeDeviceCopyMoveUnit(it_ref, it_ref.rstats, ctx));
        }
        const std::wstring src_w = StrMB2Wide(BuildPanelRelativePath());
        const std::wstring dst_w = dst_label.empty() ? src_w : StrMB2Wide(dst_label);
        auto br = RunBatch(/*is_move=*/move, /*is_upload=*/false,
                           src_w, dst_w, std::move(units));
        first_pass_ok = static_cast<size_t>(br.success_count);
        if (br.aborted && first_pass_ok == 0 && fallback.empty()) return -1;
    }
    if (!hard_err.ok) {
        SetErrorFromStatus(hard_err);
        MTPDialogs::MessageWrapped(FMSG_WARNING | FMSG_MB_OK,
            move ? Lng(MMoveFailed) : Lng(MCopyFailed),
            StrMB2Wide(CleanErrorMessage(hard_err.message)));
        return FALSE;
    }

    size_t fallback_ok = 0;
    if (!fallback.empty()) {
        DBG("InDeviceCopyMoveTo: host-mediated fallback for %zu item(s)\n", fallback.size());
        auto fb = RunHostFallbackBatch(fallback, move, &fallback_ok);
        if (!fb.ok && fb.code != proto::ErrorCode::Cancelled && fallback_ok == 0) {
            SetErrorFromStatus(fb);
            MTPDialogs::MessageWrapped(FMSG_WARNING | FMSG_MB_OK,
                move ? Lng(MMoveFailed) : Lng(MCopyFailed),
                StrMB2Wide(CleanErrorMessage(fb.message)));
            return FALSE;
        }
    }

    g_Info.Control(PANEL_ACTIVE, FCTL_CLEARSELECTION, 0, 0);
    g_Info.Control(PANEL_ACTIVE, FCTL_UPDATEPANEL, 0, 0);
    g_Info.Control(PANEL_ACTIVE, FCTL_REDRAWPANEL, 0, 0);
    DBG("InDeviceCopyMoveTo: done first_pass_ok=%zu fallback_ok=%zu\n",
        first_pass_ok, fallback_ok);
    return (first_pass_ok + fallback_ok) > 0 ? TRUE : FALSE;
}

MTPPlugin::PathResolution MTPPlugin::ResolveNewNamePath(const std::string& input,
                                                        const std::string& src_name,
                                                        bool auto_create_dirs,
                                                        const ResolveBase& base) {
    // Caller-supplied base for cross-panel; active panel by default.
    const uint32_t ctx_storage = base.storage_id ? base.storage_id : _current_storage_id;
    const uint32_t ctx_parent  = base.storage_id ? base.parent     : _current_parent;
    const std::string& ctx_storage_name =
        base.storage_id ? base.storage_name : _current_storage_name;
    PathResolution out;
    out.storage_id = ctx_storage;
    out.parent = ctx_parent;
    if (input.empty()) return out;

    // Trailing slash → destination is a folder; basename = src_name.
    const bool trailing_sep = (input.back() == '/' || input.back() == '\\');

    std::vector<std::string> parts;
    std::string cur;
    for (char c : input) {
        if (c == '/' || c == '\\') {
            if (!cur.empty()) parts.push_back(std::move(cur));
            cur.clear();
        } else {
            cur.push_back(c);
        }
    }
    if (!cur.empty()) parts.push_back(std::move(cur));
    if (parts.empty()) return out;

    // Storage-rooted: leading "Internal/..."/"External/..." walks from storage root so prompt prefills round-trip cleanly.
    if (!parts.empty() && !ctx_storage_name.empty()
            && parts.front() == ctx_storage_name) {
        out.parent = kRootParent;  // walk from storage root
        parts.erase(parts.begin());
        if (parts.empty()) {
            // Bare "Internal" → storage root, basename = src_name.
            if (src_name.empty()) return out;
            out.basename = src_name;
            out.ok = true;
            return out;
        }
    }

    // 1=handled, 0=not dot-special, -1=can't (e.g. ".." at root).
    int dotdot_depth = 0;
    constexpr int kMaxDotDotDepth = 64;  // pathological / cyclic device guard
    auto consume_dot_dot_dot = [&](const std::string& p) -> int {
        if (p == "..") {
            if (out.parent == 0 || out.parent == kRootParent) return -1;
            if (++dotdot_depth > kMaxDotDotDepth) return -1;  // cycle break
            auto st = _backend->Stat(out.parent);
            if (!st.ok) return -1;
            out.parent = (st.value.parent == 0) ? kRootParent : st.value.parent;
            return 1;
        }
        if (p == "." || p.empty()) return 1;
        return 0;
    };

    auto walk_into_dir = [&](const std::string& p) -> bool {
        auto kids = _backend->ListChildren(out.storage_id, out.parent);
        if (!kids.ok) return false;
        for (const auto& k : kids.value) {
            if (k.is_dir && _backend->NamesEqual(out.storage_id, k.name, p)) {
                out.parent = k.handle;
                return true;
            }
        }
        return false;
    };

    auto create_or_walk = [&](const std::string& p) -> bool {
        if (walk_into_dir(p)) return true;
        if (!auto_create_dirs) return false;
        auto md = _backend->MakeDirectory(p, out.storage_id, out.parent);
        if (!md.ok) return false;
        out.parent = md.value;
        return true;
    };

    // Walk all parts except last; auto-create intermediates if allowed.
    for (size_t i = 0; i + 1 < parts.size(); ++i) {
        int r = consume_dot_dot_dot(parts[i]);
        if (r == 1) continue;
        if (r == -1) return out;
        if (!create_or_walk(parts[i])) return out;
    }

    const std::string& last = parts.back();
    int last_special = consume_dot_dot_dot(last);
    if (last_special == -1) return out;

    // ".." / "." / trailing-slash → folder-only target.
    if (last_special == 1 || trailing_sep) {
        // Trailing slash on a real name: walk/create into it.
        if (trailing_sep && last != "." && last != ".." && !last.empty()) {
            if (!create_or_walk(last)) return out;
        }
        if (src_name.empty()) return out;  // caller didn't tell us the source name
        out.basename = src_name;
        out.ok = true;
        return out;
    }

    // Existing dir as last component → "into temp/" not "rename to temp".
    {
        auto kids = _backend->ListChildren(out.storage_id, out.parent);
        if (kids.ok) {
            for (const auto& k : kids.value) {
                if (k.is_dir && _backend->NamesEqual(out.storage_id, k.name, last)) {
                    if (src_name.empty()) return out;
                    out.parent = k.handle;
                    out.basename = src_name;
                    out.ok = true;
                    return out;
                }
            }
        }
    }

    // Last component is a new basename (rename / new file name).
    if (last == "." || last == "..") return out;  // shouldn't reach here, but be safe
    out.basename = last;
    out.ok = !out.basename.empty();
    return out;
}

bool MTPPlugin::ShiftF5CopyInPlace() {
    if (!_backend || !_backend->IsReady() || _view_mode != ViewMode::Objects) {
        return false;
    }

    auto handles = CollectActivePanelHandles();
    if (handles.empty()) return false;

    const bool multi = handles.size() > 1;

    uint32_t dst_storage = _current_storage_id;
    uint32_t dst_parent  = _current_parent;

    // Single: prefill "<path>/<name>.copy"; multi: "<path>/" folder-only — same-folder multi auto-suffixes ".copy" per item.
    std::string single_new_name;
    {
        std::string prefill;
        std::string resolve_src_name;
        if (multi) {
            prefill = BuildPanelRelativePath();
            resolve_src_name = "_";  // placeholder; per-item loop uses real names.
        } else {
            auto stat = _backend->Stat(handles[0]);
            if (!stat.ok) return false;
            prefill = BuildPanelRelativePath() + stat.value.name + ".copy";
            resolve_src_name = stat.value.name;
        }
        std::string typed;
        const std::wstring copy_prompt = std::wstring(Lng(MPromptCopyTitle)) + L" " + Lng(MPromptToColon);
        if (!MTPDialogs::AskInput(Lng(MPromptCopyTitle), copy_prompt.c_str(), L"MTP_Copy",
                                   typed, prefill)) {
            return true;  // user cancelled
        }
        if (typed.empty()) return true;
        auto resolved = ResolveNewNamePath(
            typed, resolve_src_name, /*auto_create_dirs=*/true);
        if (!resolved.ok) {
            MTPDialogs::MessageWrapped(FMSG_WARNING | FMSG_MB_OK,
                Lng(MCopyFailed),
                Lng(MErrResolveDstPath));
            return true;
        }
        dst_storage = resolved.storage_id;
        dst_parent  = resolved.parent;
        if (!multi) single_new_name = resolved.basename;
    }
    // Multi-select: ".copy" suffix when dst == src folder, else original names; per-item loop below branches on this flag.
    const bool same_dir_as_source = (dst_storage == _current_storage_id
                                     && dst_parent  == _current_parent);

    // Pre-list the destination folder for collision detection.
    std::map<std::string, proto::ObjectEntry> dst_existing;
    {
        auto kids = _backend->ListChildren(dst_storage, dst_parent);
        if (kids.ok) {
            for (const auto& k : kids.value) dst_existing[k.name] = k;
        }
    }

    struct Item {
        uint32_t handle;
        proto::ObjectEntry stat;
        std::string new_name;
        RecursiveStats rstats;
        bool skip = false;
        bool overwrite = false;
    };
    std::vector<Item> work;
    work.reserve(handles.size());
    {
        bool overwrite_all = false, skip_all = false, newer_all = false, rename_all = false;
        for (uint32_t h : handles) {
            auto stat = _backend->Stat(h);
            if (!stat.ok) continue;
            // single: resolver-supplied basename; multi: source names (".copy" suffix when dst==source dir to avoid collision).
            std::string new_name;
            if (!multi) {
                new_name = single_new_name.empty() ? stat.value.name : single_new_name;
            } else if (same_dir_as_source) {
                new_name = stat.value.name + ".copy";
            } else {
                new_name = stat.value.name;
            }
            Item it;
            it.handle = h;
            it.stat = stat.value;
            it.new_name = new_name;
            it.rstats = stat.value.is_dir
                ? ComputeRecursiveStats(stat.value.storage_id, stat.value.handle)
                : RecursiveStats{stat.value.size, 1};
            const proto::ObjectEntry* existing = FindExistingByName(
                dst_existing, new_name, dst_storage);
            if (existing) {
                uint64_t dst_show = existing->is_dir
                    ? ComputeRecursiveSize(existing->storage_id, existing->handle)
                    : existing->size;
                auto view_new = stat.value.is_dir ? OverwriteDialog::ViewFn{}
                    : ViewDeviceFileCallback(stat.value.handle, stat.value.name);
                auto view_existing = existing->is_dir ? OverwriteDialog::ViewFn{}
                    : ViewDeviceFileCallback(existing->handle, existing->name);
                auto d = OverwriteDialog::AskSticky(StrMB2Wide(new_name),
                    it.rstats.size, static_cast<int64_t>(stat.value.mtime_epoch),
                    dst_show, static_cast<int64_t>(existing->mtime_epoch),
                    overwrite_all, skip_all, newer_all, rename_all,
                    std::move(view_new), std::move(view_existing));
                if (d == OverwriteDialog::D_CANCEL) return true;
                if (d == OverwriteDialog::D_OVERWRITE) it.overwrite = true;
                else if (d == OverwriteDialog::D_RENAME)
                    it.new_name = FindFreeName(dst_existing, new_name, dst_storage);
                else it.skip = true;
            }
            work.push_back(std::move(it));
        }
    }

    // First pass: device CopyObject + aside-rename overwrite; Unsupported items queue for host fallback.
    std::vector<HostFallbackItem> fallback;
    proto::Status hard_err = proto::OkStatus();
    {
        struct Ctx {
            std::vector<HostFallbackItem>* fallback;
            proto::Status* hard_err;
            std::map<std::string, proto::ObjectEntry>* dst_existing;
            uint32_t dst_storage;
            uint32_t dst_parent;
        };
        const Ctx ctx{&fallback, &hard_err, &dst_existing, dst_storage, dst_parent};

        std::vector<WorkUnit> units;
        units.reserve(work.size());
        for (auto& it_ref : work) {
            const RecursiveStats stats = it_ref.rstats;
            Item it = it_ref;
            WorkUnit u;
            u.display_name = StrMB2Wide(it.new_name);
            u.is_directory = it.stat.is_dir;
            u.total_bytes = stats.size;
            u.total_files = std::max<uint64_t>(stats.count, 1);
            u.execute = [this, it, stats, ctx](ProgressTracker& tr) -> int {
                if (it.skip) { tr.MarkSkipped(); return 0; }
                // Opaque libmtp op — pin the bar so the modal stays alive.
                tr.PinNearDone(it.new_name);
                auto cp = _backend->CopyObject(it.handle, ctx.dst_storage, ctx.dst_parent);
                if (!cp.ok && cp.code == proto::ErrorCode::Unsupported) {
                    if (it.overwrite) {
                        auto del = _backend->Delete((*ctx.dst_existing)[it.new_name].handle, /*recursive=*/true);
                        if (!del.ok) DBG("Unsupported-fallback dst-existing delete failed for '%s': %s\n",
                                         it.new_name.c_str(), del.message.c_str());
                        ctx.dst_existing->erase(it.new_name);
                    }
                    ctx.fallback->push_back({it.handle, it.stat, ctx.dst_storage,
                                             ctx.dst_parent, it.new_name, stats.size,
                                             std::max<uint64_t>(stats.count, 1)});
                    tr.MarkSkipped();
                    return 0;
                }
                if (!cp.ok) {
                    *ctx.hard_err = proto::Status::Failure(cp.code, cp.message);
                    tr.HaltBatch();
                    return MapErrorToErrno(cp.code);
                }
                if (it.overwrite) {
                    auto& existing = (*ctx.dst_existing)[it.new_name];
                    const std::string aside = AsideName(it.new_name);
                    auto aside_rn = _backend->Rename(existing.handle, aside);
                    if (!aside_rn.ok) {
                        auto del = _backend->Delete(cp.value, /*recursive=*/true);
                        if (!del.ok) DBG("rollback delete of new copy failed: %s\n", del.message.c_str());
                        *ctx.hard_err = aside_rn;
                        tr.HaltBatch();
                        return MapErrorToErrno(aside_rn);
                    }
                    auto final_rn = _backend->Rename(cp.value, it.new_name);
                    if (!final_rn.ok) {
                        auto del = _backend->Delete(cp.value, /*recursive=*/true);
                        if (!del.ok) DBG("rollback delete of new copy failed: %s\n", del.message.c_str());
                        auto rest = _backend->Rename(existing.handle, it.new_name);
                        if (!rest.ok) DBG("CRITICAL aside-restore failed: %s — '%s' now named '%s.far2l-tmp.%d', manual recovery needed\n",
                                          rest.message.c_str(), it.new_name.c_str(), it.new_name.c_str(), getpid());
                        *ctx.hard_err = final_rn;
                        tr.HaltBatch();
                        return MapErrorToErrno(final_rn);
                    }
                    auto del = _backend->Delete(existing.handle, /*recursive=*/true);
                    if (!del.ok) DBG("aside cleanup after success failed: %s — stale '%s.far2l-tmp.%d' remains\n",
                                     del.message.c_str(), it.new_name.c_str(), getpid());
                    ctx.dst_existing->erase(it.new_name);
                } else if (it.new_name != it.stat.name) {
                    auto rn = _backend->Rename(cp.value, it.new_name);
                    if (!rn.ok) {
                        auto del = _backend->Delete(cp.value, /*recursive=*/true);
                        if (!del.ok) DBG("rollback delete of new copy failed: %s\n", del.message.c_str());
                        *ctx.hard_err = rn;
                        tr.HaltBatch();
                        return MapErrorToErrno(rn);
                    }
                }
                return 0;
            };
            units.push_back(std::move(u));
        }
        RunBatch(/*is_move=*/false, /*is_upload=*/false,
                 StrMB2Wide(BuildPanelRelativePath()),
                 StrMB2Wide(BuildPanelRelativePath()),
                 std::move(units));
    }
    if (!hard_err.ok) {
        SetErrorFromStatus(hard_err);
        MTPDialogs::MessageWrapped(FMSG_WARNING | FMSG_MB_OK,
            Lng(MCopyFailed), StrMB2Wide(CleanErrorMessage(hard_err.message)));
        return true;
    }

    if (!fallback.empty()) {
        size_t fb_ok = 0;
        auto fb = RunHostFallbackBatch(fallback, /*move=*/false, &fb_ok);
        if (!fb.ok && fb.code != proto::ErrorCode::Cancelled && fb_ok == 0) {
            SetErrorFromStatus(fb);
            MTPDialogs::MessageWrapped(FMSG_WARNING | FMSG_MB_OK,
                Lng(MCopyFailed), StrMB2Wide(CleanErrorMessage(fb.message)));
        }
    }

    g_Info.Control(PANEL_ACTIVE, FCTL_CLEARSELECTION, 0, 0);
    RefreshPanel();
    return true;
}

bool MTPPlugin::RenameSelectedItem() {
    // Single-item rename only (Far convention) — multi-item moves go via F6 cross-panel; no rename-mask dialogs.
    if (_view_mode != ViewMode::Objects || !_backend || !_backend->IsReady()) {
        return false;
    }

    std::string token;
    if (!GetSelectedPanelUserData(token)) return false;
    uint32_t handle = 0;
    if (!ParseObjectToken(token, handle)) return false;
    std::string old_name;
    if (!GetSelectedPanelFileName(old_name)) old_name.clear();
    if (old_name == "." || old_name == "..") return false;

    // Prefill: basename only (parity with ADB + stock far2l Shift+F6). Resolver still accepts "../" / "subdir/" if user types them.
    const std::string prefill = old_name;
    std::string raw_input;
    if (!PromptInput(Lng(MPromptRenameTitle),
                     Lng(MPromptRenameTo),
                     L"MTP_Rename",
                     prefill,
                     raw_input)) {
        return true;
    }
    if (raw_input.empty() || raw_input == prefill) return true;

    // Always resolve through ResolveNewNamePath — no-op for plain "newname"; handles "..", "./sub/", existing-dir-as-target.
    auto resolved = ResolveNewNamePath(raw_input, old_name);
    if (!resolved.ok) {
        MTPDialogs::MessageWrapped(FMSG_WARNING | FMSG_MB_OK,
            Lng(MRenameFailed),
            Lng(MErrResolveDstPath));
        return true;
    }
    uint32_t dst_storage = resolved.storage_id;
    uint32_t dst_parent  = resolved.parent;
    std::string new_name = resolved.basename;
    if (new_name == old_name && dst_parent == _current_parent && dst_storage == _current_storage_id) {
        return true;
    }

    // Cross-folder move: try MoveObject; on Unsupported, fall back to download → upload → delete-source via host.
    if (dst_parent != _current_parent || dst_storage != _current_storage_id) {
        auto src_stat = _backend->Stat(handle);
        if (!src_stat.ok) {
            SetErrorFromStatus(proto::Status::Failure(src_stat.code, src_stat.message));
            return false;
        }
        // Quick collision check at destination.
        auto dst_kids = _backend->ListChildren(dst_storage, dst_parent);
        if (dst_kids.ok) {
            for (const auto& k : dst_kids.value) {
                if (_backend->NamesEqual(_current_storage_id, k.name, new_name) && k.handle != handle) {
                    uint64_t src_show = src_stat.value.is_dir
                        ? ComputeRecursiveSize(src_stat.value.storage_id, src_stat.value.handle)
                        : src_stat.value.size;
                    uint64_t dst_show = k.is_dir
                        ? ComputeRecursiveSize(k.storage_id, k.handle)
                        : k.size;
                    OverwriteDialog dlg(StrMB2Wide(new_name),
                                        src_show,
                                        static_cast<int64_t>(src_stat.value.mtime_epoch),
                                        dst_show,
                                        static_cast<int64_t>(k.mtime_epoch));
                    auto choice = dlg.Ask();
                    if (choice == OverwriteDialog::SKIP || choice == OverwriteDialog::SKIP_ALL
                        || choice == OverwriteDialog::CANCEL) {
                        return true;
                    }
                    auto del = _backend->Delete(k.handle, /*recursive=*/true);
                    if (!del.ok) DBG("cross-folder overwrite delete failed for '%s': %s\n",
                                     k.name.c_str(), del.message.c_str());
                    break;
                }
            }
        }

        auto mv = _backend->MoveObject(handle, dst_storage, dst_parent);
        if (mv.ok) {
            // Rename to new basename if it differs from source.
            if (new_name != old_name) {
                auto rn = _backend->Rename(handle, new_name);
                if (!rn.ok) {
                    DBG("Rename: cross-folder move ok but rename failed: %s\n", rn.message.c_str());
                }
            }
            RefreshPanel();
            return true;
        }
        if (mv.code == proto::ErrorCode::Unsupported) {
            DBG("Rename: MoveObject unsupported, falling back to host-mediated move\n");
            std::vector<HostFallbackItem> fb_items;
            fb_items.push_back({handle, src_stat.value, dst_storage, dst_parent, new_name});
            size_t fb_ok = 0;
            auto fb = RunHostFallbackBatch(fb_items, /*move=*/true, &fb_ok);
            if (!fb.ok && fb_ok == 0) {
                SetErrorFromStatus(fb);
                MTPDialogs::MessageWrapped(FMSG_WARNING | FMSG_MB_OK,
                    Lng(MRenameFailed), StrMB2Wide(CleanErrorMessage(fb.message)));
            }
            RefreshPanel();
            return true;
        }
        SetErrorFromStatus(mv);
        MTPDialogs::MessageWrapped(FMSG_WARNING | FMSG_MB_OK,
            Lng(MRenameFailed), StrMB2Wide(CleanErrorMessage(mv.message)));
        return true;
    }

    // In-folder rename — original logic, with atomic-ish overwrite.
    auto kids = _backend->ListChildren(_current_storage_id, _current_parent);
    if (kids.ok) {
        for (const auto& k : kids.value) {
            if (_backend->NamesEqual(_current_storage_id, k.name, new_name) && k.handle != handle) {
                auto src_st = _backend->Stat(handle);
                uint64_t src_show = 0;
                int64_t  src_mt   = 0;
                if (src_st.ok) {
                    src_show = src_st.value.is_dir
                        ? ComputeRecursiveSize(src_st.value.storage_id, src_st.value.handle)
                        : src_st.value.size;
                    src_mt = static_cast<int64_t>(src_st.value.mtime_epoch);
                }
                uint64_t dst_show = k.is_dir
                    ? ComputeRecursiveSize(k.storage_id, k.handle)
                    : k.size;
                OverwriteDialog dlg(StrMB2Wide(new_name),
                                    src_show, src_mt,
                                    dst_show, static_cast<int64_t>(k.mtime_epoch));
                auto choice = dlg.Ask();
                if (choice == OverwriteDialog::SKIP || choice == OverwriteDialog::SKIP_ALL) {
                    return true;
                }
                if (choice == OverwriteDialog::CANCEL) return true;
                // Aside-rename overwrite: restore on failure, no data loss.
                std::string aside = AsideName(k.name);
                auto rn = _backend->Rename(k.handle, aside);
                if (!rn.ok) {
                    SetErrorFromStatus(rn);
                    MTPDialogs::MessageWrapped(FMSG_WARNING | FMSG_MB_OK,
                        Lng(MRenameFailed), StrMB2Wide(CleanErrorMessage(rn.message)));
                    return true;
                }
                auto final_rn = _backend->Rename(handle, new_name);
                if (!final_rn.ok) {
                    // Restore the aside copy so user doesn't lose data.
                    auto rest = _backend->Rename(k.handle, k.name);
                    if (!rest.ok) DBG("CRITICAL aside-restore failed: %s — '%s' now named '%s' (aside), manual recovery needed\n",
                                      rest.message.c_str(), k.name.c_str(), aside.c_str());
                    SetErrorFromStatus(final_rn);
                    MTPDialogs::MessageWrapped(FMSG_WARNING | FMSG_MB_OK,
                        Lng(MRenameFailed), StrMB2Wide(CleanErrorMessage(final_rn.message)));
                    return true;
                }
                // Final rename succeeded; clean up the aside copy.
                auto del = _backend->Delete(k.handle, /*recursive=*/true);
                if (!del.ok) DBG("aside cleanup after success failed: %s — stale '%s' remains\n",
                                 del.message.c_str(), aside.c_str());
                RefreshPanel();
                return true;
            }
        }
    }

    auto st = _backend->Rename(handle, new_name);
    if (st.ok) {
        RefreshPanel();
        return true;
    }
    // Some firmwares refuse Set_Object_Filename on folders — fall back to CopyObject(new_name) + Delete original (device-side).
    bool tried_fallback = false;
    if (st.code == proto::ErrorCode::Unsupported) {
        auto stat_now = _backend->Stat(handle);
        if (stat_now.ok && stat_now.value.is_dir
                && _backend->SupportsCopyObject()) {
            DBG("Rename: Set_File_Name unsupported on folder; trying CopyObject + Delete\n");
            auto cp = _backend->CopyObject(handle, _current_storage_id, _current_parent);
            if (cp.ok) {
                auto rn = _backend->Rename(cp.value, new_name);
                if (rn.ok) {
                    auto del = _backend->Delete(handle, /*recursive=*/true);
                    if (!del.ok) DBG("CopyObject+Rename succeeded but original delete failed: %s — duplicate folder remains\n",
                                     del.message.c_str());
                    RefreshPanel();
                    return true;
                }
                auto del = _backend->Delete(cp.value, /*recursive=*/true);
                if (!del.ok) DBG("rollback delete of copied folder failed: %s\n", del.message.c_str());
            }
            tried_fallback = true;
        }
    }
    SetErrorFromStatus(st);
    MTPDialogs::MessageWrapped(FMSG_WARNING | FMSG_MB_OK,
        Lng(MRenameFailed),
        tried_fallback
            ? (StrMB2Wide(CleanErrorMessage(st.message)) + L"\n" + Lng(MErrFallbackFailed))
            : StrMB2Wide(CleanErrorMessage(st.message)));
    return false;
}

bool MTPPlugin::CrossPanelCopyMoveSameDevice(bool move) {
    if (PluginFromPanel(PANEL_ACTIVE) != this) {
        return false;
    }
    MTPPlugin* dst = PluginFromPanel(PANEL_PASSIVE);
    if (!dst || dst == this) {
        return false;
    }

    if (!_backend || !_backend->IsReady() || !dst->_backend || !dst->_backend->IsReady()) {
        return false;
    }
    if (_view_mode != ViewMode::Objects || dst->_view_mode != ViewMode::Objects) {
        return false;
    }
    if (_current_device_key.empty() || _current_device_key != dst->_current_device_key) {
        DBG("CrossPanelCopyMoveSameDevice skip: different devices src=%s dst=%s\n",
            _current_device_key.c_str(), dst->_current_device_key.c_str());
        return false;
    }
    if (_current_storage_id == 0 || dst->_current_storage_id == 0) {
        return false;
    }

    std::string firstName;
    auto handles = CollectActivePanelHandles(&firstName);

    if (handles.empty()) {
        DBG("CrossPanelCopyMoveSameDevice skip: no selected object handles\n");
        return false;
    }
    DBG("CrossPanelCopyMoveSameDevice op=%s count=%zu dst_storage=%u dst_parent=%u\n",
        move ? "move" : "copy", handles.size(), dst->_current_storage_id, dst->_current_parent);

    // Confirmation+path-edit prompt; same UX as Shift+F5/F6 — prefill=passive panel's storage-relative path; resolver runs vs dst.
    uint32_t dst_storage = dst->_current_storage_id;
    uint32_t dst_parent  = dst->_current_parent;
    std::string user_path;
    {
        const wchar_t* title = move ? Lng(MPromptMoveTitle) : Lng(MPromptCopyTitle);
        const std::wstring verb_sp = std::wstring(title) + L" ";
        const std::wstring to_colon = std::wstring(L" ") + Lng(MPromptToColon);
        std::wstring prompt;
        if (handles.size() == 1) {
            prompt = verb_sp + StrMB2Wide(firstName) + to_colon;
        } else {
            prompt = verb_sp + std::to_wstring(handles.size()) + L" "
                   + Lng(MPromptItems) + to_colon;
        }
        const std::string prefill = dst->BuildPanelRelativePath();
        if (!MTPDialogs::AskInput(title, prompt.c_str(), L"MTP_CopyMove",
                                   user_path, prefill)) {
            return true;  // user cancelled — claim keystroke
        }
        if (user_path.empty()) return true;
        // Relative to dst panel; multi-select ignores resolver basename (per-item names supplied below).
        const std::string src_name_for_resolve =
            (handles.size() == 1) ? firstName : std::string();
        auto resolved = ResolveNewNamePath(
            user_path, src_name_for_resolve, /*auto_create_dirs=*/true,
            ResolveBase{dst->_current_storage_id, dst->_current_parent,
                        dst->_current_storage_name});
        if (!resolved.ok) {
            MTPDialogs::MessageWrapped(FMSG_WARNING | FMSG_MB_OK,
                move ? Lng(MMoveFailed) : Lng(MCopyFailed),
                Lng(MErrResolveDstFolders));
            return true;
        }
        dst_storage = resolved.storage_id;
        dst_parent  = resolved.parent;
        // Single-item rename intent: bare new basename + no trailing slash → run as Rename (or MoveObject/CopyObject + Rename for cross-folder).
        const bool trailing_sep = !user_path.empty()
            && (user_path.back() == '/' || user_path.back() == '\\');
        if (handles.size() == 1 && !resolved.basename.empty()
                && resolved.basename != firstName && !trailing_sep) {
            const bool same_parent = (dst_storage == _current_storage_id
                                      && dst_parent == _current_parent);
            // Collision short-circuit: if new name already exists in dst, fall through to the batch path which has full overwrite UI.
            auto kids = dst->_backend->ListChildren(dst_storage, dst_parent);
            bool collision = false;
            if (kids.ok) {
                for (const auto& k : kids.value) {
                    if (dst->_backend->NamesEqual(dst_storage, k.name, resolved.basename)) {
                        collision = true;
                        break;
                    }
                }
            }
            if (!collision) {
                if (move) {
                    if (!same_parent) {
                        auto mv = _backend->MoveObject(handles[0], dst_storage, dst_parent);
                        if (!mv.ok) {
                            // Unsupported → fall through to batch (host fallback).
                            if (mv.code != proto::ErrorCode::Unsupported) {
                                SetErrorFromStatus(mv);
                                MTPDialogs::MessageWrapped(FMSG_WARNING | FMSG_MB_OK,
                                    Lng(MMoveFailed), StrMB2Wide(CleanErrorMessage(mv.message)));
                                return true;
                            }
                            // Fall through.
                            goto skip_rename_intent;
                        }
                    }
                    auto rn = _backend->Rename(handles[0], resolved.basename);
                    if (!rn.ok && same_parent) {
                        SetErrorFromStatus(rn);
                        MTPDialogs::MessageWrapped(FMSG_WARNING | FMSG_MB_OK,
                            Lng(MRenameFailed), StrMB2Wide(CleanErrorMessage(rn.message)));
                        return true;
                    }
                    if (!rn.ok) DBG("rename after move failed: %s\n", rn.message.c_str());
                } else {
                    auto cp = _backend->CopyObject(handles[0], dst_storage, dst_parent);
                    if (!cp.ok) {
                        if (cp.code != proto::ErrorCode::Unsupported) {
                            SetErrorFromStatus(proto::Status::Failure(cp.code, cp.message));
                            MTPDialogs::MessageWrapped(FMSG_WARNING | FMSG_MB_OK,
                                Lng(MCopyFailed), StrMB2Wide(CleanErrorMessage(cp.message)));
                            return true;
                        }
                        goto skip_rename_intent;
                    }
                    auto rn = _backend->Rename(cp.value, resolved.basename);
                    if (!rn.ok) DBG("rename after copy failed: %s\n", rn.message.c_str());
                }
                g_Info.Control(PANEL_ACTIVE, FCTL_CLEARSELECTION, 0, 0);
                g_Info.Control(PANEL_ACTIVE, FCTL_UPDATEPANEL, 0, 0);
                g_Info.Control(PANEL_ACTIVE, FCTL_REDRAWPANEL, 0, 0);
                return true;
            }
        }
    skip_rename_intent: ;
    }

    // We own collision UI: far2l's Copy/Move never sees this path.
    std::map<std::string, proto::ObjectEntry> dst_existing;
    {
        auto kids = dst->_backend->ListChildren(dst_storage, dst_parent);
        if (kids.ok) {
            for (const auto& k : kids.value) dst_existing[k.name] = k;
        }
    }

    std::vector<DeviceCopyItem> work;
    work.reserve(handles.size());
    {
        bool overwrite_all = false, skip_all = false, newer_all = false, rename_all = false;
        for (uint32_t h : handles) {
            auto stat = _backend->Stat(h);
            if (!stat.ok) continue;
            DeviceCopyItem it;
            it.handle = h;
            it.stat = stat.value;
            it.rstats = stat.value.is_dir
                ? ComputeRecursiveStats(stat.value.storage_id, stat.value.handle)
                : RecursiveStats{stat.value.size, 1};
            const proto::ObjectEntry* existing = FindExistingByName(
                dst_existing, stat.value.name, dst_storage);
            if (existing) {
                uint64_t dst_show = existing->is_dir
                    ? dst->ComputeRecursiveSize(existing->storage_id, existing->handle)
                    : existing->size;
                auto view_new = stat.value.is_dir ? OverwriteDialog::ViewFn{}
                    : ViewDeviceFileCallback(stat.value.handle, stat.value.name);
                auto view_existing = existing->is_dir ? OverwriteDialog::ViewFn{}
                    : dst->ViewDeviceFileCallback(existing->handle, existing->name);
                auto d = OverwriteDialog::AskSticky(StrMB2Wide(stat.value.name),
                    it.rstats.size, static_cast<int64_t>(stat.value.mtime_epoch),
                    dst_show, static_cast<int64_t>(existing->mtime_epoch),
                    overwrite_all, skip_all, newer_all, rename_all,
                    std::move(view_new), std::move(view_existing));
                if (d == OverwriteDialog::D_CANCEL) return true;
                if (d == OverwriteDialog::D_OVERWRITE) it.overwrite = true;
                else if (d == OverwriteDialog::D_RENAME)
                    it.target_name = dst->FindFreeName(dst_existing, stat.value.name, dst_storage);
                else it.skip = true;
            }
            work.push_back(std::move(it));
        }
    }

    // First pass: device CopyObject/MoveObject with aside-rename overwrite + restore-on-failure.
    std::vector<HostFallbackItem> fallback;
    size_t first_pass_ok = 0;
    proto::Status hard_err = proto::OkStatus();
    {
        const DeviceCopyCtx ctx{dst, &fallback, &hard_err, &dst_existing,
                                move, dst_storage, dst_parent};

        std::vector<WorkUnit> units;
        units.reserve(work.size());
        for (auto& it_ref : work) {
            units.push_back(MakeDeviceCopyMoveUnit(it_ref, it_ref.rstats, ctx));
        }
        auto br = RunBatch(/*is_move=*/move, /*is_upload=*/false,
                           StrMB2Wide(BuildPanelRelativePath()),
                           StrMB2Wide(dst->BuildPanelRelativePath()),
                           std::move(units));
        first_pass_ok = static_cast<size_t>(br.success_count);
        if (br.aborted && first_pass_ok == 0 && fallback.empty()) {
            return true;
        }
    }
    if (!hard_err.ok) {
        SetErrorFromStatus(hard_err);
        MTPDialogs::MessageWrapped(FMSG_WARNING | FMSG_MB_OK,
            move ? Lng(MMoveFailed) : Lng(MCopyFailed),
            StrMB2Wide(CleanErrorMessage(hard_err.message)));
        return true;
    }

    size_t fallback_ok = 0;
    if (!fallback.empty()) {
        DBG("CrossPanel: host-mediated fallback for %zu item(s)\n", fallback.size());
        auto fb = RunHostFallbackBatch(fallback, move, &fallback_ok);
        if (!fb.ok && fb.code != proto::ErrorCode::Cancelled && fallback_ok == 0) {
            SetErrorFromStatus(fb);
            MTPDialogs::MessageWrapped(FMSG_WARNING | FMSG_MB_OK,
                move ? Lng(MMoveFailed) : Lng(MCopyFailed),
                StrMB2Wide(CleanErrorMessage(fb.message)));
        }
    }

    g_Info.Control(PANEL_ACTIVE, FCTL_CLEARSELECTION, 0, 0);
    g_Info.Control(PANEL_ACTIVE, FCTL_UPDATEPANEL, 0, 0);
    g_Info.Control(PANEL_ACTIVE, FCTL_REDRAWPANEL, 0, 0);
    g_Info.Control(PANEL_PASSIVE, FCTL_UPDATEPANEL, 0, 0);
    g_Info.Control(PANEL_PASSIVE, FCTL_REDRAWPANEL, 0, 0);
    DBG("CrossPanelCopyMoveSameDevice done op=%s first_pass_ok=%zu fallback_ok=%zu\n",
        move ? "move" : "copy", first_pass_ok, fallback_ok);
    return true;
}

bool MTPPlugin::EnterSelectedItem() {
    std::string token;
    if (!GetSelectedPanelUserData(token)) {
        DBG("EnterSelectedItem no selected token\n");
        return false;
    }
    if (!StrStartsFrom(token, "DEV|") && !StrStartsFrom(token, "STO|") && !StrStartsFrom(token, "OBJ|")) {
        DBG("EnterSelectedItem invalid token=%s\n", token.c_str());
        return false;
    }
    DBG("EnterSelectedItem token=%s\n", token.c_str());
    return EnterByToken(token);
}

bool MTPPlugin::EnterByToken(const std::string& token) {
    DBG("EnterByToken token=%s mode=%d\n", token.c_str(), static_cast<int>(_view_mode));

    if (StrStartsFrom(token, "DEV|")) {
        const std::string key = token.substr(4);
        DBG("EnterByToken device key=%s\n", key.c_str());
        if (!EnsureConnected(key)) {
            DBG("EnterByToken device connect failed key=%s err=%s\n", key.c_str(), _last_error_message.c_str());
            std::wstring title = Lng(MCannotOpenDevice);
            std::wstring detail = _last_error_message.empty()
                ? std::wstring(Lng(MUnknownError))
                : StrMB2Wide(_last_error_message);
            std::vector<std::wstring> lines;
            lines.push_back(title);
            // Wrap long messages into chunks of <=70 wide chars so dialog doesn't blow past screen width.
            constexpr size_t kWrap = 70;
            for (size_t i = 0; i < detail.size(); i += kWrap) {
                lines.push_back(detail.substr(i, kWrap));
            }
            std::vector<const wchar_t*> ptrs;
            ptrs.reserve(lines.size());
            for (const auto& s : lines) ptrs.push_back(s.c_str());
            g_Info.Message(g_Info.ModuleNumber, FMSG_MB_OK | FMSG_WARNING, nullptr,
                           ptrs.data(), static_cast<int>(ptrs.size()), 0);
            return false;
        }
        auto it = _device_binds.find(key);
        _current_device_name.clear();
        _current_device_serial.clear();
        if (_backend) {
            _current_device_name = _backend->FriendlyName();
        }
        if (it != _device_binds.end()) {
            _current_device_serial = it->second.candidate.serial;
            if (_current_device_name.empty()) {
                _current_device_name = it->second.candidate.product;
            }
        }
        if (_current_device_name.empty()) {
            _current_device_name = "Storages";
        }
        _view_mode = ViewMode::Storages;
        _panel_title = StrMB2Wide(_current_device_name);
        _current_storage_name.clear();
        _dir_stack.clear();

        // Single-storage devices skip the Storages list; ".." returns there.
        if (_backend) {
            auto sl = _backend->ListStorages();
            if (sl.ok && sl.value.size() == 1) {
                const auto& only = sl.value.front();
                std::string label;
                switch (only.kind) {
                    case proto::StorageKind::Internal: label = "Internal"; break;
                    case proto::StorageKind::External: label = "External"; break;
                    default:                           label = "Storage";  break;
                }
                _current_storage_name = label;
                _current_storage_id   = only.id;
                _current_parent       = 0;
                _view_mode            = ViewMode::Objects;
                UpdateObjectsPanelTitle();
                return true;
            }
        }
        return true;
    }

    uint32_t storage = 0;
    if (ParseStorageToken(token, storage)) {
        DBG("EnterByToken storage id=%u\n", storage);
        std::string storage_label;
        for (const auto& kv : _name_token_index) {
            if (kv.second == token) {
                storage_label = kv.first;
                break;
            }
        }
        _current_storage_name = storage_label.empty() ? ("Storage " + std::to_string(storage)) : storage_label;
        _dir_stack.clear();
        _current_storage_id = storage;
        _current_parent = 0;
        _view_mode = ViewMode::Objects;
        UpdateObjectsPanelTitle();
        return true;
    }

    uint32_t handle = 0;
    if (ParseObjectToken(token, handle)) {
        DBG("EnterByToken object handle=%u\n", handle);
        auto st = _backend ? _backend->Stat(handle) : proto::Result<proto::ObjectEntry>{};
        DBG("EnterByToken object stat ok=%d code=%d is_dir=%d storage=%u parent=%u name=%s\n",
            st.ok ? 1 : 0,
            st.ok ? 0 : static_cast<int>(st.code),
            (st.ok && st.value.is_dir) ? 1 : 0,
            st.ok ? st.value.storage_id : 0,
            st.ok ? st.value.parent : 0,
            st.ok ? st.value.name.c_str() : "");
        if (st.ok && st.value.is_dir) {
            _current_storage_id = st.value.storage_id;
            _current_parent = handle;
            _view_mode = ViewMode::Objects;
            _dir_stack.push_back(st.value.name);
            UpdateObjectsPanelTitle();
            return true;
        }
    }

    DBG("EnterByToken unhandled token=%s\n", token.c_str());
    return false;
}

proto::Status MTPPlugin::DownloadRecursive(uint32_t storage_id,
                                           uint32_t handle,
                                           const std::string& local_root,
                                           proto::CancellationToken token,
                                           ProgressTracker* tr) {
    if (mkdir(local_root.c_str(), 0755) != 0 && errno != EEXIST) {
        return proto::Status::Failure(proto::ErrorCode::Io, "mkdir failed");
    }

    auto children = _backend->ListChildren(storage_id, handle);
    if (!children.ok) {
        return proto::Status::Failure(children.code, children.message, children.retryable);
    }

    for (const auto& child : children.value) {
        if (token.IsCancelled()) {
            return proto::Status::Failure(proto::ErrorCode::Cancelled, "Cancelled by user");
        }

        const std::string target = JoinPath(local_root, child.name);
        if (child.is_dir) {
            auto st = DownloadRecursive(child.storage_id, child.handle, target, token, tr);
            if (!st.ok) return st;
        } else {
            auto st = _backend->Download(child.handle, target,
                [tr, name = child.name](uint64_t sent, uint64_t fileTotal) {
                    if (tr) tr->Tick(sent, fileTotal, name);
                },
                token);
            if (!st.ok) return st;
            if (tr) tr->CompleteFile(child.size);
        }
    }

    return proto::OkStatus();
}

proto::Status MTPPlugin::UploadRecursive(const std::string& local_dir,
                                         const std::string& remote_name,
                                         uint32_t storage_id,
                                         uint32_t parent,
                                         proto::CancellationToken token,
                                         proto::CancellationSource* cancel_src,
                                         ProgressTracker* tr) {
    if (token.IsCancelled()) {
        return proto::Status::Failure(proto::ErrorCode::Cancelled, "Cancelled by user");
    }
    auto mk = _backend->MakeDirectory(remote_name, storage_id, parent);
    if (!mk.ok) {
        return proto::Status::Failure(mk.code, mk.message);
    }
    uint32_t newParent = mk.value;

    DIR* dir = opendir(local_dir.c_str());
    if (!dir) {
        return proto::Status::Failure(proto::ErrorCode::Io,
            "opendir failed: " + local_dir);
    }
    proto::Status final_status = proto::OkStatus();
    while (true) {
        if (token.IsCancelled()) {
            final_status = proto::Status::Failure(proto::ErrorCode::Cancelled, "Cancelled by user");
            break;
        }
        dirent* ent = readdir(dir);
        if (!ent) break;
        if (strcmp(ent->d_name, ".") == 0 || strcmp(ent->d_name, "..") == 0) continue;

        std::string child_path = local_dir + "/" + ent->d_name;
        struct stat ss{};
        if (lstat(child_path.c_str(), &ss) != 0) continue;

        if (S_ISDIR(ss.st_mode)) {
            auto st = UploadRecursive(child_path, ent->d_name, storage_id, newParent,
                                      token, cancel_src, tr);
            if (!st.ok) { final_status = st; break; }
        } else if (S_ISREG(ss.st_mode)) {
            uint64_t fsize = static_cast<uint64_t>(ss.st_size);
            auto st = _backend->Upload(child_path, ent->d_name, storage_id, newParent,
                [tr, cancel_src, name = std::string(ent->d_name)](uint64_t sent, uint64_t total) {
                    if (cancel_src && tr && tr->Aborted()) cancel_src->Cancel();
                    if (tr) tr->Tick(sent, total, name);
                }, token);
            if (st.ok) {
                if (tr) tr->CompleteFile(fsize);
            } else if (st.code != proto::ErrorCode::Cancelled) {
                final_status = st;
                break;
            }
        }
    }
    closedir(dir);
    return final_status;
}

proto::Status MTPPlugin::ManualCopyViaHostInline(uint32_t src_handle,
                                                  const proto::ObjectEntry& src_stat,
                                                  uint32_t dst_storage_id,
                                                  uint32_t dst_parent,
                                                  const std::string& new_name,
                                                  ProgressTracker& tr,
                                                  proto::CancellationToken token,
                                                  proto::CancellationSource* cancel_src) {
    if (token.IsCancelled()) {
        return proto::Status::Failure(proto::ErrorCode::Cancelled, "Cancelled by user");
    }
    const char* tmpenv = std::getenv("TMPDIR");
    std::string tmp_template = (tmpenv && *tmpenv) ? tmpenv : "/tmp";
    if (tmp_template.back() != '/') tmp_template += '/';
    tmp_template += "far2l-mtp-copy.XXXXXX";
    std::vector<char> tmpbuf(tmp_template.begin(), tmp_template.end());
    tmpbuf.push_back(0);
    if (!mkdtemp(tmpbuf.data())) {
        return proto::Status::Failure(proto::ErrorCode::Io,
            std::string("mkdtemp failed: ") + std::strerror(errno));
    }
    const std::string tmp_root(tmpbuf.data());
    DBG("ManualCopyViaHost(inline): tmp_root=%s src=%s is_dir=%d new_name=%s\n",
        tmp_root.c_str(), src_stat.name.c_str(),
        src_stat.is_dir ? 1 : 0, new_name.c_str());

    proto::Status status = proto::OkStatus();

    if (src_stat.is_dir) {
        // Pull phase display-only: bytes/count credited during push (each byte traverses USB twice); tracker still drives current_file + per-file bar so dialog isn't frozen during libmtp's download half.
        const std::string staging = tmp_root + "/" + src_stat.name;
        tr.SetDisplayOnly(true);
        status = DownloadRecursive(src_stat.storage_id, src_handle, staging, token, &tr);
        tr.SetDisplayOnly(false);
        tr.Reset();
        if (status.ok) {
            status = UploadRecursive(staging, new_name, dst_storage_id, dst_parent,
                                     token, cancel_src, &tr);
        }
    } else {
        const std::string staged = tmp_root + "/" + src_stat.name;
        status = _backend->Download(src_handle, staged,
            [&tr, cancel_src, name = src_stat.name](uint64_t sent, uint64_t fileTotal) {
                if (tr.Aborted() && cancel_src) cancel_src->Cancel();
                tr.TickDisplayOnly(sent, fileTotal, name);
            }, token);
        if (status.ok) {
            tr.Reset();
            status = _backend->Upload(staged, new_name, dst_storage_id, dst_parent,
                [&tr, cancel_src, name = new_name](uint64_t sent, uint64_t fileTotal) {
                    if (tr.Aborted() && cancel_src) cancel_src->Cancel();
                    tr.Tick(sent, fileTotal, name);
                }, token);
            if (status.ok) tr.CompleteFile(src_stat.size);
        }
    }

    RemoveLocalPathRecursively(tmp_root);
    DBG("ManualCopyViaHost(inline): done ok=%d code=%d msg=%s\n",
        status.ok ? 1 : 0,
        static_cast<int>(status.code),
        status.message.c_str());
    return status;
}

proto::Status MTPPlugin::ManualCopyViaHost(uint32_t src_handle,
                                            const proto::ObjectEntry& src_stat,
                                            uint32_t dst_storage_id,
                                            uint32_t dst_parent,
                                            const std::string& new_name) {
    proto::CancellationSource cancelSrc;
    auto cancel_tok = cancelSrc.Token();
    proto::Status final_status = proto::OkStatus();

    const RecursiveStats stats = src_stat.is_dir
        ? ComputeRecursiveStats(src_stat.storage_id, src_handle)
        : RecursiveStats{src_stat.size, 1};

    std::vector<WorkUnit> units;
    WorkUnit u;
    u.display_name = StrMB2Wide(new_name);
    u.is_directory = src_stat.is_dir;
    u.total_bytes = stats.size;
    u.total_files = std::max<uint64_t>(stats.count, 1);
    auto* cs = &cancelSrc;
    auto* fs = &final_status;
    u.execute = [this, src_handle, src_stat, dst_storage_id, dst_parent, new_name,
                 cancel_tok, cs, fs](ProgressTracker& tr) -> int {
        auto step = ManualCopyViaHostInline(src_handle, src_stat,
                                            dst_storage_id, dst_parent, new_name,
                                            tr, cancel_tok, cs);
        if (!step.ok) {
            *fs = step;
            return MapErrorToErrno(step);
        }
        return 0;
    };
    units.push_back(std::move(u));

    auto br = RunBatch(/*is_move=*/false, /*is_upload=*/false,
                       L"", L"", std::move(units),
                       [cs]() { cs->Cancel(); });
    if (br.aborted && final_status.ok) {
        final_status = proto::Status::Failure(proto::ErrorCode::Cancelled,
            "Cancelled by user");
    }
    return final_status;
}

WorkUnit MTPPlugin::MakeDeviceCopyMoveUnit(const DeviceCopyItem& it,
                                           const RecursiveStats& stats,
                                           const DeviceCopyCtx& ctx) {
    WorkUnit u;
    u.display_name = StrMB2Wide(it.stat.name);
    u.is_directory = it.stat.is_dir;
    u.total_bytes = stats.size;
    u.total_files = std::max<uint64_t>(stats.count, 1);
    u.execute = [this, it, stats, ctx](ProgressTracker& tr) -> int {
        if (it.skip) { tr.MarkSkipped(); return 0; }
        // dst_name = target_name (Rename intent) or stat.name (default).
        const std::string dst_name = it.target_name.empty() ? it.stat.name : it.target_name;
        const bool renaming = !it.target_name.empty() && it.target_name != it.stat.name;
        uint32_t aside_handle = 0;
        if (it.overwrite) {
            const std::string aside = AsideName(dst_name);
            auto rn = ctx.dst->_backend->Rename((*ctx.dst_existing)[dst_name].handle, aside);
            if (rn.ok) {
                aside_handle = (*ctx.dst_existing)[dst_name].handle;
            } else {
                DBG("aside-rename failed for '%s': %s — falling back to direct delete (no rollback)\n",
                    dst_name.c_str(), rn.message.c_str());
                auto del = ctx.dst->_backend->Delete((*ctx.dst_existing)[dst_name].handle, /*recursive=*/true);
                if (!del.ok) DBG("fallback delete also failed: %s\n", del.message.c_str());
            }
            ctx.dst_existing->erase(dst_name);
        }
        // For Rename (no overwrite): pre-rename src in its current parent so MoveObject lands as target_name in dst with no name collision (target_name is free, src.name may collide). For F5 Copy, post-rename works.
        if (renaming && !it.overwrite && ctx.move) {
            auto pre = _backend->Rename(it.handle, it.target_name);
            if (!pre.ok) {
                *ctx.hard_err = pre;
                tr.HaltBatch();
                return MapErrorToErrno(pre);
            }
        }
        // CopyObject/MoveObject opaque (no progressfunc) — pin file bar near 100% so modal stays alive during call.
        tr.PinNearDone(dst_name);
        proto::Status st;
        uint32_t copy_handle = 0;
        if (ctx.move) {
            st = _backend->MoveObject(it.handle, ctx.dst_storage, ctx.dst_parent);
        } else {
            auto cp = _backend->CopyObject(it.handle, ctx.dst_storage, ctx.dst_parent);
            st = cp.ok ? proto::OkStatus()
                       : proto::Status::Failure(cp.code, cp.message, cp.retryable);
            if (cp.ok) copy_handle = cp.value;
        }
        if (st.ok) {
            // F5 Rename: post-rename the new copy.
            if (renaming && !ctx.move && copy_handle != 0) {
                auto rn = _backend->Rename(copy_handle, it.target_name);
                if (!rn.ok) DBG("post-copy rename to '%s' failed: %s — copy retains libmtp-assigned name\n",
                                it.target_name.c_str(), rn.message.c_str());
            }
            if (aside_handle) {
                auto del = ctx.dst->_backend->Delete(aside_handle, /*recursive=*/true);
                if (!del.ok) DBG("aside cleanup after success failed: %s — stale '%s.far2l-tmp.%d' remains\n",
                                 del.message.c_str(), dst_name.c_str(), getpid());
            }
            return 0;
        }
        // F6 Rename rollback: undo the pre-rename if the move itself failed.
        if (renaming && !it.overwrite && ctx.move) {
            auto rev = _backend->Rename(it.handle, it.stat.name);
            if (!rev.ok) DBG("pre-rename rollback failed: %s — src is now '%s'\n",
                             rev.message.c_str(), it.target_name.c_str());
        }
        if (aside_handle) {
            auto rn = ctx.dst->_backend->Rename(aside_handle, dst_name);
            if (!rn.ok) DBG("CRITICAL aside-restore failed: %s — '%s' now named '%s.far2l-tmp.%d', manual recovery needed\n",
                            rn.message.c_str(), dst_name.c_str(), dst_name.c_str(), getpid());
        }
        if (st.code == proto::ErrorCode::Unsupported) {
            if (it.overwrite) {
                auto kids = ctx.dst->_backend->ListChildren(ctx.dst_storage, ctx.dst_parent);
                if (kids.ok) for (const auto& k2 : kids.value)
                    if (k2.name == dst_name) {
                        auto del = ctx.dst->_backend->Delete(k2.handle, /*recursive=*/true);
                        if (!del.ok) DBG("Unsupported-fallback duplicate-clear delete failed: %s\n", del.message.c_str());
                        break;
                    }
            }
            ctx.fallback->push_back({it.handle, it.stat, ctx.dst_storage,
                                     ctx.dst_parent, dst_name, stats.size,
                                     std::max<uint64_t>(stats.count, 1)});
            tr.MarkSkipped();
            return 0;
        }
        *ctx.hard_err = st;
        tr.HaltBatch();
        return MapErrorToErrno(st);
    };
    return u;
}

proto::Status MTPPlugin::RunHostFallbackBatch(const std::vector<HostFallbackItem>& items,
                                              bool move,
                                              size_t* out_ok_count) {
    if (out_ok_count) *out_ok_count = 0;
    if (items.empty()) return proto::OkStatus();

    proto::CancellationSource cancelSrc;
    auto cancel_tok = cancelSrc.Token();
    proto::Status final_status = proto::OkStatus();

    std::vector<WorkUnit> units;
    units.reserve(items.size());
    for (const auto& it : items) {
        // Use caller-supplied stats when present, else walk now.
        uint64_t bytes = it.total_bytes;
        uint64_t files = it.total_files;
        if (bytes == 0 && files == 0) {
            if (it.src_stat.is_dir) {
                auto s = ComputeRecursiveStats(it.src_stat.storage_id, it.src_handle);
                bytes = s.size;
                files = std::max<uint64_t>(s.count, 1);
            } else {
                bytes = it.src_stat.size;
                files = 1;
            }
        }
        WorkUnit u;
        u.display_name = StrMB2Wide(it.dst_name);
        u.is_directory = it.src_stat.is_dir;
        u.total_bytes = bytes;
        u.total_files = files;
        auto* cs = &cancelSrc;
        auto* fs = &final_status;
        u.execute = [this, it, move, cancel_tok, cs, fs](ProgressTracker& tr) -> int {
            auto step = ManualCopyViaHostInline(it.src_handle, it.src_stat,
                                                it.dst_storage_id, it.dst_parent,
                                                it.dst_name, tr, cancel_tok, cs);
            if (!step.ok) {
                *fs = step;
                return MapErrorToErrno(step);
            }
            if (move) {
                auto del = _backend->Delete(it.src_handle, /*recursive=*/true);
                if (!del.ok) {
                    DBG("RunHostFallbackBatch: delete-after-copy failed handle=%u msg=%s\n",
                        it.src_handle, del.message.c_str());
                }
            }
            return 0;
        };
        units.push_back(std::move(u));
    }

    auto* cs_outer = &cancelSrc;
    auto br = RunBatch(/*is_move=*/move, /*is_upload=*/false,
                       L"", L"", std::move(units),
                       [cs_outer]() { cs_outer->Cancel(); });
    if (br.aborted && final_status.ok) {
        final_status = proto::Status::Failure(proto::ErrorCode::Cancelled,
            "Cancelled by user");
    }
    if (out_ok_count) *out_ok_count = static_cast<size_t>(br.success_count);
    return final_status;
}
