#include "ADBPlugin.h"
#include "ADBDevice.h"
#include "ADBShell.h"
#include "ADBDialogs.h"
#include "ADBLog.h"
#include "ProgressBatch.h"
#include "lng.h"

namespace {
// Bounded title copy — _PanelTitle is a fixed wchar_t[N]; never write past it regardless of translation length.
template<size_t N>
void SetPanelTitle(wchar_t (&dst)[N], const wchar_t* src) {
    wcsncpy(dst, src, N - 1);
    dst[N - 1] = 0;
}
} // namespace
#include <sstream>
#include <string>
#include <vector>
#include <map>
#include <unordered_map>
#include <atomic>
#include <cstring>
#include <cstdarg>
#include <unistd.h>
#include <sys/stat.h>
#include <utils.h>
#include <sys/types.h>
#include <dirent.h>
#include <errno.h>
#include <utils.h>
#include <farplug-wide.h>

// CheckOverwrite return codes. RENAME/NEWER returned as-is — caller must handle.
enum CollisionAction { CA_PROCEED = 0, CA_SKIP = 1, CA_ABORT = 2, CA_RENAME = 3, CA_NEWER = 4 };

// overwriteMode: 0=ask, 1=overwrite_all, 2=skip_all (persisted across calls). Optional sizes/mtimes/view callbacks drive the New/Existing info rows.
static int CheckOverwrite(const std::wstring& destPath, bool isMultiple, bool isDir,
                          int& overwriteMode, ProgressState& state,
                          uint64_t src_size = 0, int64_t src_mtime = 0,
                          uint64_t dst_size = 0, int64_t dst_mtime = 0,
                          OverwriteDialog::ViewFn view_new = nullptr,
                          OverwriteDialog::ViewFn view_existing = nullptr) {
	if (overwriteMode == 1) return CA_PROCEED;  // Overwrite all
	if (overwriteMode == 2) return CA_SKIP;     // Skip all

	OverwriteDialog dlg(destPath, isMultiple, isDir,
	                    src_size, src_mtime, dst_size, dst_mtime,
	                    std::move(view_new), std::move(view_existing));
	switch (dlg.Ask()) {
		case OverwriteDialog::OVERWRITE:     return CA_PROCEED;
		case OverwriteDialog::SKIP:          return CA_SKIP;
		case OverwriteDialog::OVERWRITE_ALL: overwriteMode = 1; return CA_PROCEED;
		case OverwriteDialog::SKIP_ALL:      overwriteMode = 2; return CA_SKIP;
		case OverwriteDialog::RENAME:        return CA_RENAME;
		case OverwriteDialog::ONLY_NEWER:    return CA_NEWER;
		case OverwriteDialog::CANCEL:
		default:
			state.SetAborting();
			return CA_ABORT;
	}
}

// Host-side helpers — mirror ADB-side `FindFreeAdbName` + `GetAdbMtime`.
static std::string FindFreeLocalName(const std::string& abs_path) {
	auto slash = abs_path.find_last_of('/');
	std::string parent = (slash == std::string::npos) ? std::string() : abs_path.substr(0, slash);
	std::string base   = (slash == std::string::npos) ? abs_path : abs_path.substr(slash + 1);
	std::string stem = base, ext;
	auto dot = base.find_last_of('.');
	if (dot != std::string::npos && dot > 0) { stem = base.substr(0, dot); ext = base.substr(dot); }
	for (int n = 2; n < 1000; ++n) {
		std::string cand = parent + (parent.empty() ? "" : "/") + stem + "(" + std::to_string(n) + ")" + ext;
		struct stat st{};
		if (stat(cand.c_str(), &st) != 0) return cand;
	}
	return std::string();
}

static time_t GetLocalMtime(const std::string& abs_path) {
	struct stat st{};
	if (stat(abs_path.c_str(), &st) != 0) return 0;
	return st.st_mtime;
}

static bool RemoveLocalPathRecursively(const std::string& path) {
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

// --- destination parser tunables ---
static constexpr std::string_view kAdbPrefix      = "adb:";
static constexpr std::string_view kAdbPrefixSlash = "adb:/";

// --- collision/overwrite tunables ---
static constexpr int kFindFreeNameMaxTries = 32;

// --- diagnostics tunables ---
static constexpr time_t kClockSkewWarnSec         = 86400;  // 1 day
static constexpr time_t kClockSkewWarnThrottleSec = 60;

// FAR API tunable: FCTL_GET*PANELITEM size query covers struct only on some far2l builds; pad covers trailing strings.
static constexpr size_t kPanelItemAllocPad = 0x100;

static void RefreshBothPanels() {
	g_Info.Control(PANEL_ACTIVE,  FCTL_UPDATEPANEL, 0, 0);
	g_Info.Control(PANEL_ACTIVE,  FCTL_REDRAWPANEL, 0, 0);
	g_Info.Control(PANEL_PASSIVE, FCTL_UPDATEPANEL, 0, 0);
	g_Info.Control(PANEL_PASSIVE, FCTL_REDRAWPANEL, 0, 0);
}

static void RefreshBothPanelsClearingSelection() {
	g_Info.Control(PANEL_ACTIVE, FCTL_CLEARSELECTION, 0, 0);
	RefreshBothPanels();
}

static bool NoControls(unsigned int control_state) {
	return (control_state & (PKF_CONTROL | PKF_ALT | PKF_SHIFT)) == 0;
}

using ADBUtils::PathBasename;

// Lexical POSIX path normalize: collapse "..", drop ".", merge "//".
static std::string NormalizePosixPath(const std::string& p) {
	if (p.empty()) return p;
	std::vector<std::string> parts;
	const bool absolute = (p[0] == '/');
	std::string seg;
	auto flush = [&](std::string& s) {
		if (s.empty()) return;
		if (s == "..") {
			if (!parts.empty() && parts.back() != "..") parts.pop_back();
			else if (!absolute) parts.push_back("..");
		} else if (s != ".") {
			parts.push_back(s);
		}
		s.clear();
	};
	for (char c : p) {
		if (c == '/') flush(seg);
		else seg += c;
	}
	flush(seg);
	std::string out = absolute ? "/" : "";
	for (size_t i = 0; i < parts.size(); ++i) {
		if (i > 0) out += "/";
		out += parts[i];
	}
	return out.empty() ? (absolute ? std::string("/") : std::string(".")) : out;
}

// Aside-rename suffix for atomic-overwrite. PID-scoped to avoid clashes.
static std::string AsideName(const std::string& base) {
	return base + ".far2l-tmp." + std::to_string(getpid());
}

// Routes input: "adb:/abs"→ADB-abs, "/abs"→LOCAL, "rel"→ADB-rel, ""→ADB-cwd; strips trailing ':' (far2l plugin-format suffix), keeps bare "adb:".
enum class DestKind { ADB, LOCAL };
struct ParsedDest {
	DestKind kind;
	std::string abs_path;        // device-absolute or host-absolute
	std::string cleaned_input;   // input with trailing ':' and "adb:/" stripped — for "."/".." sentinels
	bool trailing_slash = false; // explicit "/" → folder-into intent
	bool empty_input = false;    // user gave nothing
};

static ParsedDest ParseDestInput(const std::string& input, const std::string& src_adb_dir) {
	ParsedDest r{};
	if (input.empty()) {
		r.kind = DestKind::ADB;
		r.abs_path = src_adb_dir;
		r.empty_input = true;
		return r;
	}
	std::string in = input;
	while (!in.empty() && in.back() == ':') in.pop_back();
	if (in.empty()) {
		r.kind = DestKind::ADB;
		r.abs_path = src_adb_dir;
		r.empty_input = true;
		return r;
	}
	r.trailing_slash = (in.back() == '/');
	if (in.compare(0, kAdbPrefixSlash.size(), kAdbPrefixSlash) == 0) {
		r.kind = DestKind::ADB;
		std::string p = in.substr(kAdbPrefix.size());
		if (p.size() > 1 && p.back() == '/') p.pop_back();
		r.cleaned_input = p;
		r.abs_path = NormalizePosixPath(p);
		return r;
	}
	if (in.front() == '/') {
		r.kind = DestKind::LOCAL;
		std::string p = in;
		if (p.size() > 1 && p.back() == '/') p.pop_back();
		r.cleaned_input = p;
		r.abs_path = p;
		return r;
	}
	r.kind = DestKind::ADB;
	std::string trimmed = in;
	if (trimmed.size() > 1 && trimmed.back() == '/') trimmed.pop_back();
	r.cleaned_input = trimmed;
	std::string joined = src_adb_dir;
	if (joined.empty() || joined.back() != '/') joined += '/';
	joined += trimmed;
	r.abs_path = NormalizePosixPath(joined);
	return r;
}

// mkdir -p on device; stderr→stdout for errno mapping.
static int MkdirPAdb(ADBDevice& dev, const std::string& path) {
	if (path.empty() || path == "/") return 0;
	std::string out = dev.RunShellCommand(
		"mkdir -p -- " + ADBUtils::ShellQuote(path) + " 2>&1");
	if (dev.LastShellExitCode() == 0) return 0;
	int errno_mapped = ADBDevice::Str2Errno(out);
	return errno_mapped ? errno_mapped : EIO;
}

// mkdir -p the parent dir of an absolute file path on ADB.
static int EnsureAdbParentDirs(ADBDevice& dev, const std::string& abs_path) {
	auto slash = abs_path.find_last_of('/');
	if (slash == std::string::npos || slash == 0) return 0;
	return MkdirPAdb(dev, abs_path.substr(0, slash));
}

// Auto-increment "name(N)" for collision. Cap=32 — each probe is a shell roundtrip.
static std::string FindFreeAdbName(ADBDevice& dev, const std::string& abs_path) {
	auto slash = abs_path.find_last_of('/');
	std::string parent = (slash == std::string::npos) ? std::string() : abs_path.substr(0, slash);
	std::string base   = (slash == std::string::npos) ? abs_path : abs_path.substr(slash + 1);
	std::string stem = base, ext;
	auto dot = base.find_last_of('.');
	if (dot != std::string::npos && dot > 0) { stem = base.substr(0, dot); ext = base.substr(dot); }
	for (int n = 2; n < kFindFreeNameMaxTries; ++n) {
		std::string cand = parent + (parent.empty() ? "" : "/") + stem + "(" + std::to_string(n) + ")" + ext;
		if (!dev.FileExists(cand)) return cand;
	}
	return std::string();
}

// Device file size via `stat -c %s`; 0 on missing/error.
static uint64_t GetAdbSize(ADBDevice& dev, const std::string& abs_path) {
	std::string cmd = "stat -c %s -- " + ADBUtils::ShellQuote(abs_path) + " 2>/dev/null";
	std::string out = dev.RunShellCommand(cmd);
	ADBUtils::TrimTrailingNewlines(out);
	if (out.empty()) return 0;
	try { return (uint64_t)std::stoull(out); } catch (...) { return 0; }
}

// Device mtime via `stat -c %Y`; logs clock-skew warning (throttled) when device clock is wildly off.
static time_t GetAdbMtime(ADBDevice& dev, const std::string& abs_path) {
	std::string cmd = "stat -c %Y -- " + ADBUtils::ShellQuote(abs_path) + " 2>/dev/null";
	std::string out = dev.RunShellCommand(cmd);
	ADBUtils::TrimTrailingNewlines(out);
	if (out.empty()) return 0;
	time_t mt = 0;
	try { mt = (time_t)std::stoll(out); } catch (...) { return 0; }
	if (mt > 0) {
		static time_t last_warn = 0;
		const time_t now = time(nullptr);
		const time_t skew = (mt > now) ? (mt - now) : (now - mt);
		if (skew > kClockSkewWarnSec && now - last_warn > kClockSkewWarnThrottleSec) {
			last_warn = now;
			DBG("OnlyNewer warning: device mtime '%s'=%lld vs host now=%lld skew=%llds — check device clock\n",
				abs_path.c_str(), (long long)mt, (long long)now, (long long)skew);
		}
	}
	return mt;
}

// Overwrite-dialog viewer: adb=null → open host file directly; adb!=null → pull to per-pid temp (VF_DELETEONCLOSE wipes on viewer exit).
static OverwriteDialog::ViewFn MakeOverwriteViewer(std::shared_ptr<ADBDevice> adb,
                                                   const std::string& path,
                                                   bool is_dir) {
	if (is_dir) return nullptr;
	if (!adb) {
		return [path]() {
			std::wstring w = StrMB2Wide(path);
			g_Info.Viewer(w.c_str(), w.c_str(), -1, -1, -1, -1, VF_NONMODAL, CP_UTF8);
		};
	}
	return [adb, path]() {
		static std::atomic<uint64_t> seq{0};
		std::string base = path;
		auto slash = base.find_last_of('/');
		if (slash != std::string::npos) base = base.substr(slash + 1);
		for (char& c : base) if (c == '/' || c == '\\') c = '_';
		char prefix[64];
		snprintf(prefix, sizeof(prefix), "adb_overw_%d_%llu_",
		         static_cast<int>(getpid()),
		         static_cast<unsigned long long>(seq.fetch_add(1)));
		std::string tmp = InMyTemp((std::string(prefix) + base).c_str());
		if (adb->PullFile(path, tmp) != 0) return;
		::chmod(tmp.c_str(), 0644);
		std::wstring w = StrMB2Wide(tmp);
		g_Info.Viewer(w.c_str(), w.c_str(), -1, -1, -1, -1,
		              VF_NONMODAL | VF_DELETEONCLOSE, CP_UTF8);
	};
}

// Fills size + mtime for a file. Skips dirs (no recursive walk yet). adb=null → host stat.
static void FillFileMeta(std::shared_ptr<ADBDevice> adb, const std::string& path, bool is_dir,
                         uint64_t& size, int64_t& mtime) {
	size = 0; mtime = 0;
	if (is_dir) return;
	if (!adb) {
		struct stat st{};
		if (::stat(path.c_str(), &st) == 0) {
			size = static_cast<uint64_t>(st.st_size);
			mtime = static_cast<int64_t>(st.st_mtime);
		}
	} else {
		size = GetAdbSize(*adb, path);
		mtime = static_cast<int64_t>(GetAdbMtime(*adb, path));
	}
}

// mkdir -p on host FS.
static int MkdirPLocal(const std::string& path) {
	if (path.empty() || path == "/") return 0;
	for (size_t i = 1; i <= path.size(); ++i) {
		if (i == path.size() || path[i] == '/') {
			std::string cur = path.substr(0, i);
			struct stat st{};
			if (stat(cur.c_str(), &st) == 0) {
				if (!S_ISDIR(st.st_mode)) return ENOTDIR;
			} else if (mkdir(cur.c_str(), 0755) != 0 && errno != EEXIST) {
				return errno;
			}
		}
	}
	return 0;
}

// ADB resolver — folder-only inputs (trailing slash, ".", "..", or existing-dir) append src_name.
static std::string ResolveAdbDestinationFor(const std::string& currentDir,
                                            const std::string& userInput,
                                            const std::string& src_name,
                                            std::shared_ptr<ADBDevice> dev) {
	if (userInput.empty()) return userInput;
	const ParsedDest pd = ParseDestInput(userInput, currentDir);
	std::string resolved = pd.abs_path;
	const bool dot_or_dotdot = (pd.cleaned_input == "." || pd.cleaned_input == ".."
	                          || pd.cleaned_input == "./" || pd.cleaned_input == "../");
	if (pd.trailing_slash || dot_or_dotdot) {
		if (!src_name.empty()) {
			if (resolved.empty() || resolved.back() != '/') resolved += '/';
			resolved += src_name;
		}
		return resolved;
	}
	if (!src_name.empty() && dev && dev->IsDirectory(resolved)) {
		if (resolved.back() != '/') resolved += '/';
		resolved += src_name;
	}
	return resolved;
}

// Storage-relative path prefix for prompt prefills: "<absolute-dir>/".
static std::string BuildPanelPathPrefix(const std::string& currentDir) {
	std::string out = currentDir.empty() ? std::string("/") : currentDir;
	if (out.back() != '/') out += '/';
	return out;
}


PluginStartupInfo g_Info = {};
FarStandardFunctions g_FSF = {};


ADBPlugin::ADBPlugin()
	: _isConnected(false)
	, _CurrentDir("/")
{
	SetPanelTitle(_PanelTitle, L"ADB");
	wcscpy(_mk_dir, L"");

	// Enumerate once to avoid races between count/first-serial calls.
	auto devices = EnumerateDevices();
	int deviceCount = (int)devices.size();

	if (deviceCount == 1) {
		const std::string& deviceSerial = devices[0].serial;
		if (!deviceSerial.empty() && ConnectToDevice(deviceSerial)) {
			_isConnected = true;
			_deviceSerial = deviceSerial;
			UpdatePanelTitle(_deviceSerial, GetCurrentDevicePath());
		}
	} else if (deviceCount > 1) {
		SetPanelTitle(_PanelTitle, Lng(MADBSelectDevice));
	}
}

ADBPlugin::~ADBPlugin()
{
	// Explicit disconnect — tear down adb shell child before shared_ptr's own dtor (predictable order).
	if (_adbDevice) {
		try {
			_adbDevice->Disconnect();
		} catch (...) {
			// Destructor must not throw; a device whose session is already dead is fine.
		}
		_adbDevice.reset();
	}
}

int ADBPlugin::GetFindData(PluginPanelItem **pPanelItem, int *pItemsNumber, int OpMode)
{
	if (_isConnected && _adbDevice) 
		return GetFileData(pPanelItem, pItemsNumber);
	
	return GetDeviceData(pPanelItem, pItemsNumber);
}

void ADBPlugin::FreeFindData(PluginPanelItem *PanelItem, int ItemsNumber)
{
	if (!PanelItem || ItemsNumber <= 0) return;
	
	for (int i = 0; i < ItemsNumber; i++) {
		free((void*)PanelItem[i].FindData.lpwszFileName);
		free((void*)PanelItem[i].Description);
		free((void*)PanelItem[i].Owner);
		free((void*)PanelItem[i].Group);

		if (PanelItem[i].CustomColumnData) {
			for (int j = 0; j < PanelItem[i].CustomColumnNumber; j++) {
				free((void*)PanelItem[i].CustomColumnData[j]);
			}
			delete[] PanelItem[i].CustomColumnData;
		}
	}
	delete[] PanelItem;
}

void ADBPlugin::GetOpenPluginInfo(OpenPluginInfo *Info)
{
	if (!Info) return;

	Info->StructSize       = sizeof(*Info);
	Info->HostFile         = nullptr;
	Info->InfoLines        = nullptr;
	Info->InfoLinesNumber  = 0;
	Info->DescrFiles       = nullptr;
	Info->DescrFilesNumber = 0;
	Info->KeyBar           = nullptr;
	Info->ShortcutData     = nullptr;

	// Panel mode storage
	static PanelMode connectedMode = {
		.ColumnTypes        = L"N,C0",
		.ColumnWidths       = L"0,0",
		.ColumnTitles       = nullptr, // set later
		.FullScreen         = 0,
		.DetailedStatus     = 1,
		.AlignExtensions    = 0,
		.CaseConversion     = 0,
		.StatusColumnTypes  = L"N,C0",
		.StatusColumnWidths = L"0,0",
		.Reserved           = {0, 0}
	};

	const wchar_t* connectedTitles[] = { Lng(MColName), Lng(MColSize) };

	static PanelMode deviceMode = {
		.ColumnTypes        = L"N,C0,C1,C2",
		.ColumnWidths       = L"0,30,0,8",
		.ColumnTitles       = nullptr, // set later
		.FullScreen         = 0,
		.DetailedStatus     = 1,
		.AlignExtensions    = 0,
		.CaseConversion     = 0,
		.StatusColumnTypes  = L"N,C0,C1,C2",
		.StatusColumnWidths = L"0,30,0,8",
		.Reserved           = {0, 0}
	};

	const wchar_t* deviceTitles[] = { Lng(MColSerial), Lng(MColDeviceName), Lng(MColModel), Lng(MColPort) };

	if (_isConnected) {
		connectedMode.ColumnTitles = connectedTitles;
		Info->PanelModesArray      = &connectedMode;
		Info->Flags                = OPIF_SHOWPRESERVECASE | OPIF_USEHIGHLIGHTING;
		// Leave StartSortMode = 0; flplugin.cpp:904 re-applies it on every SetPluginMode and would wipe the user's sort choice.

		// Serial as last path component so FAR's PointToName-based cursor restore finds it on "..".
		std::string display_path = "/" + _deviceSerial;
		if (!_CurrentDir.empty() && _CurrentDir != "/") {
			display_path += _CurrentDir;
		}
		_curDirW  = StrMB2Wide(display_path);
		_formatW  = L"adb:" + _curDirW;

		Info->CurDir  = _curDirW.c_str();
		Info->Format  = _formatW.c_str();
	} else {
		deviceMode.ColumnTitles = deviceTitles;
		Info->PanelModesArray   = &deviceMode;
		Info->Flags             = OPIF_SHOWPRESERVECASE | OPIF_USEHIGHLIGHTING | OPIF_SHOWNAMESONLY;

		Info->CurDir = L"";
		Info->Format = L"ADB";
	}

	Info->StartPanelMode = 0;
	Info->PanelModesNumber  = 1;
	Info->PanelTitle     = _PanelTitle;
}


int ADBPlugin::ProcessKey(int Key, unsigned int ControlState)
{
	// Handle ENTER in device selection mode (when not connected)
	if (!_isConnected && Key == VK_RETURN && ControlState == 0) {
		return ByKey_TryEnterSelectedDevice() ? TRUE : FALSE;
	}
	// F5/F6 full intercept — far2l ShellCopy never fires, single dialog via unified parser.
	if ((Key == VK_F5 || Key == VK_F6) && NoControls(ControlState)) {
		HANDLE active = INVALID_HANDLE_VALUE;
		g_Info.Control(PANEL_ACTIVE, FCTL_GETPANELPLUGINHANDLE, 0, (LONG_PTR)(void*)&active);
		if (active == (void*)this) {
			(void)CrossPanelCopyMoveSameDevice(Key == VK_F6);
			return TRUE;  // claim unconditionally — never let far2l show ShellCopy
		}
	}
	if (Key == VK_F5 && ControlState == PKF_SHIFT) {
		return ShiftF5CopyInPlace() ? TRUE : FALSE;
	}
	if (Key == VK_F6 && ControlState == PKF_SHIFT) {
		return ShiftF6Rename() ? TRUE : FALSE;
	}
	return FALSE;
}

bool ADBPlugin::CrossPanelCopyMoveSameDevice(bool move)
{
	if (!_isConnected || !_adbDevice || _deviceSerial.empty()) {
		return false;
	}

	HANDLE active = INVALID_HANDLE_VALUE;
	g_Info.Control(PANEL_ACTIVE, FCTL_GETPANELPLUGINHANDLE, 0, (LONG_PTR)(void*)&active);
	if (active != (void*)this) {
		return false;
	}

	// Same-device adb (incl. dst==this) → in-device fast path. Else dstDir = passive CurDir.
	HANDLE passive = INVALID_HANDLE_VALUE;
	g_Info.Control(PANEL_PASSIVE, FCTL_GETPANELPLUGINHANDLE, 0, (LONG_PTR)(void*)&passive);
	ADBPlugin* dst = nullptr;
	if (passive != INVALID_HANDLE_VALUE && passive != nullptr) {
		uint64_t sig = 0;
		memcpy(&sig, passive, sizeof(sig));
		if (sig == PLUGIN_SIGNATURE) {
			auto* d = (ADBPlugin*)passive;
			if (d->_isConnected && d->_adbDevice && d->_deviceSerial == _deviceSerial) {
				dst = d;
			}
		}
	}

	const std::string srcDir = GetCurrentDevicePath();
	if (srcDir.empty()) return false;

	// dstDir = adb path if same-device adb, else passive's raw CurDir (likely local fs).
	std::string dstDir;
	bool dst_is_adb = (dst != nullptr);
	if (dst_is_adb) {
		dstDir = dst->GetCurrentDevicePath();
		if (dstDir.empty()) dstDir = srcDir;
	} else {
		intptr_t sz = g_Info.Control(PANEL_PASSIVE, FCTL_GETPANELDIR, 0, 0);
		if (sz > 0) {
			std::vector<wchar_t> buf(sz);
			if (g_Info.Control(PANEL_PASSIVE, FCTL_GETPANELDIR, sz, (LONG_PTR)buf.data())) {
				dstDir = StrWide2MB(buf.data());
			}
		}
	}
	// Used by in-device fast path (resolveDst) when no dst plugin is available.
	if (!dst) dst = this;

	PanelInfo pi = {};
	if (g_Info.Control(PANEL_ACTIVE, FCTL_GETPANELINFO, 0, (LONG_PTR)(void*)&pi) == 0) {
		return false;
	}

	auto getItem = [&](int cmd, int index, PluginPanelItem& out, std::vector<char>& buf) -> bool {
		intptr_t size = g_Info.Control(PANEL_ACTIVE, cmd, index, 0);
		if (size < (intptr_t)sizeof(PluginPanelItem)) {
			return false;
		}
		buf.assign((size_t)size + kPanelItemAllocPad, 0);
		auto* item = (PluginPanelItem*)buf.data();
		if (!g_Info.Control(PANEL_ACTIVE, cmd, index, (LONG_PTR)(void*)item)) {
			return false;
		}
		out = *item;
		return true;
	};

	struct SelectedItem {
		std::string name;
		bool is_dir = false;
		uint64_t size = 0;
	};
	std::vector<SelectedItem> items;
	auto pushItem = [&](const PluginPanelItem& it) {
		if (!it.FindData.lpwszFileName) return;
		std::string name = StrWide2MB(it.FindData.lpwszFileName);
		if (name.empty() || name == "." || name == "..") return;
		const bool is_dir = (it.FindData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) != 0;
		items.push_back(SelectedItem{name, is_dir, is_dir ? 0 : it.FindData.nFileSize});
	};
	if (pi.SelectedItemsNumber > 0) {
		for (int i = 0; i < pi.SelectedItemsNumber; ++i) {
			PluginPanelItem it = {};
			std::vector<char> buf;
			if (!getItem(FCTL_GETSELECTEDPANELITEM, i, it, buf)) continue;
			pushItem(it);
		}
	} else {
		PluginPanelItem it = {};
		std::vector<char> buf;
		if (getItem(FCTL_GETCURRENTPANELITEM, 0, it, buf)) pushItem(it);
	}

	if (items.empty()) {
		return false;
	}

	// Single-item rename intent: full path, no trailing slash, dst not a dir → *As variants.
	bool rename_mode = false;
	std::string rename_dst;

	// Confirmation prompt; user can edit dst path ("..", "./sub/", abs).
	{
		const wchar_t* title = move ? Lng(MMove) : Lng(MCopy);
		std::wstring prompt;
		const std::wstring verb = std::wstring(move ? Lng(MMove) : Lng(MCopy)) + L" ";
		if (items.size() == 1) {
			prompt = verb + StrMB2Wide(items[0].name) + L" " + Lng(MToColon);
		} else {
			prompt = verb + std::to_wstring(items.size()) + Lng(MItemsSuffix) + L" " + Lng(MToColon);
		}
		// Prefill: adb→"adb:/dst/", local→"/dst/". Parser dispatches: adb:/abs|rel→in-device, /abs→pull.
		std::string prefill = dst_is_adb
			? std::string(kAdbPrefix) + BuildPanelPathPrefix(dstDir)
			: BuildPanelPathPrefix(dstDir);
		std::string user_path;
		if (!ADBDialogs::AskInput(title, prompt.c_str(), L"ADB_CopyMove",
		                           user_path, prefill)) {
			return true;  // user cancelled
		}
		if (user_path.empty()) return true;
		const ParsedDest pd = ParseDestInput(user_path, srcDir);
		if (pd.kind == DestKind::LOCAL) {
			// Host pull (download). Multi-item or trailing-slash → into dir.
			const bool into_dir = (items.size() > 1) || pd.trailing_slash;
			if (into_dir) (void)MkdirPLocal(pd.abs_path);
			else {
				auto slash = pd.abs_path.find_last_of('/');
				if (slash != std::string::npos && slash > 0) (void)MkdirPLocal(pd.abs_path.substr(0, slash));
			}
			// Per-item collision check before pull (`adb pull` would otherwise overwrite silently).
			int overwriteMode = 0;
			std::vector<int> per_action(items.size(), CA_PROCEED);
			std::vector<std::string> renamed(items.size());
			auto adb = _adbDevice;
			for (size_t i = 0; i < items.size(); ++i) {
				const std::string localDst = into_dir
					? (pd.abs_path + "/" + items[i].name)
					: pd.abs_path;
				struct stat st{};
				if (stat(localDst.c_str(), &st) != 0) continue;
				const std::string srcPath = ADBUtils::JoinPath(srcDir, items[i].name);
				const bool isDir = items[i].is_dir;
				uint64_t src_sz = 0, dst_sz = 0;
				int64_t src_mt = 0, dst_mt = 0;
				FillFileMeta(adb,     srcPath,  isDir, src_sz, src_mt);
				FillFileMeta(nullptr, localDst, isDir, dst_sz, dst_mt);
				ProgressState dummy;
				int act = CheckOverwrite(StrMB2Wide(localDst), items.size() > 1, isDir,
				                         overwriteMode, dummy,
				                         src_sz, src_mt, dst_sz, dst_mt,
				                         MakeOverwriteViewer(adb, srcPath, isDir),
				                         MakeOverwriteViewer(nullptr, localDst, isDir));
				if (act == CA_ABORT) return true;
				per_action[i] = act;
				if (act == CA_RENAME) {
					std::string fresh = FindFreeLocalName(localDst);
					if (fresh.empty()) per_action[i] = CA_SKIP;
					else renamed[i] = fresh;
				}
			}
			std::vector<std::string> dirsToScan;
			for (size_t i = 0; i < items.size(); ++i)
				if (per_action[i] != CA_SKIP && items[i].is_dir)
					dirsToScan.push_back(ADBUtils::JoinPath(srcDir, items[i].name));
			auto dirInfo = PrescanDeviceDirs(dirsToScan);
			std::vector<WorkUnit> units;
			for (size_t i = 0; i < items.size(); ++i) {
				if (per_action[i] == CA_SKIP) continue;
				const auto& it = items[i];
				const std::string srcPath = ADBUtils::JoinPath(srcDir, it.name);
				const std::string localDst = !renamed[i].empty()
					? renamed[i]
					: (into_dir ? (pd.abs_path + "/" + it.name) : pd.abs_path);
				const bool isDir = it.is_dir;
				const bool move_cap = move;
				const bool only_newer = per_action[i] == CA_NEWER;
				auto t = ComputeUnitTotals(srcPath, isDir, it.size, dirInfo);
				WorkUnit u;
				u.display_name = StrMB2Wide(it.name);
				u.is_directory = isDir;
				u.total_bytes = t.total_bytes;
				u.total_files = t.total_files;
				u.execute = [adb, srcPath, localDst, isDir, move_cap, only_newer, idx = std::move(t.idx)](ProgressTracker& tr) -> int {
					if (only_newer) {
						time_t s = GetAdbMtime(*adb, srcPath);
						time_t d = GetLocalMtime(localDst);
						if (s > 0 && d > 0 && s <= d) { tr.MarkSkipped(); return 0; }
					}
					auto onAbort = [&tr]{ return tr.Aborted(); };
					auto cb = [&tr, &idx, isDir](int p, const std::string& path) {
						tr.Tick(p, path, isDir ? LookupSubitemSize(idx, path) : 0);
					};
					int rc = isDir ? adb->PullDirectory(srcPath, localDst, cb, onAbort)
					               : adb->PullFile(srcPath, localDst, cb, onAbort);
					if (rc != 0) return rc;
					if (move_cap) {
						return isDir ? adb->DeleteDirectory(srcPath) : adb->DeleteFile(srcPath);
					}
					return 0;
				};
				units.push_back(std::move(u));
			}
			auto br = RunBatch(move ? Lng(MMoveToHost) : Lng(MCopyToHost),
			                   StrMB2Wide(srcDir), StrMB2Wide(pd.abs_path),
			                   std::move(units));
			if (br.success_count == 0 && br.last_error != 0) WINPORT(SetLastError)(br.last_error);
			RefreshBothPanels();
			return true;
		}
		const bool is_dot_or_dotdot = (pd.cleaned_input == "." || pd.cleaned_input == "..");
		// Single-item rename intent: leaf path (no trailing slash) + name isn't "."/"..".
		if (items.size() == 1 && !pd.trailing_slash && !is_dot_or_dotdot
		    && !dst->_adbDevice->IsDirectory(pd.abs_path)) {
			rename_mode = true;
			rename_dst = pd.abs_path;
			(void)EnsureAdbParentDirs(*dst->_adbDevice, rename_dst);
		} else {
			dstDir = pd.abs_path;
			if (!dst->_adbDevice->IsDirectory(dstDir)) {
				(void)MkdirPAdb(*dst->_adbDevice, dstDir);
			}
			if (dstDir.empty() || dstDir.back() != '/') dstDir += '/';
		}
	}

	DBG("CrossPanelCopyMoveSameDevice op=%s count=%zu src=%s dst=%s serial=%s\n",
		move ? "move" : "copy", items.size(), srcDir.c_str(), dstDir.c_str(), _deviceSerial.c_str());

	bool overwrite_all = false, skip_all = false;
	std::vector<bool> skip(items.size(), false);
	std::vector<bool> overwrite(items.size(), false);
	std::vector<bool> only_newer(items.size(), false);
	std::vector<std::string> rename_override(items.size());
	auto resolveDst = [&](size_t i, const SelectedItem& it) -> std::string {
		if (!rename_override[i].empty()) return rename_override[i];
		return rename_mode ? rename_dst : ADBUtils::JoinPath(dstDir, it.name);
	};
	// Single `ls` instead of N+1 FileExists per item.
	std::unordered_set<std::string> dst_listing;
	if (!rename_mode) dst->_adbDevice->ListDirNames(dstDir, dst_listing);
	for (size_t i = 0; i < items.size(); ++i) {
		const auto& it = items[i];
		const std::string dstPath = rename_mode ? rename_dst : ADBUtils::JoinPath(dstDir, it.name);
		const bool exists = rename_mode ? dst->_adbDevice->FileExists(dstPath)
		                                : dst_listing.count(it.name) > 0;
		if (!exists) continue;
		if (skip_all)      { skip[i] = true; continue; }
		if (overwrite_all) { overwrite[i] = true; continue; }
		const std::string srcPath = ADBUtils::JoinPath(srcDir, it.name);
		uint64_t src_sz = 0, dst_sz = 0;
		int64_t src_mt = 0, dst_mt = 0;
		FillFileMeta(_adbDevice,      srcPath, it.is_dir, src_sz, src_mt);
		FillFileMeta(dst->_adbDevice, dstPath, it.is_dir, dst_sz, dst_mt);
		OverwriteDialog dlg(StrMB2Wide(dstPath), items.size() > 1, it.is_dir,
		                    src_sz, src_mt, dst_sz, dst_mt,
		                    MakeOverwriteViewer(_adbDevice,      srcPath, it.is_dir),
		                    MakeOverwriteViewer(dst->_adbDevice, dstPath, it.is_dir));
		switch (dlg.Ask()) {
			case OverwriteDialog::OVERWRITE:     overwrite[i] = true; break;
			case OverwriteDialog::SKIP:          skip[i] = true; break;
			case OverwriteDialog::OVERWRITE_ALL: overwrite_all = true; overwrite[i] = true; break;
			case OverwriteDialog::SKIP_ALL:      skip_all = true; skip[i] = true; break;
			case OverwriteDialog::RENAME: {
				std::string fresh = FindFreeAdbName(*dst->_adbDevice, dstPath);
				if (fresh.empty()) { skip[i] = true; }
				else { rename_override[i] = fresh; }
				break;
			}
			case OverwriteDialog::ONLY_NEWER:    only_newer[i] = true; break;
			case OverwriteDialog::CANCEL:
			default:
				return true;
		}
	}

	int okCount = 0;
	int lastErr = 0;
	std::vector<size_t> needFallback;
	{
		// RunBatch wrap — file_complete=99 keeps modal alive while opaque shell cp/mv runs.
		auto* fallback_ptr = &needFallback;
		auto* lastErr_ptr  = &lastErr;
		auto srcAdb = _adbDevice;
		auto dstAdb = dst->_adbDevice;
		const std::string dstDirCap = dstDir;
		const bool rename_mode_cap = rename_mode;
		const bool move_cap = move;

		// Pre-scan all dirs once — feeds accurate "X of Y"; shell cp/mv emits no per-file progress so we pin file_complete=99.
		std::vector<std::string> dirsToScan;
		for (size_t i = 0; i < items.size(); ++i)
			if (!skip[i] && items[i].is_dir)
				dirsToScan.push_back(ADBUtils::JoinPath(srcDir, items[i].name));
		auto dirInfo = PrescanDeviceDirs(dirsToScan);

		std::vector<WorkUnit> units;
		units.reserve(items.size());
		for (size_t i = 0; i < items.size(); ++i) {
			if (skip[i]) continue;
			const auto& it = items[i];
			const std::string srcPath = ADBUtils::JoinPath(srcDir, it.name);
			const std::string dstPath = resolveDst(i, it);
			const bool ow = overwrite[i];
			const bool isDir = it.is_dir;
			const bool only_newer_cap = only_newer[i];
			// Conflict→Rename forces *As variant for this item (dst path differs from folder mode).
			const bool item_use_as = rename_mode_cap || !rename_override[i].empty();
			auto ut = ComputeUnitTotals(srcPath, isDir, it.size, dirInfo);
			WorkUnit u;
			u.display_name = StrMB2Wide(it.name);
			u.is_directory = isDir;
			u.total_bytes = ut.total_bytes;
			u.total_files = ut.total_files;
			u.execute = [srcAdb, dstAdb, srcPath, dstPath, dstDirCap, ow, isDir,
			             item_use_as, only_newer_cap, move_cap, idx = i,
			             fallback_ptr, lastErr_ptr](ProgressTracker& tr) -> int {
				if (only_newer_cap) {
					time_t src_mt = GetAdbMtime(*srcAdb, srcPath);
					time_t dst_mt = GetAdbMtime(*dstAdb, dstPath);
					if (src_mt > 0 && dst_mt > 0 && src_mt <= dst_mt) {
						tr.MarkSkipped();
						return 0;
					}
				}
				std::string aside;
				bool have_aside = false;
				if (ow) {
					aside = AsideName(dstPath);
					if (dstAdb->MoveRemoteAs(dstPath, aside) == 0) {
						have_aside = true;
					} else {
						if (isDir) dstAdb->DeleteDirectory(dstPath);
						else       dstAdb->DeleteFile(dstPath);
					}
				}
				tr.PinNearDone();
				int rc;
				auto onAbort = [&tr]{ return tr.Aborted(); };
				if (item_use_as) {
					rc = move_cap ? srcAdb->MoveRemoteAs(srcPath, dstPath, onAbort)
					              : srcAdb->CopyRemoteAs(srcPath, dstPath, onAbort);
				} else {
					rc = move_cap ? srcAdb->MoveRemote(srcPath, dstDirCap, onAbort)
					              : srcAdb->CopyRemote(srcPath, dstDirCap, onAbort);
				}
				if (rc == 0) {
					if (have_aside) {
						if (isDir) dstAdb->DeleteDirectory(aside);
						else       dstAdb->DeleteFile(aside);
					}
					return 0;
				}
				if (have_aside) dstAdb->MoveRemoteAs(aside, dstPath);
				*lastErr_ptr = rc;
				fallback_ptr->push_back(idx);
				tr.MarkSkipped();  // primary skipped; fallback retries below
				return 0;
			};
			units.push_back(std::move(u));
		}
		if (!units.empty()) {
			auto br = RunBatch(move ? Lng(MMoveOnDevice) : Lng(MCopyOnDevice),
			                   StrMB2Wide(srcDir), StrMB2Wide(dstDir),
			                   std::move(units));
			okCount += br.success_count;
		}
	}

	if (!needFallback.empty()) {
		const char* tmpDir = getenv("TMPDIR");
		if (!tmpDir) tmpDir = "/tmp";
		char tmpTpl[PATH_MAX];
		snprintf(tmpTpl, sizeof(tmpTpl), "%s/adb_crossload_XXXXXX", tmpDir);
		char* tmpBase = mkdtemp(tmpTpl);
		if (!tmpBase) {
			WINPORT(SetLastError)(lastErr != 0 ? lastErr : EIO);
			return TRUE;
		}
		const std::string tmpRoot = tmpBase;
		DBG("CrossPanelCopyMoveSameDevice fallback via host tmp=%s items=%zu\n",
			tmpRoot.c_str(), needFallback.size());

		std::wstring popTitle = move ? Lng(MMoveOnDevice) : Lng(MCopyOnDevice);

		std::vector<WorkUnit> units;
		units.reserve(needFallback.size());
		auto srcAdb = _adbDevice;
		auto dstAdb = dst->_adbDevice;
		const std::string srcDirCap = srcDir;
		const std::string dstDirCap = dstDir;
		const std::string tmpRootCap = tmpRoot;

		// Pre-scan source dirs — fallback = tmp roundtrip, needs per-file map for PushDirectory idx-lookup.
		std::vector<std::string> fbDirs;
		for (size_t idx : needFallback) if (items[idx].is_dir)
			fbDirs.push_back(ADBUtils::JoinPath(srcDirCap, items[idx].name));
		auto fbDirInfo = PrescanDeviceDirs(fbDirs);

		for (size_t idx : needFallback) {
			const auto& it = items[idx];
			const bool needOverwrite = overwrite[idx];
			const std::string srcPath = ADBUtils::JoinPath(srcDirCap, it.name);
			const std::string tmpPath = ADBUtils::JoinPath(tmpRootCap, it.name);
			const std::string dstPath = !rename_override[idx].empty() ? rename_override[idx]
			                          : (rename_mode ? rename_dst
			                                        : ADBUtils::JoinPath(dstDirCap, it.name));
			const bool isDir = it.is_dir;
			auto ut = ComputeUnitTotals(srcPath, isDir, it.size, fbDirInfo);

			WorkUnit u;
			u.display_name = StrMB2Wide(it.name);
			u.is_directory = isDir;
			u.total_bytes = ut.total_bytes;
			u.total_files = ut.total_files;
			u.execute = [srcAdb, dstAdb, isDir, move, needOverwrite,
			             srcPath, tmpPath, dstPath, idxMap = std::move(ut.idx)](ProgressTracker& tr) -> int {
				if (needOverwrite) {
					if (isDir) dstAdb->DeleteDirectory(dstPath);
					else       dstAdb->DeleteFile(dstPath);
				}
				auto onAbort = [&]() { return tr.Aborted(); };
				auto displayCb = [&](int pct, const std::string& path) {
					tr.TickDisplayOnly(pct, path);
				};
				int pullRc = isDir
					? srcAdb->PullDirectory(srcPath, tmpPath, displayCb, onAbort)
					: srcAdb->PullFile(srcPath, tmpPath, displayCb, onAbort);
				if (pullRc != 0) return pullRc;

				tr.Reset();
				auto pushCb = [&](int pct, const std::string& path) {
					tr.Tick(pct, path, isDir ? LookupSubitemSize(idxMap, path) : 0);
				};
				int pushRc = isDir
					? dstAdb->PushDirectory(tmpPath, dstPath, pushCb, onAbort)
					: dstAdb->PushFile(tmpPath, dstPath, pushCb, onAbort);
				(void)RemoveLocalPathRecursively(tmpPath);
				if (pushRc != 0) return pushRc;

				if (move) {
					int delRc = isDir ? srcAdb->DeleteDirectory(srcPath)
					                  : srcAdb->DeleteFile(srcPath);
					if (delRc != 0) return delRc;
				}
				return 0;
			};
			units.push_back(std::move(u));
		}

		auto br = RunBatch(popTitle, StrMB2Wide(srcDirCap), StrMB2Wide(dstDirCap),
		                   std::move(units));
		okCount += br.success_count;
		if (br.last_error != 0) lastErr = br.last_error;
		(void)RemoveLocalPathRecursively(tmpRoot);
	}

	if (okCount > 0) {
		RefreshBothPanelsClearingSelection();
		return TRUE;
	}

	if (lastErr != 0) {
		WINPORT(SetLastError)(lastErr);
	}
	return TRUE;
}


namespace {
struct ADBSelected {
	std::string name;
	bool is_dir = false;
	uint64_t size = 0;
};

bool GatherSelectedItems(std::vector<ADBSelected>& out) {
	auto getItem = [](int cmd, int index, PluginPanelItem& target, std::vector<char>& buf) -> bool {
		intptr_t size = g_Info.Control(PANEL_ACTIVE, cmd, index, 0);
		if (size < (intptr_t)sizeof(PluginPanelItem)) return false;
		buf.assign((size_t)size + kPanelItemAllocPad, 0);
		auto* item = (PluginPanelItem*)buf.data();
		if (!g_Info.Control(PANEL_ACTIVE, cmd, index, (LONG_PTR)(void*)item)) return false;
		target = *item;
		return true;
	};

	PanelInfo pi = {};
	if (g_Info.Control(PANEL_ACTIVE, FCTL_GETPANELINFO, 0, (LONG_PTR)(void*)&pi) == 0) {
		return false;
	}

	auto pushItem = [&](const PluginPanelItem& it) {
		if (!it.FindData.lpwszFileName) return;
		std::string name = StrWide2MB(it.FindData.lpwszFileName);
		if (name.empty() || name == "." || name == "..") return;
		const bool is_dir = (it.FindData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) != 0;
		out.push_back(ADBSelected{name, is_dir, is_dir ? 0 : it.FindData.nFileSize});
	};

	if (pi.SelectedItemsNumber > 0) {
		for (int i = 0; i < pi.SelectedItemsNumber; ++i) {
			PluginPanelItem it = {};
			std::vector<char> buf;
			if (!getItem(FCTL_GETSELECTEDPANELITEM, i, it, buf)) continue;
			pushItem(it);
		}
	} else {
		PluginPanelItem it = {};
		std::vector<char> buf;
		if (getItem(FCTL_GETCURRENTPANELITEM, 0, it, buf)) pushItem(it);
	}
	return !out.empty();
}
} // namespace

bool ADBPlugin::ShiftF5CopyInPlace() {
	if (!_isConnected || !_adbDevice || _deviceSerial.empty()) return false;

	std::vector<ADBSelected> items;
	if (!GatherSelectedItems(items)) return false;

	const std::string dir = GetCurrentDevicePath();
	if (dir.empty()) return false;
	const bool multi = items.size() > 1;

	// Prefill basename-only: single→"<name>.copy" (stock duplicate); multi→"" (user types dir).
	std::string typed;
	{
		std::string prefill = multi ? std::string("") : (items[0].name + ".copy");
		if (!ADBDialogs::AskInput(L"Copy", L"Copy to:", L"ADB_Copy", typed, prefill)) {
			return true;  // user cancelled — claim keystroke
		}
		if (typed.empty()) return true;
	}

	{
		const ParsedDest pd = ParseDestInput(typed, dir);
		if (pd.kind == DestKind::LOCAL) {
			DBG("ShiftF5: LOCAL target abs=%s trailing_slash=%d items=%zu\n",
				pd.abs_path.c_str(), pd.trailing_slash, items.size());
			const bool into_dir = multi || pd.trailing_slash;
			if (into_dir) (void)MkdirPLocal(pd.abs_path);
			else {
				auto slash = pd.abs_path.find_last_of('/');
				if (slash != std::string::npos && slash > 0) {
					(void)MkdirPLocal(pd.abs_path.substr(0, slash));
				}
			}
			int overwriteMode = 0;
			std::vector<int> per_action(items.size(), CA_PROCEED);
			std::vector<std::string> renamed(items.size());
			auto src_adb = _adbDevice;
			for (size_t i = 0; i < items.size(); ++i) {
				const std::string localDst = into_dir
					? (pd.abs_path + "/" + items[i].name)
					: pd.abs_path;
				struct stat st{};
				if (stat(localDst.c_str(), &st) != 0) continue;
				const std::string srcPath = ADBUtils::JoinPath(dir, items[i].name);
				const bool isDir = items[i].is_dir;
				uint64_t src_sz = 0, dst_sz = 0;
				int64_t src_mt = 0, dst_mt = 0;
				FillFileMeta(src_adb, srcPath,  isDir, src_sz, src_mt);
				FillFileMeta(nullptr, localDst, isDir, dst_sz, dst_mt);
				ProgressState dummy;
				int act = CheckOverwrite(StrMB2Wide(localDst), items.size() > 1, isDir,
				                         overwriteMode, dummy,
				                         src_sz, src_mt, dst_sz, dst_mt,
				                         MakeOverwriteViewer(src_adb, srcPath, isDir),
				                         MakeOverwriteViewer(nullptr, localDst, isDir));
				if (act == CA_ABORT) return true;
				per_action[i] = act;
				if (act == CA_RENAME) {
					std::string fresh = FindFreeLocalName(localDst);
					if (fresh.empty()) per_action[i] = CA_SKIP;
					else renamed[i] = fresh;
				}
			}
			std::vector<std::string> dirsToScan;
			for (size_t i = 0; i < items.size(); ++i)
				if (per_action[i] != CA_SKIP && items[i].is_dir)
					dirsToScan.push_back(ADBUtils::JoinPath(dir, items[i].name));
			auto dirInfo = PrescanDeviceDirs(dirsToScan);

			std::vector<WorkUnit> units;
			auto adb = _adbDevice;
			for (size_t i = 0; i < items.size(); ++i) {
				if (per_action[i] == CA_SKIP) continue;
				const auto& it = items[i];
				const std::string srcPath = ADBUtils::JoinPath(dir, it.name);
				std::string localDst = !renamed[i].empty()
					? renamed[i]
					: (into_dir ? (pd.abs_path + "/" + it.name) : pd.abs_path);
				const bool isDir = it.is_dir;
				const bool only_newer = per_action[i] == CA_NEWER;
				const std::string srcCap = srcPath;
				auto ut = ComputeUnitTotals(srcPath, isDir, it.size, dirInfo);
				WorkUnit u;
				u.display_name = StrMB2Wide(it.name);
				u.is_directory = isDir;
				u.total_bytes = ut.total_bytes;
				u.total_files = ut.total_files;
				u.execute = [adb, srcCap, localDst, isDir, only_newer, idx = std::move(ut.idx)](ProgressTracker& tr) -> int {
					if (only_newer) {
						time_t s = GetAdbMtime(*adb, srcCap);
						time_t d = GetLocalMtime(localDst);
						if (s > 0 && d > 0 && s <= d) { tr.MarkSkipped(); return 0; }
					}
					auto onAbort = [&tr]{ return tr.Aborted(); };
					auto cb = [&tr, &idx, isDir](int p, const std::string& path) {
						tr.Tick(p, path, isDir ? LookupSubitemSize(idx, path) : 0);
					};
					return isDir ? adb->PullDirectory(srcCap, localDst, cb, onAbort)
					             : adb->PullFile(srcCap, localDst, cb, onAbort);
				};
				units.push_back(std::move(u));
			}
			auto br = RunBatch(Lng(MCopyToHost), StrMB2Wide(dir), StrMB2Wide(pd.abs_path),
			                   std::move(units));
			if (br.success_count == 0 && br.last_error != 0) WINPORT(SetLastError)(br.last_error);
			RefreshBothPanels();
			return true;
		}
	}

	// Folder-only: trailing slash, "..", ".", existing-dir match, or multi.
	std::string folder_target = ResolveAdbDestinationFor(dir, typed, "", _adbDevice);
	const bool folder_only = multi
		|| (typed.back() == '/')
		|| typed == ".."
		|| typed == "."
		|| _adbDevice->IsDirectory(folder_target);

	if (folder_only && !folder_target.empty()) {
		std::string trimmed = folder_target;
		if (trimmed.size() > 1 && trimmed.back() == '/') trimmed.pop_back();
		if (!_adbDevice->IsDirectory(trimmed)) {
			(void)MkdirPAdb(*_adbDevice, trimmed);
		}
	}

	const bool same_dir_as_source = (folder_only && (folder_target == dir
		|| folder_target == BuildPanelPathPrefix(dir)
		|| folder_target + "/" == BuildPanelPathPrefix(dir)));

	std::string single_new_name;
	if (!multi && !folder_only) {
		// Extract basename — resolver already placed it under the right parent.
		auto slash = typed.find_last_of('/');
		single_new_name = (slash == std::string::npos) ? typed : typed.substr(slash + 1);
		if (single_new_name.empty()) single_new_name = items[0].name;
	}

	struct Plan {
		std::string src_path;
		std::string dst_path;
		std::string new_name;
		bool is_dir;
		uint64_t size = 0;
		bool skip = false;
		bool overwrite = false;
		bool only_newer = false;
	};

	std::vector<Plan> plan;
	plan.reserve(items.size());
	bool overwrite_all = false, skip_all = false;
	// Pre-fetch dst dir listing once — avoids N+1 FileExists per item.
	std::unordered_set<std::string> dst_listing;
	if (folder_only && !folder_target.empty()) {
		std::string trimmed = folder_target;
		if (trimmed.size() > 1 && trimmed.back() == '/') trimmed.pop_back();
		_adbDevice->ListDirNames(trimmed, dst_listing);
	}
	for (const auto& it : items) {
		Plan p;
		// single → typed name; multi/same-dir → ".copy"; multi/elsewhere → src.
		if (!multi && !folder_only) {
			p.new_name = single_new_name;
		} else if (same_dir_as_source) {
			p.new_name = it.name + ".copy";
		} else {
			p.new_name = it.name;
		}
		p.src_path = ADBUtils::JoinPath(dir, it.name);
		// dst_path: place new_name under the resolved folder target.
		std::string folder_with_slash = folder_target;
		if (folder_with_slash.empty() || folder_with_slash.back() != '/') folder_with_slash += '/';
		p.dst_path = folder_only
			? NormalizePosixPath(folder_with_slash + p.new_name)
			: ResolveAdbDestinationFor(dir, typed, "", _adbDevice);
		p.is_dir = it.is_dir;
		p.size = it.size;

		const bool exists = folder_only
			? dst_listing.count(p.new_name) > 0
			: _adbDevice->FileExists(p.dst_path);
		if (exists) {
			if (skip_all)      { p.skip = true; }
			else if (overwrite_all) { p.overwrite = true; }
			else {
				uint64_t src_sz = 0, dst_sz = 0;
				int64_t src_mt = 0, dst_mt = 0;
				FillFileMeta(_adbDevice, p.src_path, p.is_dir, src_sz, src_mt);
				FillFileMeta(_adbDevice, p.dst_path, p.is_dir, dst_sz, dst_mt);
				OverwriteDialog dlg(StrMB2Wide(p.dst_path), multi, p.is_dir,
				                    src_sz, src_mt, dst_sz, dst_mt,
				                    MakeOverwriteViewer(_adbDevice, p.src_path, p.is_dir),
				                    MakeOverwriteViewer(_adbDevice, p.dst_path, p.is_dir));
				switch (dlg.Ask()) {
					case OverwriteDialog::OVERWRITE:     p.overwrite = true; break;
					case OverwriteDialog::SKIP:          p.skip = true; break;
					case OverwriteDialog::OVERWRITE_ALL: overwrite_all = true; p.overwrite = true; break;
					case OverwriteDialog::SKIP_ALL:      skip_all = true; p.skip = true; break;
					case OverwriteDialog::RENAME: {
						std::string fresh = FindFreeAdbName(*_adbDevice, p.dst_path);
						if (fresh.empty()) p.skip = true;
						else { p.dst_path = fresh; p.new_name = ADBUtils::PathBasename(fresh); }
						break;
					}
					case OverwriteDialog::ONLY_NEWER:    p.only_newer = true; break;
					case OverwriteDialog::CANCEL:
					default:
						return true;
				}
			}
		}
		plan.push_back(std::move(p));
	}

	int okCount = 0;
	int lastErr = 0;
	std::vector<size_t> needFallback;
	auto adb = _adbDevice;

	bool any_dir = false;
	for (const auto& p : plan) if (!p.skip && p.is_dir) { any_dir = true; break; }

	if (!any_dir) {
		for (size_t i = 0; i < plan.size(); ++i) {
			if (plan[i].skip) continue;
			if (plan[i].only_newer) {
				time_t s = GetAdbMtime(*adb, plan[i].src_path);
				time_t d = GetAdbMtime(*adb, plan[i].dst_path);
				if (s > 0 && d > 0 && s <= d) continue;  // not newer
			}
			std::string aside; bool have_aside = false;
			if (plan[i].overwrite) {
				aside = AsideName(plan[i].dst_path);
				if (adb->MoveRemoteAs(plan[i].dst_path, aside) == 0) have_aside = true;
				else adb->DeleteFile(plan[i].dst_path);
			}
			const int rc = adb->CopyRemoteAs(plan[i].src_path, plan[i].dst_path);
			if (rc == 0) {
				++okCount;
				if (have_aside) adb->DeleteFile(aside);
			} else {
				if (have_aside) adb->MoveRemoteAs(aside, plan[i].dst_path);
				lastErr = rc;
				needFallback.push_back(i);
			}
		}
	} else {
		std::vector<std::string> dirsToScan;
		for (const auto& p : plan) if (p.is_dir && !p.skip) dirsToScan.push_back(p.src_path);
		auto dirInfo = PrescanDeviceDirs(dirsToScan);
		// Single multi-unit RunBatch — aside-rename + cp + finalize live inside execute.
		auto* fallback_ptr = &needFallback;
		auto* lastErr_ptr  = &lastErr;
		std::vector<WorkUnit> units;
		units.reserve(plan.size());
		for (size_t i = 0; i < plan.size(); ++i) {
			const Plan& p = plan[i];
			auto ut = ComputeUnitTotals(p.src_path, p.is_dir, p.size, dirInfo);
			WorkUnit u;
			u.display_name = StrMB2Wide(p.new_name);
			u.is_directory = p.is_dir;
			u.total_bytes = ut.total_bytes;
			u.total_files = ut.total_files;
			u.execute = [adb, p, idx = i, fallback_ptr, lastErr_ptr](ProgressTracker& tr) -> int {
				if (p.skip) { tr.MarkSkipped(); return 0; }
				if (p.only_newer) {
					time_t s = GetAdbMtime(*adb, p.src_path);
					time_t d = GetAdbMtime(*adb, p.dst_path);
					if (s > 0 && d > 0 && s <= d) { tr.MarkSkipped(); return 0; }
				}
				std::string aside; bool have_aside = false;
				if (p.overwrite) {
					aside = AsideName(p.dst_path);
					if (adb->MoveRemoteAs(p.dst_path, aside) == 0) have_aside = true;
					else if (p.is_dir) adb->DeleteDirectory(p.dst_path);
					else               adb->DeleteFile(p.dst_path);
				}
				tr.PinNearDone();
				const int rc = adb->CopyRemoteAs(p.src_path, p.dst_path,
				                                 [&tr]{ return tr.Aborted(); });
				if (rc == 0) {
					if (have_aside) {
						if (p.is_dir) adb->DeleteDirectory(aside);
						else          adb->DeleteFile(aside);
					}
					return 0;
				}
				if (have_aside) adb->MoveRemoteAs(aside, p.dst_path);
				*lastErr_ptr = rc;
				fallback_ptr->push_back(idx);
				tr.MarkSkipped();  // primary didn't credit; fallback will retry
				return 0;
			};
			units.push_back(std::move(u));
		}
		auto br = RunBatch(Lng(MCopyOnDevice), L"", L"", std::move(units));
		okCount += br.success_count;
	}

	if (!needFallback.empty()) {
		const char* tmpDir = getenv("TMPDIR");
		if (!tmpDir) tmpDir = "/tmp";
		char tmpTpl[PATH_MAX];
		snprintf(tmpTpl, sizeof(tmpTpl), "%s/adb_shiftf5_XXXXXX", tmpDir);
		char* tmpBase = mkdtemp(tmpTpl);
		if (tmpBase) {
			const std::string tmpRoot = tmpBase;
			DBG("ShiftF5: host-mediated fallback for %zu item(s) tmp=%s\n",
				needFallback.size(), tmpRoot.c_str());

			std::vector<std::string> fbDirs;
			for (size_t idx : needFallback) if (plan[idx].is_dir) fbDirs.push_back(plan[idx].src_path);
			auto fbDirInfo = PrescanDeviceDirs(fbDirs);

			std::vector<WorkUnit> units;
			units.reserve(needFallback.size());
			auto adb = _adbDevice;
			const std::string tmpRootCap = tmpRoot;
			for (size_t idx : needFallback) {
				auto& p = plan[idx];
				const bool isDir = p.is_dir;
				auto ut = ComputeUnitTotals(p.src_path, isDir, p.size, fbDirInfo);
				const std::string staged = ADBUtils::JoinPath(tmpRootCap, p.new_name);
				const std::string srcPathCap = p.src_path;
				const std::string dstPathCap = p.dst_path;

				WorkUnit u;
				u.display_name = StrMB2Wide(p.new_name);
				u.is_directory = isDir;
				u.total_bytes = ut.total_bytes;
				u.total_files = ut.total_files;
				u.execute = [adb, isDir, srcPathCap, staged, dstPathCap, idxMap = std::move(ut.idx)](ProgressTracker& tr) -> int {
					auto onAbort = [&]() { return tr.Aborted(); };
					auto displayCb = [&](int pct, const std::string& path) {
						tr.TickDisplayOnly(pct, path);
					};
					int pullRc = isDir
						? adb->PullDirectory(srcPathCap, staged, displayCb, onAbort)
						: adb->PullFile(srcPathCap, staged, displayCb, onAbort);
					if (pullRc != 0) return pullRc;

					tr.Reset();
					auto pushCb = [&, isDir](int pct, const std::string& path) {
						tr.Tick(pct, path, isDir ? LookupSubitemSize(idxMap, path) : 0);
					};
					int pushRc = isDir
						? adb->PushDirectory(staged, dstPathCap, pushCb, onAbort)
						: adb->PushFile(staged, dstPathCap, pushCb, onAbort);
					(void)RemoveLocalPathRecursively(staged);
					return pushRc;
				};
				units.push_back(std::move(u));
			}
			auto br = RunBatch(Lng(MCopyOnDevice), L"", L"", std::move(units));
			okCount += br.success_count;
			if (br.last_error != 0) lastErr = br.last_error;
			(void)RemoveLocalPathRecursively(tmpRoot);
		}
	}

	if (okCount == 0 && lastErr != 0) {
		WINPORT(SetLastError)(lastErr);
		ADBDialogs::MessageWrapped(FMSG_WARNING | FMSG_MB_OK,
		                           Lng(MCopyFailed),
		                           Lng(MErrCpFallbackFailed));
	}
	RefreshBothPanels();
	return true;
}

bool ADBPlugin::ShiftF6Rename() {
	// Stock Far convention: rename file UNDER CURSOR (multi-selection ignored).
	if (!_isConnected || !_adbDevice || _deviceSerial.empty()) return false;

	intptr_t size = g_Info.Control(PANEL_ACTIVE, FCTL_GETCURRENTPANELITEM, 0, 0);
	if (size < (intptr_t)sizeof(PluginPanelItem)) return false;
	std::vector<char> buf((size_t)size + kPanelItemAllocPad, 0);
	auto* raw = (PluginPanelItem*)buf.data();
	if (!g_Info.Control(PANEL_ACTIVE, FCTL_GETCURRENTPANELITEM, 0, (LONG_PTR)(void*)raw)) return false;
	if (!raw->FindData.lpwszFileName) return false;
	const std::string nm = StrWide2MB(raw->FindData.lpwszFileName);
	if (nm.empty() || nm == "." || nm == "..") return false;
	ADBSelected it;
	it.name = nm;
	it.is_dir = (raw->FindData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) != 0;
	it.size = it.is_dir ? 0 : raw->FindData.nFileSize;

	const std::string dir = GetCurrentDevicePath();
	if (dir.empty()) return false;

	// Prefill basename-only (stock Shift+F6); user can type rel "subdir/x", abs "/abs", or "adb:/abs".
	const std::string prefill = it.name;
	std::string raw_input;
	if (!ADBDialogs::AskInput(L"Rename", L"Rename to:", L"ADB_Rename",
	                          raw_input, prefill)) {
		return true;
	}
	if (raw_input.empty() || raw_input == prefill) return true;

	const std::string srcPath = ADBUtils::JoinPath(dir, it.name);

	{
		const ParsedDest pd = ParseDestInput(raw_input, dir);
		if (pd.kind == DestKind::LOCAL) {
			DBG("ShiftF6: LOCAL target abs=%s trailing=%d\n", pd.abs_path.c_str(), pd.trailing_slash);
			std::string localDst = pd.trailing_slash
				? (pd.abs_path + "/" + it.name)
				: pd.abs_path;
			if (pd.trailing_slash) (void)MkdirPLocal(pd.abs_path);
			else {
				auto slash = localDst.find_last_of('/');
				if (slash != std::string::npos && slash > 0) (void)MkdirPLocal(localDst.substr(0, slash));
			}
			// Single-item collision check.
			bool only_newer = false;
			struct stat st{};
			if (stat(localDst.c_str(), &st) == 0) {
				int overwriteMode = 0;
				ProgressState dummy;
				uint64_t src_sz = 0, dst_sz = 0;
				int64_t src_mt = 0, dst_mt = 0;
				FillFileMeta(_adbDevice, srcPath,  it.is_dir, src_sz, src_mt);
				FillFileMeta(nullptr,    localDst, it.is_dir, dst_sz, dst_mt);
				int act = CheckOverwrite(StrMB2Wide(localDst), false, it.is_dir,
				                         overwriteMode, dummy,
				                         src_sz, src_mt, dst_sz, dst_mt,
				                         MakeOverwriteViewer(_adbDevice, srcPath,  it.is_dir),
				                         MakeOverwriteViewer(nullptr,    localDst, it.is_dir));
				if (act == CA_ABORT || act == CA_SKIP) return true;
				if (act == CA_RENAME) {
					std::string fresh = FindFreeLocalName(localDst);
					if (fresh.empty()) return true;
					localDst = fresh;
				} else if (act == CA_NEWER) {
					only_newer = true;
				}
			}
			auto adb = _adbDevice;
			const bool isDir = it.is_dir;
			const std::string srcCap = srcPath;
			auto dirInfo = isDir ? PrescanDeviceDirs({srcPath}) : std::map<std::string, DirMeta>{};
			auto ut = ComputeUnitTotals(srcPath, isDir, it.size, dirInfo);
			std::vector<WorkUnit> units;
			WorkUnit u;
			u.display_name = StrMB2Wide(it.name);
			u.is_directory = isDir;
			u.total_bytes = ut.total_bytes;
			u.total_files = ut.total_files;
			u.execute = [adb, srcCap, localDst, isDir, only_newer, idx = std::move(ut.idx)](ProgressTracker& tr) -> int {
				if (only_newer) {
					time_t s = GetAdbMtime(*adb, srcCap);
					time_t d = GetLocalMtime(localDst);
					if (s > 0 && d > 0 && s <= d) { tr.MarkSkipped(); return 0; }
				}
				auto onAbort = [&tr]{ return tr.Aborted(); };
				auto cb = [&tr, &idx, isDir](int p, const std::string& path) {
					tr.Tick(p, path, isDir ? LookupSubitemSize(idx, path) : 0);
				};
				int pullRc = isDir ? adb->PullDirectory(srcCap, localDst, cb, onAbort)
				                   : adb->PullFile(srcCap, localDst, cb, onAbort);
				if (pullRc != 0) return pullRc;
				return isDir ? adb->DeleteDirectory(srcCap) : adb->DeleteFile(srcCap);
			};
			units.push_back(std::move(u));
			auto br = RunBatch(Lng(MMoveToHost), StrMB2Wide(srcPath), StrMB2Wide(localDst),
			                   std::move(units));
			if (br.success_count == 0 && br.last_error != 0) WINPORT(SetLastError)(br.last_error);
			RefreshBothPanels();
			return true;
		}
	}

	const std::string dstPath = ResolveAdbDestinationFor(dir, raw_input, it.name, _adbDevice);
	if (dstPath == srcPath) return true;
	(void)EnsureAdbParentDirs(*_adbDevice, dstPath);

	// Dir mv across FS can take seconds → RunBatch with 99% pin + Esc-aborted `adb shell -c "mv"`. Files skip modal.
	auto adb = _adbDevice;
	auto runMv = [&](const std::string& s, const std::string& d) -> int {
		if (!it.is_dir) return adb->MoveRemoteAs(s, d);
		std::vector<WorkUnit> units;
		WorkUnit u;
		u.display_name = StrMB2Wide(it.name);
		u.is_directory = true;
		u.total_files = 1;
		u.execute = [adb, s, d](ProgressTracker& tr) -> int {
			tr.PinNearDone();
			return adb->MoveRemoteAs(s, d, [&tr]{ return tr.Aborted(); });
		};
		units.push_back(std::move(u));
		auto br = RunBatch(Lng(MRenameTitle), StrMB2Wide(s), StrMB2Wide(d), std::move(units));
		return br.success_count > 0 ? 0 : (br.last_error ? br.last_error : ECANCELED);
	};

	std::string finalDst = dstPath;
	if (_adbDevice->FileExists(dstPath)) {
		uint64_t src_sz = 0, dst_sz = 0;
		int64_t src_mt = 0, dst_mt = 0;
		FillFileMeta(_adbDevice, srcPath, it.is_dir, src_sz, src_mt);
		FillFileMeta(_adbDevice, dstPath, it.is_dir, dst_sz, dst_mt);
		OverwriteDialog dlg(StrMB2Wide(dstPath), false, it.is_dir,
		                    src_sz, src_mt, dst_sz, dst_mt,
		                    MakeOverwriteViewer(_adbDevice, srcPath, it.is_dir),
		                    MakeOverwriteViewer(_adbDevice, dstPath, it.is_dir));
		auto choice = dlg.Ask();
		if (choice == OverwriteDialog::SKIP || choice == OverwriteDialog::SKIP_ALL) {
			return true;
		}
		if (choice == OverwriteDialog::CANCEL) return true;
		if (choice == OverwriteDialog::RENAME) {
			std::string fresh = FindFreeAdbName(*_adbDevice, dstPath);
			if (fresh.empty()) return true;
			finalDst = fresh;
			// No aside-rename needed (target doesn't exist), do direct mv:
			(void)EnsureAdbParentDirs(*_adbDevice, finalDst);
			const int rc = runMv(srcPath, finalDst);
			if (rc != 0) {
				ADBDialogs::MessageWrapped(FMSG_WARNING | FMSG_MB_OK,
					Lng(MRenameFailed), StrMB2Wide(strerror(rc)));
				WINPORT(SetLastError)(rc);
				return true;
			}
			RefreshBothPanels();
			return true;
		}
		if (choice == OverwriteDialog::ONLY_NEWER) {
			time_t s = GetAdbMtime(*_adbDevice, srcPath);
			time_t d = GetAdbMtime(*_adbDevice, dstPath);
			if (s > 0 && d > 0 && s <= d) return true;  // not newer → skip
			// Otherwise proceed as overwrite.
		}
		// Atomic overwrite: aside-rename → rename → drop aside on ok / restore on fail.
		const std::string aside = AsideName(dstPath);
		const int aside_rc = _adbDevice->MoveRemoteAs(dstPath, aside);
		if (aside_rc != 0) {
			ADBDialogs::MessageWrapped(FMSG_WARNING | FMSG_MB_OK,
			                           Lng(MRenameFailed),
			                           Lng(MErrCouldNotMoveAside));
			return true;
		}
		const int rc = runMv(srcPath, dstPath);
		if (rc != 0) {
			// Restore so user doesn't lose data.
			_adbDevice->MoveRemoteAs(aside, dstPath);
			ADBDialogs::MessageWrapped(FMSG_WARNING | FMSG_MB_OK,
			                           Lng(MRenameFailed),
			                           StrMB2Wide(strerror(rc)));
			WINPORT(SetLastError)(rc);
			return true;
		}
		if (it.is_dir) _adbDevice->DeleteDirectory(aside);
		else           _adbDevice->DeleteFile(aside);
		RefreshBothPanels();
		return true;
	}

	const int rc = runMv(srcPath, dstPath);
	if (rc != 0) {
		ADBDialogs::MessageWrapped(FMSG_WARNING | FMSG_MB_OK,
		                           Lng(MRenameFailed),
		                           StrMB2Wide(strerror(rc)));
		WINPORT(SetLastError)(rc);
		return true;
	}
	RefreshBothPanels();
	return true;
}

int ADBPlugin::GetFileData(PluginPanelItem **pPanelItem, int *pItemsNumber)
{
	try {
		std::vector<PluginPanelItem> files;
		std::string current_path = _adbDevice->DirectoryEnum(GetCurrentDevicePath(), files);
		
		PluginPanelItem parentDir{};
		parentDir.FindData.lpwszFileName = ADBDevice::AllocateItemString("..");
		parentDir.FindData.dwFileAttributes = FILE_ATTRIBUTE_DIRECTORY;
		parentDir.FindData.dwUnixMode = S_IFDIR | 0755;
		files.insert(files.begin(), parentDir);
		
		DBG("Created '..' entry with attributes: 0x%x, mode: 0%o\n", 
			parentDir.FindData.dwFileAttributes, parentDir.FindData.dwUnixMode);
		
		*pItemsNumber = files.size();
		*pPanelItem = new PluginPanelItem[files.size()];
		
		for (size_t i = 0; i < files.size(); i++) {
			(*pPanelItem)[i] = files[i];
		}
		
		return files.size();
		
	} catch (const std::exception &ex) {
		*pItemsNumber = 1;
		*pPanelItem = new PluginPanelItem[1];
		
		PluginPanelItem &item = (*pPanelItem)[0];
		memset(&item, 0, sizeof(PluginPanelItem));
		
		item.FindData.dwFileAttributes = FILE_ATTRIBUTE_NORMAL;
		item.FindData.dwUnixMode = 0644;
		wchar_t *filename = ADBDevice::AllocateItemString("Error accessing device");
		if (!filename) {
			delete[] *pPanelItem;
			*pPanelItem = nullptr;
			*pItemsNumber = 0;
			return 0;
		}
		item.FindData.lpwszFileName = filename;
		
		return 1;
	}
}

int ADBPlugin::GetDeviceData(PluginPanelItem **pPanelItem, int *pItemsNumber)
{
	DBG("GetDeviceData: Starting device enumeration\n");

	auto deviceInfos = EnumerateDevices();

	// First entry is ".." → exits plugin back to host panel (no Esc/F10 needed).
	std::vector<PluginPanelItem> rows;
	{
		PluginPanelItem dotdot{};
		dotdot.FindData.lpwszFileName = ADBDevice::AllocateItemString("..");
		dotdot.FindData.dwFileAttributes = FILE_ATTRIBUTE_DIRECTORY;
		wchar_t **cd = new wchar_t*[3];
		cd[0] = ADBDevice::AllocateItemString("");
		cd[1] = ADBDevice::AllocateItemString("");
		cd[2] = ADBDevice::AllocateItemString("");
		dotdot.CustomColumnData = cd;
		dotdot.CustomColumnNumber = 3;
		rows.push_back(dotdot);
	}

	if (deviceInfos.empty()) {
		DBG("No ADB devices found\n");
		SetPanelTitle(_PanelTitle, Lng(MNoDevicesPanelTitle));
		PluginPanelItem placeholder{};
		placeholder.FindData.lpwszFileName = ADBDevice::AllocateItemString("<Not found>");
		placeholder.FindData.dwFileAttributes = FILE_ATTRIBUTE_NORMAL;
		wchar_t **cd = new wchar_t*[3];
		cd[0] = ADBDevice::AllocateItemString("<Connect device>");
		cd[1] = ADBDevice::AllocateItemString("");
		cd[2] = ADBDevice::AllocateItemString("");
		placeholder.CustomColumnData = cd;
		placeholder.CustomColumnNumber = 3;
		rows.push_back(placeholder);
	} else {
		SetPanelTitle(_PanelTitle, deviceInfos.size() > 1 ? Lng(MADBSelectDevice) : L"ADB");
		for (const auto& info : deviceInfos) {
			PluginPanelItem device{};
			device.FindData.lpwszFileName = ADBDevice::AllocateItemString(info.serial);
			device.FindData.dwFileAttributes = FILE_ATTRIBUTE_DIRECTORY;
			wchar_t **cd = new wchar_t*[3];
			cd[0] = ADBDevice::AllocateItemString(info.name);
			cd[1] = ADBDevice::AllocateItemString(info.model);
			cd[2] = ADBDevice::AllocateItemString(info.usb);
			device.CustomColumnData = cd;
			device.CustomColumnNumber = 3;
			rows.push_back(device);
		}
	}

	*pItemsNumber = (int)rows.size();
	*pPanelItem = new PluginPanelItem[rows.size()];
	for (size_t i = 0; i < rows.size(); i++) {
		(*pPanelItem)[i] = rows[i];
	}
	return (int)rows.size();
}

std::string ADBPlugin::GetDeviceFriendlyName(const std::string& serial)
{
	// Try to get device name from global settings first
	std::string cmd = "-s " + serial + " shell settings get global device_name";
	std::string output = ADBShell::adbExec(cmd);
	
	// Remove trailing newline if present
	if (!output.empty() && output.back() == '\n') {
		output.pop_back();
	}
	
	if (output.empty() || output == "null") {
		return "";
	}
	
	return output;
}

bool ADBPlugin::ByKey_TryEnterSelectedDevice()
{
	std::string deviceSerial = GetCurrentPanelItemDeviceName();
	if (deviceSerial == "..") {
		// Exit the plugin back to the host far2l panel.
		g_Info.Control(this, FCTL_CLOSEPLUGIN, 0, 0);
		return true;
	}
	if (deviceSerial.empty() || deviceSerial[0] == '<') {
		DBG("No device selected (placeholder item)\n");
		return false;
	}
	
	DBG("Connecting to selected device: %s\n", deviceSerial.c_str());
	
	if (!ConnectToDevice(deviceSerial)) {
		DBG("Failed to connect to device: %s\n", deviceSerial.c_str());
		return false;
	}
	
	_isConnected = true;
	_deviceSerial = deviceSerial;
	// Non-empty CurDir required: empty routes ".." through FAR's PopPlugin branch (filelist.cpp:2498).
	_CurrentDir = GetCurrentDevicePath();
	UpdatePanelTitle(_deviceSerial, _CurrentDir);
	
	// Refresh the panel to show the new directory contents
	g_Info.Control(PANEL_ACTIVE, FCTL_UPDATEPANEL, 0, 0);
	
	// Reset cursor position to top of the panel
	PanelRedrawInfo ri = {};
	ri.CurrentItem = ri.TopPanelItem = 0;
	g_Info.Control(PANEL_ACTIVE, FCTL_REDRAWPANEL, 0, (LONG_PTR)&ri);
	
	DBG("Successfully connected to device: %s\n", deviceSerial.c_str());
	return true;
}

int ADBPlugin::ExitDeviceFilePanel()
{
	if (_adbDevice) {
		_adbDevice->Disconnect();
		_adbDevice.reset();
	}

	_isConnected = false;
	_deviceSerial.clear();
	_CurrentDir.clear();
	_friendlyNamesCache.clear();
	SetPanelTitle(_PanelTitle, Lng(MPluginTitle));

	return 1;
}

std::string ADBPlugin::GetCurrentPanelItemDeviceName()
{
	// Get the currently focused/selected item from the panel (following NetRocks pattern)
	intptr_t size = g_Info.Control(PANEL_ACTIVE, FCTL_GETSELECTEDPANELITEM, 0, 0);
	if (size < (intptr_t)sizeof(PluginPanelItem)) {
		DBG("No selected item or invalid size: %ld\n", (long)size);
		return "";
	}
	
	PluginPanelItem *item = (PluginPanelItem *)malloc(static_cast<size_t>(size));
	if (!item) {
		DBG("Failed to allocate memory for panel item\n");
		return "";
	}

	memset(item, 0, static_cast<size_t>(size));

	if (!g_Info.Control(PANEL_ACTIVE, FCTL_GETSELECTEDPANELITEM, 0, (LONG_PTR)(void *)item)) {
		free(item);
		return "";
	}

	if (!item->FindData.lpwszFileName) {
		free(item);
		return "";
	}

	// Filename is the device serial — set during GetDeviceData enumeration.
	std::string deviceSerial = StrWide2MB(item->FindData.lpwszFileName);
	free(item);
	return deviceSerial;
}

bool ADBPlugin::ConnectToDevice(const std::string &deviceSerial)
{
	DBG("ConnectToDevice: deviceSerial='%s'\n", deviceSerial.c_str());
	try {
		_adbDevice = std::make_shared<ADBDevice>(deviceSerial);
		
		DBG("ConnectToDevice: ADBDevice created\n");
		
		if (!_adbDevice->IsConnected()) {
			DBG("ConnectToDevice: ADBDevice not connected\n");
			return false;
		}
		
		DBG("ConnectToDevice: Successfully connected\n");
		return true;
		
	} catch (const std::exception &ex) {
		DBG("ConnectToDevice: Exception: %s\n", ex.what());
		return false;
	}
}

std::vector<ADBPlugin::DeviceInfo> ADBPlugin::EnumerateDevices() {
	std::vector<DeviceInfo> devices;
	std::string output = ADBShell::adbExec("devices -l");
	if (output.empty()) return devices;

	std::istringstream stream(output);
	std::string line;
	if (std::getline(stream, line)) { /* skip header */ }

	while (std::getline(stream, line)) {
		if (line.empty()) continue;
		std::istringstream lineStream(line);
		std::string serial, status;
		if (lineStream >> serial >> status) {
			if (serial != "List" && serial != "daemon" && serial != "starting" && serial != "adb" && status == "device") {
				DeviceInfo info;
				info.serial = serial;
				info.status = status;
				std::string field;
				while (lineStream >> field) {
					if (field.find("model:") == 0) info.model = field.substr(6);
					else if (field.find("usb:") == 0) info.usb = field;
				}

				auto it = _friendlyNamesCache.find(serial);
				if (it != _friendlyNamesCache.end()) {
					info.name = it->second;
				} else {
					info.name = GetDeviceFriendlyName(serial);
					if (!info.name.empty()) {
						_friendlyNamesCache[serial] = info.name;
					}
				}

				if (info.name.empty()) info.name = info.model.empty() ? serial : info.model;
				devices.push_back(info);
			}
		}
	}
	return devices;
}

PluginStartupInfo *ADBPlugin::GetInfo()
{
	return &g_Info;
}

// Recursive walk. perFileMap (if non-null) keys are paths RELATIVE to scan root, matching adb push -p output.
static void ScanLocalDirectoryImpl(const std::string& path, const std::string& relPrefix,
                                    uint64_t& totalSize, uint64_t& totalFiles,
                                    std::unordered_map<std::string, uint64_t>* perFileMap) {
    DIR* dir = opendir(path.c_str());
    if (!dir) return;

    struct dirent* ent;
    while ((ent = readdir(dir)) != nullptr) {
        if (strcmp(ent->d_name, ".") == 0 || strcmp(ent->d_name, "..") == 0) continue;
        std::string subPath = ADBUtils::JoinPath(path, ent->d_name);
        std::string subRel = relPrefix.empty() ? std::string(ent->d_name)
                                                : relPrefix + "/" + ent->d_name;
        struct stat st;
        if (stat(subPath.c_str(), &st) == 0) {
            if (S_ISDIR(st.st_mode)) {
                ScanLocalDirectoryImpl(subPath, subRel, totalSize, totalFiles, perFileMap);
            } else {
                totalSize += st.st_size;
                totalFiles++;
                if (perFileMap) (*perFileMap)[subRel] = st.st_size;
            }
        }
    }
    closedir(dir);
}

static void ScanLocalDirectory(const std::string& path, uint64_t& totalSize, uint64_t& totalFiles,
                                std::unordered_map<std::string, uint64_t>* perFileMap = nullptr) {
    ScanLocalDirectoryImpl(path, "", totalSize, totalFiles, perFileMap);
}

int ADBPlugin::ProcessEventCommand(const wchar_t *cmd, HANDLE hPlugin)
{
	DBG("Called with cmd='%ls'\n", cmd ? cmd : L"NULL");

	if (!cmd) return FALSE;
	if (!_isConnected || !_adbDevice) return FALSE;

	const wchar_t *commandToExecute = cmd;
	if (wcsncasecmp(cmd, L"adb:", 4) == 0) {
		commandToExecute = cmd + 4;
	}

	while (*commandToExecute == L' ') commandToExecute++;
	if (*commandToExecute == L'\0') return FALSE;

	std::string command = StrWide2MB(commandToExecute);
	DBG("to-device command='%s' (len=%zu)\n", command.c_str(), command.size());

	// Use persistent stateful session
	std::string output = _adbDevice->RunShellCommand(command);
	int exitCode = _adbDevice->LastShellExitCode();
	DBG("raw output length=%zu bytes exit=%d\n", output.size(), exitCode);

	// Sync path after every command - run pwd to get current directory
	_adbDevice->SyncPath();
	std::string newPath = _adbDevice->GetCurrentPath();
	if (newPath != _CurrentDir) {
		_CurrentDir = newPath;
		UpdatePanelTitle(_deviceSerial, _CurrentDir);
	}
	DBG("cwd='%s' rc=%d\n", newPath.c_str(), exitCode);

	// Render to far2l user screen (Ctrl+O area) with a "<cwd> $ <cmd>" prefix echoing what the user typed.
	std::wstring batch;
	batch += StrMB2Wide(_CurrentDir);
	batch += L" $ ";
	batch += StrMB2Wide(command);
	batch += L"\r\n";

	size_t batch_before_output = batch.size();
	{
		std::istringstream iss(output);
		std::string line;
		while (std::getline(iss, line)) {
			if (!line.empty() && line.back() == '\r') {
				line.pop_back();
			}
			if (line.empty()) {
				continue;
			}
			// Skip bare shell prompts leaked by the marker protocol (e.g. "/$", "#")
			if (line == "/$" || line == "/#" || line == "$" || line == "#") {
				continue;
			}
			batch += StrMB2Wide(line);
			batch += L"\r\n";
		}
	}

	// Only flag silent failures: empty output + non-zero exit. Skip for grep/diff/test — they use non-zero as semantic "no/false" and have output of their own.
	const bool output_was_empty = (batch.size() == batch_before_output);
	if (output_was_empty && exitCode != 0 && exitCode != -1) {
		batch += L"->[Exit code: ";
		batch += std::to_wstring(exitCode);
		batch += L"]\r\n";
	}

	DBG("batch wchars=%zu output_empty=%d rc=%d\n",
		batch.size(), output_was_empty ? 1 : 0, exitCode);

	// Temporarily enable ENABLE_PROCESSED_OUTPUT: panel mode leaves it off so \r\n would render as CP437 glyphs (♪◙). Same trick as FarExecuteScope.
	g_Info.Control(hPlugin, FCTL_GETUSERSCREEN, 0, 0);
	DWORD saved_mode = 0;
	WINPORT(GetConsoleMode)(NULL, &saved_mode);
	WINPORT(SetConsoleMode)(NULL, saved_mode | ENABLE_PROCESSED_OUTPUT | ENABLE_WRAP_AT_EOL_OUTPUT);
	DWORD dw = 0;
	WINPORT(WriteConsole)(NULL, batch.c_str(), (DWORD)batch.size(), &dw, NULL);
	WINPORT(SetConsoleMode)(NULL, saved_mode);
	DBG("WriteConsole wrote=%u of %zu wchars (saved_mode=0x%lx)\n",
		(unsigned)dw, batch.size(), (unsigned long)saved_mode);
	g_Info.Control(hPlugin, FCTL_SETUSERSCREEN, 0, 0);

	g_Info.Control(hPlugin, FCTL_SETCMDLINE, 0, (LONG_PTR)L"");

	// Unconditional UPDATEPANEL so rm/mkdir/touch/mv are reflected without Ctrl+R; FCTL_SETPANELDIR would close the plugin. CmdLine prompt lags one command (far2l cmdline.cpp:527-544 bug).
	g_Info.Control(PANEL_ACTIVE, FCTL_UPDATEPANEL, 0, 0);
	g_Info.Control(PANEL_ACTIVE, FCTL_REDRAWPANEL, 0, 0);
	g_Info.Control(PANEL_PASSIVE, FCTL_REDRAWPANEL, 0, 0);

	return TRUE;
}


int ADBPlugin::SetDirectory(const wchar_t *Dir, int OpMode)
{
	if (!Dir || wcslen(Dir) == 0) return FALSE;

	std::string target_dir = StrWide2MB(Dir);

	// Device-selector mode: ".." closes the plugin back to host panel.
	if (!_isConnected) {
		if (target_dir == "..") {
			g_Info.Control(this, FCTL_CLOSEPLUGIN, 0, 0);
			return TRUE;
		}
		return FALSE;
	}
	if (!_adbDevice) return FALSE;

	// Strip the "/<serial>" prefix added in GetOpenPluginInfo so absolute paths round-trip to the shell.
	const std::string serial_prefix = "/" + _deviceSerial;
	const std::string serial_prefix_slash = serial_prefix + "/";
	if (target_dir == serial_prefix) {
		target_dir = "/";
	} else if (target_dir.compare(0, serial_prefix_slash.size(), serial_prefix_slash) == 0) {
		target_dir = target_dir.substr(serial_prefix.size());
	}

	// ".." at device root exits to the device-selection panel; FAR runs Update() itself after TRUE.
	if (target_dir == ".." && _adbDevice->GetCurrentPath() == "/") {
		ExitDeviceFilePanel();
		return TRUE;
	}

	if (!_adbDevice->SetDirectory(target_dir)) {
		return FALSE;
	}

	_CurrentDir = _adbDevice->GetCurrentPath();
	UpdatePanelTitle(_deviceSerial, _CurrentDir);
	return TRUE;
}

int ADBPlugin::GetFiles(PluginPanelItem *PanelItem, int ItemsNumber, int Move, const wchar_t **DestPath, int OpMode) {
	DBG("ItemsNumber=%d, Move=%d, OpMode=0x%x\n", ItemsNumber, Move, OpMode);
	
	if (ItemsNumber <= 0 || !_isConnected || !_adbDevice || !PanelItem || !DestPath) {
		return FALSE;
	}
	
	std::string destPath;
	if (DestPath && DestPath[0]) {
		destPath = StrWide2MB(DestPath[0]);
	} else {
		destPath = GetCurrentDevicePath();
	}
	
	// far2l's plugin→non-plugin path skips its dialog → we prompt here. Plugin→plugin sets OPM_SILENT.
	if (!(OpMode & OPM_SILENT)) {
		std::string firstName = (ItemsNumber > 0) ? StrWide2MB(PanelItem[0].FindData.lpwszFileName) : "";
		if (!ADBDialogs::AskCopyMove(Move != 0, false, destPath, firstName, ItemsNumber)) {
			return -1;
		}
	}

	// Routing via unified ParseDestInput. ADB → in-device cp/mv; LOCAL → fall through to host pull.
	if (!(OpMode & OPM_VIEW)) {
		const std::string srcDir = GetCurrentDevicePath();
		const ParsedDest pd = ParseDestInput(destPath, srcDir);
		// Rename intent: single item + ADB target + leaf path (no trailing slash) + name isn't "."/"..".
		bool rename_intent = false;
		std::string rename_dst;
		if (pd.kind == DestKind::ADB && ItemsNumber == 1 && !pd.empty_input && !pd.trailing_slash) {
			const bool is_dot_or_dotdot = (pd.cleaned_input == "." || pd.cleaned_input == "..");
			if (!is_dot_or_dotdot && !_adbDevice->IsDirectory(pd.abs_path)) {
				rename_intent = true;
				rename_dst = pd.abs_path;
			}
		}
		DBG("GetFiles route: input='%s' kind=%s abs=%s trailing=%d rename=%d items=%d\n",
			destPath.c_str(), (pd.kind == DestKind::ADB ? "ADB" : "LOCAL"),
			pd.abs_path.c_str(), pd.trailing_slash, rename_intent, ItemsNumber);
		if (pd.kind == DestKind::ADB) {
			int okCount = 0;
			int lastErr = 0;
			auto adb = _adbDevice;
			auto* lastErr_ptr = &lastErr;
			std::vector<WorkUnit> units;

			if (rename_intent) {
				std::string nm = StrWide2MB(PanelItem[0].FindData.lpwszFileName);
				const std::string srcPath = ADBUtils::JoinPath(srcDir, nm);
				const bool isDir = (PanelItem[0].FindData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) != 0;
				const std::string dstCap = rename_dst;
				const bool move_cap = Move != 0;
				auto dirInfo = isDir ? PrescanDeviceDirs({srcPath}) : std::map<std::string, DirMeta>{};
				auto ut = ComputeUnitTotals(srcPath, isDir, PanelItem[0].FindData.nFileSize, dirInfo);
				WorkUnit u;
				u.display_name = StrMB2Wide(nm);
				u.is_directory = isDir;
				u.total_bytes = ut.total_bytes;
				u.total_files = ut.total_files;
				u.execute = [adb, srcPath, dstCap, move_cap, lastErr_ptr]
				            (ProgressTracker& tr) -> int {
					if (int mkrc = EnsureAdbParentDirs(*adb, dstCap); mkrc != 0) {
						*lastErr_ptr = mkrc;
						return mkrc;
					}
					tr.PinNearDone();
					return move_cap ? adb->MoveRemoteAs(srcPath, dstCap, [&tr]{ return tr.Aborted(); })
					                : adb->CopyRemoteAs(srcPath, dstCap, [&tr]{ return tr.Aborted(); });
				};
				units.push_back(std::move(u));
				DBG("GetFiles: in-device rename src=%s dst=%s move=%d\n",
					srcPath.c_str(), dstCap.c_str(), Move);
			} else {
				// Folder target: mkdir-p, then cp/mv each item into it.
				const std::string& dstDir = pd.abs_path;
				DBG("GetFiles: in-device target srcDir=%s dstDir=%s move=%d\n",
					srcDir.c_str(), dstDir.c_str(), Move);
				if (!_adbDevice->IsDirectory(dstDir)) {
					if (int mkrc = MkdirPAdb(*_adbDevice, dstDir); mkrc != 0) {
						DBG("GetFiles: mkdir -p failed dst=%s err=%d\n", dstDir.c_str(), mkrc);
						lastErr = mkrc;
					}
				}
				// Per-item collision check: ask user once per conflict, accumulate Skip/Overwrite All state.
				int overwriteMode = 0;  // 0=ask, 1=overwrite_all, 2=skip_all
				std::vector<int> per_item_action(ItemsNumber, CA_PROCEED);
				std::vector<std::string> renamed_to(ItemsNumber);
				bool aborted = false;
				std::unordered_set<std::string> dst_listing;
				if (lastErr == 0) _adbDevice->ListDirNames(dstDir, dst_listing);
				if (lastErr == 0) {
					for (int i = 0; i < ItemsNumber && !aborted; ++i) {
						if (!PanelItem[i].FindData.lpwszFileName) continue;
						std::string nm = StrWide2MB(PanelItem[i].FindData.lpwszFileName);
						if (nm.empty() || nm == "." || nm == "..") continue;
						if (dst_listing.count(nm) == 0) continue;
						const std::string dstFull = ADBUtils::JoinPath(dstDir, nm);
						const std::string srcFull = ADBUtils::JoinPath(srcDir, nm);
						const bool isDir = (PanelItem[i].FindData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) != 0;
						ProgressState dummyState;
						uint64_t src_sz = 0, dst_sz = 0;
						int64_t src_mt = 0, dst_mt = 0;
						FillFileMeta(_adbDevice, srcFull, isDir, src_sz, src_mt);
						FillFileMeta(_adbDevice, dstFull, isDir, dst_sz, dst_mt);
						int act = CheckOverwrite(StrMB2Wide(dstFull), ItemsNumber > 1, isDir,
						                         overwriteMode, dummyState,
						                         src_sz, src_mt, dst_sz, dst_mt,
						                         MakeOverwriteViewer(_adbDevice, srcFull, isDir),
						                         MakeOverwriteViewer(_adbDevice, dstFull, isDir));
						if (act == CA_ABORT) { aborted = true; break; }
						per_item_action[i] = act;
						if (act == CA_RENAME) {
							std::string fresh = FindFreeAdbName(*_adbDevice, dstFull);
							if (fresh.empty()) per_item_action[i] = CA_SKIP;
							else renamed_to[i] = fresh;
						}
					}
				}
				if (aborted) {
					return FALSE;
				}
				if (lastErr == 0) {
					std::vector<std::string> dirsToScan;
					for (int i = 0; i < ItemsNumber; ++i) {
						if (per_item_action[i] == CA_SKIP) continue;
						if (!(PanelItem[i].FindData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)) continue;
						std::string nm2 = PanelItem[i].FindData.lpwszFileName
							? StrWide2MB(PanelItem[i].FindData.lpwszFileName) : "";
						if (nm2.empty() || nm2 == "." || nm2 == "..") continue;
						dirsToScan.push_back(ADBUtils::JoinPath(srcDir, nm2));
					}
					auto dirInfo = PrescanDeviceDirs(dirsToScan);
					for (int i = 0; i < ItemsNumber; ++i) {
						if (!PanelItem[i].FindData.lpwszFileName) continue;
						std::string nm = StrWide2MB(PanelItem[i].FindData.lpwszFileName);
						if (nm.empty() || nm == "." || nm == "..") continue;
						const int act = per_item_action[i];
						if (act == CA_SKIP) continue;
						const std::string srcPath = ADBUtils::JoinPath(srcDir, nm);
						const bool isDir = (PanelItem[i].FindData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) != 0;
						auto ut = ComputeUnitTotals(srcPath, isDir, PanelItem[i].FindData.nFileSize, dirInfo);
						const bool move_cap = Move != 0;
						const std::string dstDirCap = dstDir;
						const std::string dstFull = ADBUtils::JoinPath(dstDir, nm);
						const std::string renamedDst = renamed_to[i];
						const bool only_newer = (act == CA_NEWER);
						const bool use_renamed = !renamedDst.empty();
						WorkUnit u;
						u.display_name = StrMB2Wide(nm);
						u.is_directory = isDir;
						u.total_bytes = ut.total_bytes;
						u.total_files = ut.total_files;
						u.execute = [adb, srcPath, dstDirCap, dstFull, renamedDst, move_cap,
						             only_newer, use_renamed, lastErr_ptr]
						            (ProgressTracker& tr) -> int {
							if (only_newer) {
								time_t s = GetAdbMtime(*adb, srcPath);
								time_t d = GetAdbMtime(*adb, dstFull);
								if (s > 0 && d > 0 && s <= d) { tr.MarkSkipped(); return 0; }
							}
							tr.PinNearDone();
							auto onAbort = [&tr]{ return tr.Aborted(); };
							int rc;
							if (use_renamed) {
								// User picked Rename → use *As variant with the fresh path.
								rc = move_cap ? adb->MoveRemoteAs(srcPath, renamedDst, onAbort)
								              : adb->CopyRemoteAs(srcPath, renamedDst, onAbort);
							} else {
								rc = move_cap ? adb->MoveRemote(srcPath, dstDirCap, onAbort)
								              : adb->CopyRemote(srcPath, dstDirCap, onAbort);
							}
							if (rc != 0) *lastErr_ptr = rc;
							return rc;
						};
						units.push_back(std::move(u));
					}
				}
			}
			if (!units.empty()) {
				auto br = RunBatch(Move ? Lng(MMoveOnDevice) : Lng(MCopyOnDevice),
				                   StrMB2Wide(srcDir), StrMB2Wide(pd.abs_path),
				                   std::move(units));
				okCount = br.success_count;
				if (br.last_error != 0) lastErr = br.last_error;
			}
			if (okCount == 0 && lastErr != 0) WINPORT(SetLastError)(lastErr);
			RefreshBothPanelsClearingSelection();
			return okCount > 0 ? TRUE : FALSE;
		}
		DBG("GetFiles route: '%s' → host pull (local FS)\n", destPath.c_str());
	}

	if (OpMode & OPM_VIEW) {
		DBG("ItemsNumber=%d\n", ItemsNumber);
		if (ItemsNumber > 0) {
			std::string fileName = StrWide2MB(PanelItem[0].FindData.lpwszFileName);
			DBG("fileName='%s'\n", fileName.c_str());
			
			if (PanelItem[0].FindData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) {
				DBG("Directory, returning FALSE\n");
				return FALSE;
			}
			
			std::string devicePath = ADBUtils::JoinPath(GetCurrentDevicePath(), fileName);
			DBG("F3 View: devicePath='%s'\n", devicePath.c_str());
			
			std::string localPath = ADBUtils::JoinPath(destPath, fileName);
			DBG("localPath='%s'\n", localPath.c_str());
			
			int result = _adbDevice->PullFile(devicePath, localPath);
			DBG("PullFile result=%d\n", result);
			// `adb pull -a` preserves device mode (may be 0000/0640) — chmod for host viewer.
			if (result == 0) {
				(void)chmod(localPath.c_str(), 0644);
			}
			return (result == 0) ? TRUE : FALSE;
		}
		DBG("No items, returning FALSE\n");
		return FALSE;
	}

	// Batched pre-scan: one `find` over all top-level dirs — single ~250ms roundtrip regardless of N (was N×~250ms).
	std::vector<std::string> dirsToScan;
	for (int i = 0; i < ItemsNumber; i++) {
		if (PanelItem[i].FindData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) {
			std::string fileName = StrWide2MB(PanelItem[i].FindData.lpwszFileName);
			dirsToScan.push_back(ADBUtils::JoinPath(GetCurrentDevicePath(), fileName));
		}
	}
	std::map<std::string, std::unordered_map<std::string, uint64_t>> batchSizes;
	if (!dirsToScan.empty()) {
		_adbDevice->BatchDirectoryFileSizes(dirsToScan, batchSizes);
	}

	uint64_t totalBytes = 0, totalFiles = 0;
	std::map<std::string, DirMeta> dirMetas;
	for (int i = 0; i < ItemsNumber; i++) {
		if (PanelItem[i].FindData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) {
			std::string fileName = StrWide2MB(PanelItem[i].FindData.lpwszFileName);
			std::string devicePath = ADBUtils::JoinPath(GetCurrentDevicePath(), fileName);
			DirMeta dm;
			auto it = batchSizes.find(devicePath);
			if (it != batchSizes.end()) {
				dm.file_sizes = std::move(it->second);
				dm.file_count = dm.file_sizes.size();
				for (const auto& kv : dm.file_sizes) dm.total_size += kv.second;
			}
			DBG("GetFiles batched dir='%s' files=%llu bytes=%llu\n", devicePath.c_str(),
				(unsigned long long)dm.file_count, (unsigned long long)dm.total_size);
			totalBytes += dm.total_size;
			totalFiles += dm.file_count;
			dirMetas[devicePath] = std::move(dm);
		} else {
			totalBytes += PanelItem[i].FindData.nFileSize;
			totalFiles++;
		}
	}

	return RunTransfer(PanelItem, ItemsNumber, false, Move != 0,
	                   destPath, GetCurrentDevicePath(), dirMetas,
	                   totalBytes, totalFiles, OpMode);
}

int ADBPlugin::PutFiles(PluginPanelItem *PanelItem, int ItemsNumber, int Move, const wchar_t *SrcPath, int OpMode) {
	DBG("ItemsNumber=%d, Move=%d, OpMode=0x%x\n", ItemsNumber, Move, OpMode);

	if (ItemsNumber <= 0 || !_isConnected || !_adbDevice || !PanelItem || !SrcPath) {
		return FALSE;
	}

	std::string srcPath = StrWide2MB(SrcPath);

	// PutFiles: far2l's ShellCopy already prompts collisions — don't double-prompt.

	uint64_t totalBytes = 0;
	uint64_t totalFiles = 0;
	std::map<std::string, DirMeta> dirMetas;

	try {
		for (int i = 0; i < ItemsNumber; i++) {
			std::string fileName = StrWide2MB(PanelItem[i].FindData.lpwszFileName);
			std::string localPath = ADBUtils::JoinPath(srcPath, fileName);

			if (PanelItem[i].FindData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) {
				DirMeta dm;
				dm.total_size = 0;
				dm.file_count = 0;
				ScanLocalDirectory(localPath, dm.total_size, dm.file_count, &dm.file_sizes);
				totalBytes += dm.total_size;
				totalFiles += dm.file_count;
				DBG("PutFiles scan dir='%s' files=%llu bytes=%llu sizes_map=%zu\n",
					localPath.c_str(), (unsigned long long)dm.file_count,
					(unsigned long long)dm.total_size, dm.file_sizes.size());
				dirMetas[localPath] = std::move(dm);
			} else {
				totalBytes += PanelItem[i].FindData.nFileSize;
				totalFiles++;
				DBG("PutFiles scan file='%s' size=%llu\n", localPath.c_str(),
					(unsigned long long)PanelItem[i].FindData.nFileSize);
			}
		}
	} catch (const std::exception& ex) {
		DBG("PutFiles pre-scan exception: %s\n", ex.what());
		WINPORT(SetLastError)(EIO);
		return FALSE;
	}
	DBG("PutFiles pre-scan TOTAL items=%d files=%llu bytes=%llu\n",
		ItemsNumber, (unsigned long long)totalFiles, (unsigned long long)totalBytes);

	return RunTransfer(PanelItem, ItemsNumber, true, Move != 0,
	                   srcPath, GetCurrentDevicePath(), dirMetas,
	                   totalBytes, totalFiles, OpMode);
}

std::map<std::string, ADBPlugin::DirMeta> ADBPlugin::PrescanDeviceDirs(const std::vector<std::string>& dirs)
{
	std::map<std::string, DirMeta> out;
	if (dirs.empty() || !_isConnected || !_adbDevice) return out;
	auto t0 = std::chrono::steady_clock::now();
	std::map<std::string, std::unordered_map<std::string, uint64_t>> raw;
	_adbDevice->BatchDirectoryFileSizes(dirs, raw);
	uint64_t total_files = 0, total_bytes = 0;
	for (auto& kv : raw) {
		DirMeta dm;
		dm.file_sizes = std::move(kv.second);
		dm.file_count = dm.file_sizes.size();
		for (const auto& fk : dm.file_sizes) dm.total_size += fk.second;
		total_files += dm.file_count;
		total_bytes += dm.total_size;
		out[kv.first] = std::move(dm);
	}
	auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(
		std::chrono::steady_clock::now() - t0).count();
	DBG("PrescanDeviceDirs dirs=%zu out=%zu files=%llu bytes=%llu in %lldms\n",
		dirs.size(), out.size(), (unsigned long long)total_files,
		(unsigned long long)total_bytes, (long long)ms);
	if (out.size() < dirs.size()) {
		DBG("PrescanDeviceDirs WARN missing %zu dir(s) — totals will fallback to 0/0\n",
			dirs.size() - out.size());
	}
	return out;
}

ADBPlugin::UnitTotals ADBPlugin::ComputeUnitTotals(
	const std::string& srcPath, bool isDir, uint64_t hostKnownSize,
	const std::map<std::string, DirMeta>& dirInfo) const
{
	UnitTotals t;
	if (!isDir) {
		t.total_bytes = hostKnownSize;
		t.total_files = 1;
		return t;
	}
	auto fit = dirInfo.find(srcPath);
	if (fit == dirInfo.end()) {
		// Prescan miss — leave 0/0; Tick gate prevents counter race.
		DBG("ComputeUnitTotals: prescan miss for dir='%s' — totals=0/0\n", srcPath.c_str());
		return t;
	}
	t.total_bytes = fit->second.total_size;
	t.total_files = fit->second.file_count;
	t.idx = BuildSubitemIndex(fit->second.file_sizes);
	return t;
}

int ADBPlugin::RunTransfer(PluginPanelItem *items, int itemsCount, bool is_upload, bool move,
                           const std::string& localDir, const std::string& deviceDir,
                           const std::map<std::string, DirMeta>& dirMetas,
                           uint64_t totalBytes, uint64_t totalFiles, int OpMode)
{
	int successCount = 0;
	int lastErrorCode = 0;
	auto adb = _adbDevice;

	if (OpMode & OPM_SILENT) {
		// Wrap: ADBDevice transfer methods may throw on device disconnect.
		try {
			for (int i = 0; i < itemsCount; i++) {
				std::string fileName = StrWide2MB(items[i].FindData.lpwszFileName);
				std::string localPath = ADBUtils::JoinPath(localDir, fileName);
				std::string devicePath = ADBUtils::JoinPath(deviceDir, fileName);
				bool isDir = (items[i].FindData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) != 0;

				int result;
				if (is_upload) {
					result = isDir ? adb->PushDirectory(localPath, devicePath)
					               : adb->PushFile(localPath, devicePath);
				} else {
					result = isDir ? adb->PullDirectory(devicePath, localPath)
					               : adb->PullFile(devicePath, localPath);
				}

				if (result == 0) {
					successCount++;
					if (move) {
						if (is_upload) {
							(void)RemoveLocalPathRecursively(localPath);
						} else {
							(void)(isDir ? adb->DeleteDirectory(devicePath)
							             : adb->DeleteFile(devicePath));
						}
					}
				} else {
					lastErrorCode = result;
				}
			}
		} catch (const std::exception& ex) {
			DBG("RunTransfer (silent) exception: %s\n", ex.what());
			lastErrorCode = EIO;
		}
	} else {
		std::wstring title = move
			? (is_upload ? Lng(MMoveToDevice) : Lng(MMoveFromDevice))
			: (is_upload ? Lng(MCopyToDevice) : Lng(MCopyFromDevice));

		auto overwriteMode = std::make_shared<int>(0);
		const bool isMultiple = itemsCount > 1;

		std::vector<WorkUnit> units;
		units.reserve(itemsCount);
		for (int i = 0; i < itemsCount; i++) {
			std::string fileName = StrWide2MB(items[i].FindData.lpwszFileName);
			std::string localPath = ADBUtils::JoinPath(localDir, fileName);
			std::string devicePath = ADBUtils::JoinPath(deviceDir, fileName);
			const bool isDir = (items[i].FindData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) != 0;
			const uint64_t itemSize = isDir ? 0 : items[i].FindData.nFileSize;

			std::string metaKey = is_upload ? localPath : devicePath;
			auto ut = ComputeUnitTotals(metaKey, isDir, itemSize, dirMetas);
			auto idx = std::move(ut.idx);

			WorkUnit u;
			u.display_name = StrMB2Wide(fileName);
			u.is_directory = isDir;
			u.total_bytes = ut.total_bytes;
			u.total_files = ut.total_files;

			u.execute = [adb, is_upload, isDir, isMultiple, move, overwriteMode,
			             localPath = std::move(localPath), devicePath = std::move(devicePath),
			             itemSize, idx = std::move(idx)](ProgressTracker& tr) -> int {
				bool dst_exists = false;
				if (is_upload) {
					dst_exists = adb->FileExists(devicePath);
				} else {
					struct stat st;
					if (stat(localPath.c_str(), &st) == 0) dst_exists = true;
				}
				DBG("RunTransfer unit: is_upload=%d isDir=%d dst_exists=%d local='%s' device='%s'\n",
					is_upload, isDir, dst_exists, localPath.c_str(), devicePath.c_str());
				// Mutable copies — RENAME may rewrite dst path, ONLY_NEWER may flip dst_exists.
				std::string activeLocal = localPath;
				std::string activeDevice = devicePath;
				if (dst_exists) {
					// is_upload=true → src is host, dst is device. is_upload=false → reverse.
					const std::string& src_path = is_upload ? activeLocal  : activeDevice;
					const std::string& dst_path = is_upload ? activeDevice : activeLocal;
					auto src_adb = is_upload ? std::shared_ptr<ADBDevice>() : adb;
					auto dst_adb = is_upload ? adb : std::shared_ptr<ADBDevice>();
					uint64_t src_size = 0, dst_size = 0;
					int64_t src_mtime = 0, dst_mtime = 0;
					FillFileMeta(src_adb, src_path, isDir, src_size, src_mtime);
					FillFileMeta(dst_adb, dst_path, isDir, dst_size, dst_mtime);
					int action = CheckOverwrite(
						StrMB2Wide(dst_path),
						isMultiple, isDir, *overwriteMode, tr.StateRef(),
						src_size, src_mtime, dst_size, dst_mtime,
						MakeOverwriteViewer(src_adb, src_path, isDir),
						MakeOverwriteViewer(dst_adb, dst_path, isDir));
					if (action == CA_SKIP || action == CA_ABORT) { tr.MarkSkipped(); return 0; }
					if (action == CA_RENAME) {
						std::string fresh = is_upload ? FindFreeAdbName(*adb, activeDevice)
						                              : FindFreeLocalName(activeLocal);
						if (fresh.empty()) { tr.MarkSkipped(); return 0; }
						if (is_upload) activeDevice = fresh; else activeLocal = fresh;
						dst_exists = false;  // new name is free
					} else if (action == CA_NEWER) {
						time_t srcMt = is_upload ? GetLocalMtime(activeLocal) : GetAdbMtime(*adb, activeDevice);
						time_t dstMt = is_upload ? GetAdbMtime(*adb, activeDevice) : GetLocalMtime(activeLocal);
						if (srcMt > 0 && dstMt > 0 && srcMt <= dstMt) { tr.MarkSkipped(); return 0; }
						// else: fall through as overwrite
					}
				}
				if (tr.Aborted()) return ECANCELED;

				auto cb = [&](int pct, const std::string& path) {
					uint64_t sz = isDir ? LookupSubitemSize(idx, path) : itemSize;
					tr.Tick(pct, path, sz);
				};
				auto onAbort = [&]() { return tr.Aborted(); };

				int rc;
				if (is_upload) {
					// Aside-rename for atomic overwrite (pre-delete loses data on push fail).
					std::string aside;
					bool have_aside = false;
					if (dst_exists) {
						aside = AsideName(activeDevice);
						if (adb->MoveRemoteAs(activeDevice, aside) == 0) {
							have_aside = true;
						} else {
							if (isDir) adb->DeleteDirectory(activeDevice);
							else       adb->DeleteFile(activeDevice);
						}
					}
					rc = isDir
						? adb->PushDirectory(activeLocal, activeDevice, cb, onAbort)
						: adb->PushFile(activeLocal, activeDevice, cb, onAbort);
					if (rc == 0) {
						if (have_aside) {
							if (isDir) adb->DeleteDirectory(aside);
							else       adb->DeleteFile(aside);
						}
					} else if (have_aside) {
						adb->MoveRemoteAs(aside, activeDevice);
					}
				} else {
					rc = isDir
						? adb->PullDirectory(activeDevice, activeLocal, cb, onAbort)
						: adb->PullFile(activeDevice, activeLocal, cb, onAbort);
				}

				// Move: delete original src on success (rename only mutates dst).
				if (rc == 0 && move) {
					if (is_upload) {
						(void)RemoveLocalPathRecursively(localPath);
					} else {
						(void)(isDir ? adb->DeleteDirectory(devicePath)
						             : adb->DeleteFile(devicePath));
					}
				}
				return rc;
			};
			units.push_back(std::move(u));
		}

		auto src_label = is_upload ? StrMB2Wide(localDir) : StrMB2Wide(deviceDir);
		auto dst_label = is_upload ? StrMB2Wide(deviceDir) : StrMB2Wide(localDir);
		auto br = RunBatch(title, src_label, dst_label, std::move(units));
		successCount  = br.success_count;
		lastErrorCode = br.last_error;
	}

	if (successCount == 0) {
		WINPORT(SetLastError)(lastErrorCode);
	}
	return (successCount > 0) ? TRUE : FALSE;
}

int ADBPlugin::ProcessHostFile(PluginPanelItem *PanelItem, int ItemsNumber, int OpMode)
{
	return TRUE;
}

int ADBPlugin::DeleteFiles(PluginPanelItem *PanelItem, int ItemsNumber, int OpMode) {
	if (ItemsNumber <= 0 || !_isConnected || !_adbDevice || !PanelItem) {
		return FALSE;
	}
	// Cursor on ".." with no selection — refuse silently (far2l still calls DeleteFiles for it).
	if (ItemsNumber == 1 && PanelItem[0].FindData.lpwszFileName) {
		std::string nm = StrWide2MB(PanelItem[0].FindData.lpwszFileName);
		if (nm == "." || nm == "..") return FALSE;
	}

	if (!(OpMode & OPM_SILENT)) {
		int fileCount = 0;
		int folderCount = 0;
		for (int i = 0; i < ItemsNumber; i++) {
			if (PanelItem[i].FindData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
				folderCount++;
			else
				fileCount++;
		}

		// Single confirmation dialog, use warning style for dirs/multiple items
		int result = 0;
		bool dangerous = (folderCount > 0 || ItemsNumber > 1);
		unsigned int flags = dangerous ? (FMSG_WARNING | FMSG_MB_YESNO) : FMSG_MB_YESNO;

		if (ItemsNumber == 1 && folderCount == 1) {
			result = ADBDialogs::Message(flags,
				Lng(MDeleteFolderTitle),
				Lng(MDeleteFolderQ),
				std::wstring(PanelItem[0].FindData.lpwszFileName));
		} else if (ItemsNumber == 1) {
			result = ADBDialogs::Message(flags,
				Lng(MDelete),
				Lng(MDeleteFileQ),
				std::wstring(PanelItem[0].FindData.lpwszFileName));
		} else if (fileCount > 0 && folderCount > 0) {
			result = ADBDialogs::Message(flags,
				Lng(MDeleteItems),
				Lng(MDeleteItemsQ),
				std::to_wstring(folderCount) + Lng(MFoldersAnd) + std::to_wstring(fileCount) + Lng(MFilesSuffix));
		} else if (folderCount > 0) {
			result = ADBDialogs::Message(flags,
				Lng(MDeleteFolders),
				Lng(MDeleteItemsQ),
				std::to_wstring(folderCount) + Lng(MFoldersSuffix));
		} else {
			result = ADBDialogs::Message(flags,
				Lng(MDeleteFiles),
				Lng(MDeleteItemsQ),
				std::to_wstring(fileCount) + Lng(MFilesSuffix));
		}

		if (result != 0) {
			return -1;
		}
	}
	
	int successCount = 0;
	int lastErrorCode = 0;

	// Capture variables for lambda
	PluginPanelItem* items = PanelItem;
	int itemsCount = ItemsNumber;
	std::string deviceDir = GetCurrentDevicePath();
	auto adb = _adbDevice;
	int* pSuccessCount = &successCount;
	int* pLastError = &lastErrorCode;

	if (OpMode & OPM_SILENT) {
		// Silent mode - no progress dialog
		try {
			for (int i = 0; i < itemsCount; i++) {
				std::string fileName = StrWide2MB(items[i].FindData.lpwszFileName);
				std::string devicePath = ADBUtils::JoinPath(deviceDir, fileName);

				int result = 0;
				if (items[i].FindData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) {
					result = adb->DeleteDirectory(devicePath);
				} else {
					result = adb->DeleteFile(devicePath);
				}

				if (result == 0) {
					(*pSuccessCount)++;
				} else {
					*pLastError = result;
				}
			}
		} catch (const std::exception& ex) {
			DBG("DeleteFiles (silent) exception: %s\n", ex.what());
			lastErrorCode = EIO;
		}
	} else {
		// DeleteProgressDialog shows current file during deletion.
		DeleteOperation op;
		op.GetState().count_total = itemsCount;

		op.Run([&](ProgressState& state) {
			// Worker-thread exception guard (std::terminate on escape).
			try {
			for (int i = 0; i < itemsCount && !state.ShouldAbort(); i++) {
				std::string fileName = StrWide2MB(items[i].FindData.lpwszFileName);
				std::string devicePath = ADBUtils::JoinPath(deviceDir, fileName);

				// Update state with current file
				{
					std::lock_guard<std::mutex> lock(state.mtx_strings);
					state.current_file = StrMB2Wide(fileName);
				}
				state.count_complete = i;

				int result = 0;
				if (items[i].FindData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) {
					result = adb->DeleteDirectory(devicePath);
				} else {
					result = adb->DeleteFile(devicePath);
				}

				if (result == 0 && !state.ShouldAbort()) {
					(*pSuccessCount)++;
				} else if (result != 0) {
					*pLastError = result;
				}
			}

			// Final update
			state.count_complete = itemsCount;
			} catch (const std::exception& ex) {
				DBG("DeleteFiles worker exception: %s\n", ex.what());
				*pLastError = EIO;
				state.SetAborting();
			} catch (...) {
				*pLastError = EIO;
				state.SetAborting();
			}
		});
	}

	if (successCount == 0) {
		WINPORT(SetLastError)(lastErrorCode);
	}
	// far2l auto-refreshes active after DeleteFiles; passive may show same dir.
	if (successCount > 0) {
		g_Info.Control(PANEL_PASSIVE, FCTL_UPDATEPANEL, 0, 0);
		g_Info.Control(PANEL_PASSIVE, FCTL_REDRAWPANEL, 0, 0);
	}

	return (successCount > 0) ? TRUE : FALSE;
}

int ADBPlugin::MakeDirectory(const wchar_t **Name, int OpMode)
{
	if (!_isConnected || !_adbDevice) {
		return FALSE;
	}
	
	std::string dir_name;
	if (Name && *Name) {
		dir_name = StrWide2MB(*Name);
	}

	// far2l's F7 already prompted for the name; only ask if it didn't.
	if (dir_name.empty() && !(OpMode & OPM_SILENT)) {
		if (!ADBDialogs::AskCreateDirectory(dir_name)) {
			return -1;
		}
	}
	
	if (dir_name.empty()) {
		return FALSE;
	}
	
	std::string devicePath = GetCurrentDevicePath();
	if (!devicePath.empty() && devicePath.back() != '/') {
		devicePath += "/";
	}
	devicePath += dir_name;
	
	int result = _adbDevice->CreateDirectory(devicePath);
	if (result == 0) {
		if (Name && !(OpMode & OPM_SILENT)) {
			std::wstring wdir = StrMB2Wide(dir_name);
			wcsncpy(_mk_dir, wdir.c_str(), ARRAYSIZE(_mk_dir) - 1);
			_mk_dir[ARRAYSIZE(_mk_dir) - 1] = L'\0';
			*Name = _mk_dir;
		}
		// far2l auto-refreshes active after MakeDirectory; passive may show same dir.
		g_Info.Control(PANEL_PASSIVE, FCTL_UPDATEPANEL, 0, 0);
		g_Info.Control(PANEL_PASSIVE, FCTL_REDRAWPANEL, 0, 0);
		return TRUE;
	} else {
		WINPORT(SetLastError)(result);
		return FALSE;
	}
}

std::string ADBPlugin::GetCurrentDevicePath() const
{
	if (_isConnected && _adbDevice) {
		return _adbDevice->GetCurrentPath();
	}
	return "/";
}

void ADBPlugin::UpdatePanelTitle(const std::string& deviceSerial, const std::string& path)
{
	std::wstring panel_title = StrMB2Wide(deviceSerial) + L":" + StrMB2Wide(path);
	if (panel_title.size() >= ARRAYSIZE(_PanelTitle)) {
		size_t rmlen = 4 + (panel_title.size() - ARRAYSIZE(_PanelTitle));
		panel_title.replace((panel_title.size() - rmlen) / 2, rmlen, L"...");
	}
	SetPanelTitle(_PanelTitle, panel_title.c_str());
}
