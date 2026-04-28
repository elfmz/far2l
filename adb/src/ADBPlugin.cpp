#include "ADBPlugin.h"
#include "ADBDevice.h"
#include "ADBShell.h"
#include "ADBDialogs.h"
#include "ADBLog.h"
#include <sstream>
#include <string>
#include <vector>
#include <map>
#include <set>
#include <mutex>
#include <cstring>
#include <cstdarg>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <dirent.h>
#include <errno.h>
#include <utils.h>
#include <farplug-wide.h>

// Returns 0=proceed / 1=skip / 2=abort; overwriteMode carries state (0=ask, 1=overwrite all, 2=skip all) across calls.
// state is optional — same-device F5/F6 has no progress UI to abort, so pass nullptr.
static int CheckOverwrite(const std::wstring& destPath, bool isMultiple, bool isDir,
                          int& overwriteMode, ProgressState* state) {
	if (overwriteMode == 1) return 0;  // Overwrite all
	if (overwriteMode == 2) return 1;  // Skip all

	OverwriteDialog dlg(destPath, isMultiple, isDir);
	switch (dlg.Ask()) {
		case OverwriteDialog::OVERWRITE:
			return 0;
		case OverwriteDialog::SKIP:
			return 1;
		case OverwriteDialog::OVERWRITE_ALL:
			overwriteMode = 1;
			return 0;
		case OverwriteDialog::SKIP_ALL:
			overwriteMode = 2;
			return 1;
		case OverwriteDialog::CANCEL:
		default:
			if (state) state->SetAborting();
			return 2;
	}
}

// Returns 0 on success, errno on failure. Reports the errno of the *first* failing
// syscall — successive errno-clobbering by readdir/closedir doesn't pollute the result.
static int RemoveLocalPathRecursively(const std::string& path) {
	struct stat st = {};
	if (lstat(path.c_str(), &st) != 0) {
		return errno ? errno : EIO;
	}
	if (S_ISDIR(st.st_mode)) {
		DIR* dir = opendir(path.c_str());
		if (!dir) {
			return errno ? errno : EIO;
		}
		int firstErr = 0;
		while (true) {
			errno = 0;
			dirent* ent = readdir(dir);
			if (!ent) {
				if (errno != 0 && firstErr == 0) firstErr = errno;
				break;
			}
			if (strcmp(ent->d_name, ".") == 0 || strcmp(ent->d_name, "..") == 0) {
				continue;
			}
			int childErr = RemoveLocalPathRecursively(path + "/" + ent->d_name);
			if (childErr != 0 && firstErr == 0) {
				firstErr = childErr;
				break;
			}
		}
		closedir(dir);
		if (firstErr != 0) {
			return firstErr;
		}
		return rmdir(path.c_str()) == 0 ? 0 : (errno ? errno : EIO);
	}
	return unlink(path.c_str()) == 0 ? 0 : (errno ? errno : EIO);
}

static bool NoControls(unsigned int control_state) {
	return (control_state & (PKF_CONTROL | PKF_ALT | PKF_SHIFT)) == 0;
}


PluginStartupInfo g_Info = {};
FarStandardFunctions g_FSF = {};

// Registry of live ADBPlugin instances — used to safely identify whether an opaque
// FCTL_GETPANELPLUGINHANDLE result is one of ours before dereferencing it.
// Mutex-guarded: far2l's UI-thread invariant covers ProcessKey/ctor/dtor, but the
// progress-UI worker runs in a std::thread; cheap insurance against future drift.
static std::mutex g_live_instances_mtx;
static std::set<const ADBPlugin*> g_live_instances;

bool ADBPlugin::IsLiveInstance(const void* candidate) {
	if (!candidate) return false;
	std::lock_guard<std::mutex> lock(g_live_instances_mtx);
	return g_live_instances.count(static_cast<const ADBPlugin*>(candidate)) > 0;
}

ADBPlugin::ADBPlugin(const wchar_t *path, bool path_is_standalone_config, int OpMode)
	: _allow_remember_location_dir(false)
	, _isConnected(false)
	, _deviceSerial("")
	, _CurrentDir("/")
{
	{
		std::lock_guard<std::mutex> lock(g_live_instances_mtx);
		g_live_instances.insert(this);
	}
	wcscpy(_PanelTitle, L"ADB");
	wcscpy(_mk_dir, L"");

	if (path && path_is_standalone_config) {
		_standalone_config = path;
	}

	// Auto-connect logic: enumerate once and reuse result to avoid race
	auto devices = EnumerateDevices();
	int deviceCount = (int)devices.size();

	if (deviceCount == 1) {
		std::string deviceSerial = devices[0].serial;

		if (!deviceSerial.empty()) {
			if (ConnectToDevice(deviceSerial)) {
				_isConnected = true;
				_deviceSerial = deviceSerial;
				// Update panel title using helper
				UpdatePanelTitle(_deviceSerial, GetCurrentDevicePath());
			}
		}
	} else if (deviceCount > 1) {
		// Multiple devices available, show device selection mode
		_isConnected = false;
		wcscpy(_PanelTitle, L"ADB - Select Device");
	}
}

ADBPlugin::~ADBPlugin()
{
	{
		std::lock_guard<std::mutex> lock(g_live_instances_mtx);
		g_live_instances.erase(this);
	}
	// Explicit disconnect so the persistent adb shell child is torn down before the
	// shared_ptr's own destructor runs (easier to trace in logs, predictable order).
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

	static const wchar_t* connectedTitles[] = { L"Name", L"Size" };

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

	static const wchar_t* deviceTitles[] = { L"Serial Number", L"Device Name", L"Model", L"Port" };

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
	// F5/F6/Shift+F5/Shift+F6 — funnel to one robust handler.
	// Plain F5/F6 → cross-panel; we only consume the key if passive is another ADB
	// plugin (Far would otherwise route plugin↔plugin through host temp). For
	// ADB↔host FS we let Far's default flow (its "Move to:" dialog → GetFiles/PutFiles)
	// take over. Shift variants are always intercepted — Far has no plugin hook for
	// in-place rename / duplicate.
	const bool shift_only = (ControlState & PKF_SHIFT) && !(ControlState & (PKF_CONTROL | PKF_ALT));
	if ((Key == VK_F5 || Key == VK_F6) && NoControls(ControlState)) {
		return HandleCopyMove(/*move=*/Key == VK_F6, /*in_place=*/false) ? TRUE : FALSE;
	}
	if ((Key == VK_F5 || Key == VK_F6) && shift_only) {
		return HandleCopyMove(/*move=*/Key == VK_F6, /*in_place=*/true) ? TRUE : FALSE;
	}
	return FALSE;
}

bool ADBPlugin::HandleCopyMove(bool move, bool in_place)
{
	DBG("HandleCopyMove: move=%d in_place=%d _isConnected=%d serial=%s\n",
		move, in_place, _isConnected, _deviceSerial.c_str());
	if (!_isConnected || !_adbDevice || _deviceSerial.empty()) {
		DBG("HandleCopyMove: not connected — defer to Far default\n");
		return false;
	}

	HANDLE active = INVALID_HANDLE_VALUE;
	g_Info.Control(PANEL_ACTIVE, FCTL_GETPANELPLUGINHANDLE, 0, (LONG_PTR)(void*)&active);
	if (active != (void*)this) {
		DBG("HandleCopyMove: active mismatch (active=%p this=%p) — defer\n", (void*)active, (void*)this);
		return false;
	}

	const std::string srcDir = GetCurrentDevicePath();
	if (srcDir.empty()) {
		DBG("HandleCopyMove: empty srcDir — defer\n");
		return false;
	}

	// For cross-panel ops, only intercept if passive is another ADB plugin instance.
	// Otherwise (host FS) we defer so Far's default routing handles it.
	ADBPlugin* dst = nullptr;
	std::string dstDirDefault = srcDir;
	if (!in_place) {
		HANDLE passive = INVALID_HANDLE_VALUE;
		g_Info.Control(PANEL_PASSIVE, FCTL_GETPANELPLUGINHANDLE, 0, (LONG_PTR)(void*)&passive);
		if (!passive || passive == INVALID_HANDLE_VALUE) {
			DBG("HandleCopyMove: passive=null — defer to Far default (ADB↔FS path)\n");
			return false;
		}
		if (!IsLiveInstance(passive)) {
			DBG("HandleCopyMove: passive=%p not an ADBPlugin — defer to Far default\n", (void*)passive);
			return false;
		}
		dst = static_cast<ADBPlugin*>(passive);
		if (dst == this) {
			DBG("HandleCopyMove: passive == active — defer\n");
			return false;
		}
		if (!dst->_isConnected || !dst->_adbDevice) {
			DBG("HandleCopyMove: passive ADBPlugin not connected — defer\n");
			return false;
		}
		if (dst->_deviceSerial != _deviceSerial) {
			DBG("HandleCopyMove: different serial src=%s dst=%s — defer (Far does host roundtrip)\n",
				_deviceSerial.c_str(), dst->_deviceSerial.c_str());
			return false;
		}
		const std::string passiveDir = dst->GetCurrentDevicePath();
		if (passiveDir.empty()) {
			DBG("HandleCopyMove: empty passive dir — defer\n");
			return false;
		}
		dstDirDefault = passiveDir;
	}

	// Collect items from active panel, filtering out "." and "..".
	PanelInfo pi = {};
	if (g_Info.Control(PANEL_ACTIVE, FCTL_GETPANELINFO, 0, (LONG_PTR)(void*)&pi) == 0) {
		DBG("HandleCopyMove: FCTL_GETPANELINFO failed — defer\n");
		return false;
	}

	auto getItem = [&](int cmd, int index, PluginPanelItem& out, std::vector<char>& buf) -> bool {
		intptr_t size = g_Info.Control(PANEL_ACTIVE, cmd, index, 0);
		if (size < (intptr_t)sizeof(PluginPanelItem)) {
			return false;
		}
		buf.assign((size_t)size + 0x100, 0);
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
	};
	std::vector<SelectedItem> items;
	auto pushItem = [&](const PluginPanelItem& it) {
		if (!it.FindData.lpwszFileName) return;
		std::string name = StrWide2MB(it.FindData.lpwszFileName);
		if (name.empty() || name == "." || name == "..") return;
		items.push_back(SelectedItem{name, (it.FindData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) != 0});
	};

	if (in_place) {
		// In-place rename/duplicate operates on cursor item only — Far's standard
		// Shift+F5/F6 convention; selection is ignored.
		PluginPanelItem it = {};
		std::vector<char> buf;
		if (getItem(FCTL_GETCURRENTPANELITEM, 0, it, buf)) {
			pushItem(it);
		}
	} else if (pi.SelectedItemsNumber > 0) {
		for (int i = 0; i < pi.SelectedItemsNumber; ++i) {
			PluginPanelItem it = {};
			std::vector<char> buf;
			if (getItem(FCTL_GETSELECTEDPANELITEM, i, it, buf)) {
				pushItem(it);
			}
		}
	} else {
		PluginPanelItem it = {};
		std::vector<char> buf;
		if (getItem(FCTL_GETCURRENTPANELITEM, 0, it, buf)) {
			pushItem(it);
		}
	}

	if (items.empty()) {
		DBG("HandleCopyMove: no eligible items (cursor on '..' / empty selection)\n");
		if (in_place) {
			// Surface a clear message — Shift+F6 with cursor on ".." is a common mistake.
			ADBDialogs::Message(FMSG_MB_OK, move ? L"Rename" : L"Duplicate",
				L"Cannot operate on the parent-directory entry.");
		}
		return true;
	}

	// Cross-panel single-item with same source/destination directory: redirect to
	// the in-place flow so the user gets a "New name:" prompt instead of a path
	// dialog where the only useful action is to type a different basename.
	if (!in_place && items.size() == 1 && srcDir == dstDirDefault) {
		DBG("HandleCopyMove: cross-panel single-item same-dir → redirecting to in_place\n");
		return HandleCopyMove(move, /*in_place=*/true);
	}

	// === Confirmation dialog ===
	// We always go through AskInput directly. AskCopyMove can't be used for either
	// branch because it queries FCTL_GETPANELDIR(PANEL_PASSIVE), which for our plugin
	// returns the Far-displayed path "/<serial>/sdcard/Music" — not the raw device
	// path "/sdcard/Music" that mv on the device needs. (See GetOpenPluginInfo where
	// CurDir is built from "/" + _deviceSerial + _CurrentDir.)
	std::string entered;
	bool dialog_ok;
	{
		const wchar_t* title;
		const wchar_t* prompt;
		std::string default_value;
		std::wstring promptW;
		if (in_place) {
			title  = move ? L"Rename" : L"Duplicate";
			prompt = move ? L"New name:" : L"Copy as (new name):";
			default_value = items[0].name;
		} else {
			title = move ? L"Move on device" : L"Copy on device";
			if (items.size() == 1) {
				promptW = (move ? L"Move \"" : L"Copy \"") + StrMB2Wide(items[0].name) + L"\" to:";
			} else {
				promptW = (move ? L"Move " : L"Copy ") + std::to_wstring(items.size()) + L" items to:";
			}
			prompt = promptW.c_str();
			default_value = dstDirDefault;
		}
		entered = default_value;
		DBG("HandleCopyMove: showing AskInput title='%ls' default='%s' (in_place=%d)\n",
			title, default_value.c_str(), in_place);
		dialog_ok = ADBDialogs::AskInput(title, prompt,
			in_place ? L"ADB_InPlace" : L"ADB_CopyMove",
			entered, default_value);
	}
	if (!dialog_ok) {
		DBG("HandleCopyMove: user cancelled\n");
		return true;
	}
	// Trim trailing whitespace; preserve trailing '/' (user's "this is a dir" hint).
	while (!entered.empty() && (entered.back() == ' ' || entered.back() == '\t')) {
		entered.pop_back();
	}
	if (entered.empty()) {
		DBG("HandleCopyMove: empty input after trim\n");
		return true;
	}

	// in_place + single item + same name: silent no-op. Both Shift+F6 (rename to
	// same name) and Shift+F5 (duplicate as same name) are meaningless.
	if (in_place && items.size() == 1 && entered == items[0].name) {
		DBG("HandleCopyMove: in_place no-op (entered == original name)\n");
		return true;
	}

	// Resolve destination: relative paths anchor at active panel's dir. ".." / "subdir/foo"
	// thus go where the user expects (one level up / into a subdirectory).
	std::string dstDir;
	if (entered[0] == '/') {
		dstDir = entered;
	} else {
		dstDir = ADBUtils::JoinPath(srcDir, entered);
	}
	DBG("HandleCopyMove: resolved entered='%s' → dstDir='%s' srcDir='%s'\n",
		entered.c_str(), dstDir.c_str(), srcDir.c_str());

	// Resolve target semantic via a single shell roundtrip:
	//   dir_target=true  → items go INTO dstDir as dstDir/<name>
	//   dir_target=false → single item is renamed to dstDir verbatim
	// Pre-mkdir whenever the user indicated a directory (multi-item, or trailing slash)
	// but the path doesn't exist yet — mv would otherwise fail with ENOENT.
	const bool dst_endsWithSlash = !dstDir.empty() && dstDir.back() == '/';
	const ADBDevice::PathStat dst_stat = _adbDevice->StatPath(dstDir);
	bool dir_target;
	if (items.size() > 1) {
		dir_target = true;
		if (dst_stat.exists && !dst_stat.is_dir) {
			DBG("HandleCopyMove multi-item dst is a file, not dir: %s\n", dstDir.c_str());
			WINPORT(SetLastError)(ENOTDIR);
			return true;
		}
		if (!dst_stat.exists) {
			const int mkRc = _adbDevice->CreateDirectory(dstDir);
			if (mkRc != 0) {
				DBG("HandleCopyMove mkdir failed dst=%s err=%d\n", dstDir.c_str(), mkRc);
				WINPORT(SetLastError)(mkRc);
				return true;
			}
		}
	} else if (dst_endsWithSlash) {
		dir_target = true;
		if (!dst_stat.exists) {
			const int mkRc = _adbDevice->CreateDirectory(dstDir);
			if (mkRc != 0) {
				DBG("HandleCopyMove trailing-slash mkdir failed dst=%s err=%d\n", dstDir.c_str(), mkRc);
				WINPORT(SetLastError)(mkRc);
				return true;
			}
		}
	} else if (!dst_stat.exists) {
		dir_target = false;  // single item → rename to full path
	} else {
		dir_target = dst_stat.is_dir;
	}

	DBG("HandleCopyMove op=%s count=%zu src=%s dst=%s dir_target=%d serial=%s\n",
		move ? "move" : "copy", items.size(), srcDir.c_str(), dstDir.c_str(), dir_target, _deviceSerial.c_str());

	int okCount = 0;
	int selfOpCount = 0;
	int lastErr = 0;
	int overwriteMode = 0;  // 0=ask, 1=overwrite-all, 2=skip-all
	const bool isMultiple = items.size() > 1;
	bool aborted = false;
	// Track the last item's effective destination basename — used to position the
	// cursor on the renamed item when in_place && single (Far's standard rename UX).
	std::string lastDstBasename;
	for (size_t i = 0; i < items.size() && !aborted; ++i) {
		const auto& it = items[i];
		const std::string srcPath = ADBUtils::JoinPath(srcDir, it.name);
		const std::string dstPath = dir_target ? ADBUtils::JoinPath(dstDir, it.name) : dstDir;
		DBG("HandleCopyMove item=%s is_dir=%d srcPath=%s dstPath=%s\n",
			it.name.c_str(), it.is_dir, srcPath.c_str(), dstPath.c_str());

		// Self-op handling:
		//   • srcPath == dstPath: skip (cp/mv a file/dir onto itself is meaningless).
		//   • src is ancestor of dst (e.g. /sdcard/ucl → /sdcard/ucl/ucl):
		//       - move: skip — moving a dir under itself is genuinely impossible.
		//       - copy: shell cp would either refuse or recurse forever, so detour via
		//         a sibling temp (outside source subtree) and mv into final dst.
		const bool selfEqual = (srcPath == dstPath);
		const bool selfDescendant = (it.is_dir &&
			dstPath.size() > srcPath.size() &&
			dstPath.compare(0, srcPath.size(), srcPath) == 0 &&
			dstPath[srcPath.size()] == '/');
		if (selfEqual || (selfDescendant && move)) {
			DBG("HandleCopyMove self-op skip: src=%s dst=%s\n", srcPath.c_str(), dstPath.c_str());
			++selfOpCount;
			continue;
		}
		if (selfDescendant) {
			// Build a sibling-temp name in the parent of srcPath. cp won't recurse into
			// it because it lives outside source subtree; mv lands it at the cycle dst.
			auto slash = srcPath.find_last_of('/');
			std::string parent = (slash == 0) ? "/" : srcPath.substr(0, slash);
			std::string tempPath;
			for (int n = 0; n < 100; ++n) {
				std::string candidate = parent + "/.adbcp.tmp." +
					std::to_string((long long)getpid()) + "." + std::to_string(n);
				if (!_adbDevice->StatPath(candidate).exists) { tempPath = candidate; break; }
			}
			if (tempPath.empty()) {
				DBG("HandleCopyMove cycle: no free temp name in %s\n", parent.c_str());
				lastErr = EIO;
				break;
			}
			DBG("HandleCopyMove cycle: src=%s dst=%s temp=%s\n", srcPath.c_str(), dstPath.c_str(), tempPath.c_str());
			// CopyRemote/MoveRemote pass dst verbatim to shell cp/mv; with dst being a
			// non-existing full path, shell creates/renames it.
			int rc = _adbDevice->CopyRemote(srcPath, tempPath);
			if (rc != 0) {
				DBG("HandleCopyMove cycle copy failed err=%d\n", rc);
				lastErr = rc; break;
			}
			rc = _adbDevice->MoveRemote(tempPath, dstPath);
			if (rc != 0) {
				DBG("HandleCopyMove cycle mv-into-place failed err=%d\n", rc);
				_adbDevice->DeleteDirectory(tempPath);  // best-effort cleanup
				lastErr = rc; break;
			}
			++okCount;
			continue;
		}

		// Overwrite confirmation — same dialog as cross-host F5/F6.
		// Stat the *destination* (not source): a file→dir or dir→file overlap requires
		// the matching delete syscall, otherwise rm would silently fail or wrong-type.
		// Pre-remove on confirmed overwrite: mv/cp -a refuse to clobber a non-empty
		// directory, so without this an explicit "Overwrite" of a dir would still fail.
		// NB: between this rm and the subsequent mv, dst is gone — if mv fails for any
		// reason (rare: disconnect, EROFS, ENOSPC), the original dst content is lost.
		ADBDevice::PathStat dp = _adbDevice->StatPath(dstPath);
		if (dp.exists) {
			int action = CheckOverwrite(StrMB2Wide(dstPath), isMultiple, dp.is_dir, overwriteMode, nullptr);
			if (action == 1) continue;          // skip this one
			if (action == 2) { aborted = true; break; }  // user cancelled
			const int delRc = dp.is_dir ? _adbDevice->DeleteDirectory(dstPath) : _adbDevice->DeleteFile(dstPath);
			if (delRc != 0) {
				lastErr = delRc;
				DBG("HandleCopyMove pre-overwrite delete failed item=%s err=%d\n", it.name.c_str(), delRc);
				break;
			}
		}

		const int rc = move ? _adbDevice->MoveRemote(srcPath, dstPath)
		                    : _adbDevice->CopyRemote(srcPath, dstPath);
		if (rc == 0) {
			++okCount;
			lastDstBasename = dstPath.substr(dstPath.find_last_of('/') + 1);
		} else {
			lastErr = rc;
			DBG("HandleCopyMove failed op=%s item=%s err=%d\n",
				move ? "move" : "copy", it.name.c_str(), rc);
			// On hard errors (disconnect, permission), abort to avoid cascading failures.
			// EEXIST shouldn't reach here (CheckOverwrite handled it); ENOENT means src
			// vanished — also bail out since something is racing us.
			break;
		}
	}

	if (okCount > 0) {
		g_Info.Control(PANEL_ACTIVE, FCTL_CLEARSELECTION, 0, 0);
		g_Info.Control(PANEL_ACTIVE, FCTL_UPDATEPANEL, 0, 0);
		// in_place + single item: position the cursor on the renamed/copied item.
		// Without this the cursor stays on the old screen row, which after sort lands
		// on an unrelated item.
		bool positioned = false;
		if (in_place && items.size() == 1 && !lastDstBasename.empty()) {
			PanelInfo refreshed = {};
			if (g_Info.Control(PANEL_ACTIVE, FCTL_GETPANELINFO, 0, (LONG_PTR)(void*)&refreshed) != 0) {
				const std::wstring targetW = StrMB2Wide(lastDstBasename);
				PanelRedrawInfo ri = {};
				ri.CurrentItem = -1;
				for (int i = 0; i < refreshed.ItemsNumber; ++i) {
					intptr_t sz = g_Info.Control(PANEL_ACTIVE, FCTL_GETPANELITEM, i, 0);
					if (sz < (intptr_t)sizeof(PluginPanelItem)) continue;
					std::vector<char> ibuf((size_t)sz + 0x100, 0);
					auto* pit = (PluginPanelItem*)ibuf.data();
					if (!g_Info.Control(PANEL_ACTIVE, FCTL_GETPANELITEM, i, (LONG_PTR)(void*)pit)) continue;
					if (pit->FindData.lpwszFileName && targetW == pit->FindData.lpwszFileName) {
						ri.CurrentItem = i;
						ri.TopPanelItem = i;
						break;
					}
				}
				if (ri.CurrentItem >= 0) {
					g_Info.Control(PANEL_ACTIVE, FCTL_REDRAWPANEL, 0, (LONG_PTR)(void*)&ri);
					positioned = true;
				}
			}
		}
		if (!positioned) {
			g_Info.Control(PANEL_ACTIVE, FCTL_REDRAWPANEL, 0, 0);
		}
		// Refresh passive panel — always, so a second ADB panel on the same device
		// reflects in-place renames/duplicates without requiring a manual Ctrl+R.
		g_Info.Control(PANEL_PASSIVE, FCTL_UPDATEPANEL, 0, 0);
		g_Info.Control(PANEL_PASSIVE, FCTL_REDRAWPANEL, 0, 0);
		// Surface partial-failure errors even when some items succeeded.
		if (lastErr != 0) {
			WINPORT(SetLastError)(lastErr);
		}
		return true;
	}

	if (lastErr != 0) {
		WINPORT(SetLastError)(lastErr);
	} else if (selfOpCount > 0 && okCount == 0) {
		// Skipped because src equals dst (always impossible) or, for move, dst is a
		// descendant of src (a directory cannot move under itself). Copy of a folder
		// into a descendant is actually handled via a sibling temp above, so reaching
		// here means an unrecoverable self-op.
		ADBDialogs::Message(FMSG_MB_OK, move ? L"Move" : L"Copy",
			move ? L"Cannot move a folder into itself or one of its sub-folders."
			     : L"Source and destination are the same.");
	}
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

	// ".." as first entry — with empty CurDir, FAR routes selection through PopPlugin
	// (filelist.cpp:2498), closing the plugin and returning to the host panel.
	std::vector<PluginPanelItem> items;
	{
		PluginPanelItem parent{};
		parent.FindData.lpwszFileName = ADBDevice::AllocateItemString("..");
		parent.FindData.dwFileAttributes = FILE_ATTRIBUTE_DIRECTORY;
		items.push_back(parent);
	}

	if (deviceInfos.empty()) {
		DBG("No ADB devices found\n");
		wcscpy(_PanelTitle, L"ADB: No devices found");

		PluginPanelItem nf{};
		nf.FindData.lpwszFileName = ADBDevice::AllocateItemString("<Not found>");
		nf.FindData.dwFileAttributes = FILE_ATTRIBUTE_NORMAL;

		wchar_t **customData = new wchar_t*[3];
		customData[0] = ADBDevice::AllocateItemString("<Connect device>");  // C0: Device Name
		customData[1] = ADBDevice::AllocateItemString("");                  // C1: Model
		customData[2] = ADBDevice::AllocateItemString("");                  // C2: Port
		nf.CustomColumnData = customData;
		nf.CustomColumnNumber = 3;
		items.push_back(nf);
	} else {
		wcscpy(_PanelTitle, deviceInfos.size() > 1 ? L"ADB - Select Device" : L"ADB");

		for (const auto& info : deviceInfos) {
			PluginPanelItem device{};
			device.FindData.lpwszFileName = ADBDevice::AllocateItemString(info.serial);
			device.FindData.dwFileAttributes = FILE_ATTRIBUTE_DIRECTORY; // Make it look like a directory

			// Custom columns: C0=Serial (filename), C1=Device Name, C2=Model, C3=Port.
			wchar_t **customData = new wchar_t*[3];
			customData[0] = ADBDevice::AllocateItemString(info.name);  // C1: Device Name
			customData[1] = ADBDevice::AllocateItemString(info.model); // C2: Model
			customData[2] = ADBDevice::AllocateItemString(info.usb);   // C3: Port

			device.CustomColumnData = customData;
			device.CustomColumnNumber = 3;

			items.push_back(device);
		}
	}

	*pItemsNumber = (int)items.size();
	*pPanelItem = new PluginPanelItem[items.size()];

	for (size_t i = 0; i < items.size(); i++) {
		(*pPanelItem)[i] = items[i];
	}

	return (int)items.size();
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
	// Get the currently selected device from the panel
	std::string deviceSerial = GetCurrentPanelItemDeviceName();
	if (deviceSerial.empty() || deviceSerial[0] == '<' || deviceSerial == "..") {
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

int ADBPlugin::GetHighlightedDeviceIndex()
{
	// TODO: return actually-highlighted item; currently always 0 (first device).
	return 0;
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
	wcscpy(_PanelTitle, L"ADB Plugin");

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
	
	// Clear the memory first
	memset(item, 0, static_cast<size_t>(size));
	
	if (!g_Info.Control(PANEL_ACTIVE, FCTL_GETSELECTEDPANELITEM, 0, (LONG_PTR)(void *)item)) {
		free(item);
		return "";
	}
	
	if (!item->FindData.lpwszFileName) {
		free(item);
		return "";
	}
	
	// Get serial number from filename (since we set it there)
	std::string deviceSerial = StrWide2MB(item->FindData.lpwszFileName);
	
	free(item);
	return deviceSerial;
}

std::string ADBPlugin::GetFallbackDeviceName()
{
	if (!_adbDevice) {
		return "Unknown Device";
	}
	
	std::string output = ADBShell::adbExec("devices -l");
	if (output.empty()) {
		return "No Device";
	}
	
	std::vector<std::string> deviceLines;
	std::istringstream outputStream(output);
	std::string line;
	
	if (std::getline(outputStream, line)) {
		while (std::getline(outputStream, line)) {
			if (!line.empty() && line[line.length()-1] == '\r') {
				line.erase(line.length()-1);
			}
			
			if (line.find("device") != std::string::npos) {
				deviceLines.push_back(line);
			}
		}
	}
	
	if (deviceLines.empty()) {
		return "No Device";
	}
	
	int selectedIndex = GetHighlightedDeviceIndex();
	std::string deviceLine;
	
	if (selectedIndex >= 0 && selectedIndex < (int)deviceLines.size()) {
		deviceLine = deviceLines[selectedIndex];
	} else {
		deviceLine = deviceLines[0];
	}
	
	std::vector<std::string> tokens;
	std::string token;
	std::istringstream tokenStream(deviceLine);
	while (tokenStream >> token) {
		tokens.push_back(token);
	}
	
	if (tokens.size() >= 3) {
		for (size_t i = 2; i < tokens.size(); i++) {
			if (tokens[i].find("product:") == 0) {
				std::string product = tokens[i].substr(8);
				return product;
			}
			if (tokens[i].find("model:") == 0) {
				std::string model = tokens[i].substr(6);
				return model;
			}
		}
	}
	
	if (!tokens.empty()) {
		return tokens[0];
	}
	
	return "Unknown Device";
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

std::string ADBPlugin::GetFirstAvailableDevice()
{
	auto devices = EnumerateDevices();
	return devices.empty() ? "" : devices[0].serial;
}

int ADBPlugin::GetAvailableDeviceCount()
{
	return (int)EnumerateDevices().size();
}


PluginStartupInfo *ADBPlugin::GetInfo()
{
	return &g_Info;
}

// Helper for recursive local directory scanning (DRY)
static void ScanLocalDirectory(const std::string& path, uint64_t& totalSize, uint64_t& totalFiles) {
    DIR* dir = opendir(path.c_str());
    if (!dir) return;

    struct dirent* ent;
    while ((ent = readdir(dir)) != nullptr) {
        if (strcmp(ent->d_name, ".") == 0 || strcmp(ent->d_name, "..") == 0) continue;
        
        std::string subPath = ADBUtils::JoinPath(path, ent->d_name);
        struct stat st;
        if (stat(subPath.c_str(), &st) == 0) {
            if (S_ISDIR(st.st_mode)) {
                ScanLocalDirectory(subPath, totalSize, totalFiles);
            } else {
                totalSize += st.st_size;
                totalFiles++;
            }
        }
    }
    closedir(dir);
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
	if (!_isConnected || !_adbDevice || !Dir || wcslen(Dir) == 0) {
		return FALSE;
	}

	std::string target_dir = StrWide2MB(Dir);

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

	// Far doesn't show its own "Move/Copy to:" dialog when source is a plugin panel
	// and destination is the host FS — see filelist.cpp ProcessCopyKeys: with passive
	// as a non-plugin panel, Far skips ShellCopy and calls PluginGetFiles directly.
	// We have to show the destination prompt ourselves; otherwise F5/F6 ADB→host runs
	// instantly without confirmation. (PutFiles is the mirror direction — Far DOES
	// show its dialog there via ShellCopy(this, Move, ..., ToPlugin), so no prompt
	// from us in PutFiles.)
	if (!(OpMode & (OPM_SILENT | OPM_VIEW | OPM_FIND))) {
		std::string firstName = (ItemsNumber > 0) ? StrWide2MB(PanelItem[0].FindData.lpwszFileName) : "";
		if (!ADBDialogs::AskCopyMove(Move != 0, /*is_upload=*/false, destPath, firstName, ItemsNumber)) {
			return -1;  // user cancelled
		}
		if (DestPath) {
			static std::wstring updatedDest;
			updatedDest = StrMB2Wide(destPath);
			*DestPath = updatedDest.c_str();
		}
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
			return (result == 0) ? TRUE : FALSE;
		}
		DBG("No items, returning FALSE\n");
		return FALSE;
	}

	uint64_t totalBytes = 0;
	uint64_t totalFiles = 0;
	std::map<std::string, uint64_t> dirSizes;

	// Pre-scan to get total size/count (including directory contents).
	// Wrap: _adbDevice->GetDirectoryInfo may throw on device disconnect; letting an
	// exception escape into far2l core is a hard crash.
	try {
		for (int i = 0; i < ItemsNumber; i++) {
			if (PanelItem[i].FindData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) {
				std::string fileName = StrWide2MB(PanelItem[i].FindData.lpwszFileName);
				std::string devicePath = ADBUtils::JoinPath(GetCurrentDevicePath(), fileName);
				auto dirInfo = _adbDevice->GetDirectoryInfo(devicePath);
				totalBytes += dirInfo.total_size;
				totalFiles += dirInfo.file_count;
				dirSizes[devicePath] = dirInfo.total_size;
			} else {
				totalBytes += PanelItem[i].FindData.nFileSize;
				totalFiles++;
			}
		}
	} catch (const std::exception& ex) {
		DBG("GetFiles pre-scan exception: %s\n", ex.what());
		WINPORT(SetLastError)(EIO);
		return FALSE;
	}

	return RunTransfer(PanelItem, ItemsNumber, false, Move != 0,
	                   destPath, GetCurrentDevicePath(), dirSizes,
	                   totalBytes, totalFiles, OpMode);
}

int ADBPlugin::PutFiles(PluginPanelItem *PanelItem, int ItemsNumber, int Move, const wchar_t *SrcPath, int OpMode) {
	DBG("ItemsNumber=%d, Move=%d, OpMode=0x%x\n", ItemsNumber, Move, OpMode);

	if (ItemsNumber <= 0 || !_isConnected || !_adbDevice || !PanelItem || !SrcPath) {
		return FALSE;
	}

	// Far has already shown its "Copy/Move to plugin" dialog and called SetDirectory
	// for any user retargeting before invoking PutFiles. Items go INTO our plugin's
	// current panel dir — no second prompt needed.
	std::string srcPath = StrWide2MB(SrcPath);

	uint64_t totalBytes = 0;
	uint64_t totalFiles = 0;
	std::map<std::string, uint64_t> dirSizes;

	// Pre-scan local filesystem: ScanLocalDirectory doesn't throw, but SetDirectory above may.
	try {
		for (int i = 0; i < ItemsNumber; i++) {
			std::string fileName = StrWide2MB(PanelItem[i].FindData.lpwszFileName);
			std::string localPath = ADBUtils::JoinPath(srcPath, fileName);

			if (PanelItem[i].FindData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) {
				uint64_t dirSize = 0;
				uint64_t dirFiles = 0;
				ScanLocalDirectory(localPath, dirSize, dirFiles);
				totalBytes += dirSize;
				totalFiles += dirFiles;
				dirSizes[localPath] = dirSize;
			} else {
				totalBytes += PanelItem[i].FindData.nFileSize;
				totalFiles++;
			}
		}
	} catch (const std::exception& ex) {
		DBG("PutFiles pre-scan exception: %s\n", ex.what());
		WINPORT(SetLastError)(EIO);
		return FALSE;
	}

	return RunTransfer(PanelItem, ItemsNumber, true, Move != 0,
	                   srcPath, GetCurrentDevicePath(), dirSizes,
	                   totalBytes, totalFiles, OpMode);
}

int ADBPlugin::RunTransfer(PluginPanelItem *items, int itemsCount, bool is_upload, bool move,
                           const std::string& localDir, const std::string& deviceDir,
                           const std::map<std::string, uint64_t>& dirSizes,
                           uint64_t totalBytes, uint64_t totalFiles, int OpMode)
{
	int successCount = 0;
	int lastErrorCode = 0;
	auto adb = _adbDevice;
	// Copy dirSizes by value for safe lambda capture
	std::map<std::string, uint64_t> dirSizesCopy = dirSizes;

	if (OpMode & OPM_SILENT) {
		// Wrap: ADBDevice transfer methods may throw on device disconnect.
		try {
			for (int i = 0; i < itemsCount; i++) {
				std::string fileName = StrWide2MB(items[i].FindData.lpwszFileName);
				std::string localPath = ADBUtils::JoinPath(localDir, fileName);
				std::string devicePath = ADBUtils::JoinPath(deviceDir, fileName);
				bool isDir = (items[i].FindData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) != 0;

				// Silent-mode overwrite is implicit; pre-remove dst so push/pull doesn't
				// nest the source into an existing dir-with-same-name (mirror of the
				// progress-UI overwrite path below).
				if (is_upload) {
					ADBDevice::PathStat dp = adb->StatPath(devicePath);
					if (dp.exists) {
						int delRc = dp.is_dir ? adb->DeleteDirectory(devicePath) : adb->DeleteFile(devicePath);
						if (delRc != 0) { lastErrorCode = delRc; continue; }
					}
				} else {
					struct stat st;
					if (lstat(localPath.c_str(), &st) == 0) {
						bool dstIsDir = S_ISDIR(st.st_mode);
						int delRc = dstIsDir ? RemoveLocalPathRecursively(localPath)
						                     : (unlink(localPath.c_str()) == 0 ? 0 : (errno ? errno : EIO));
						if (delRc != 0) { lastErrorCode = delRc; continue; }
					}
				}

				int result;
				if (is_upload) {
					result = isDir ? adb->PushDirectory(localPath, devicePath)
					               : adb->PushFile(localPath, devicePath);
				} else {
					result = isDir ? adb->PullDirectory(devicePath, localPath)
					               : adb->PullFile(devicePath, localPath);
				}

				if (result == 0) {
					if (move) {
						int delRc;
						if (is_upload) {
							delRc = isDir ? RemoveLocalPathRecursively(localPath)
							              : (unlink(localPath.c_str()) == 0 ? 0 : (errno ? errno : EIO));
						} else {
							delRc = isDir ? adb->DeleteDirectory(devicePath)
							              : adb->DeleteFile(devicePath);
						}
						if (delRc != 0) {
							DBG("RunTransfer (silent) move: source delete failed item=%s err=%d\n", fileName.c_str(), delRc);
							lastErrorCode = delRc;
						}
					}
					successCount++;
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
			? (is_upload ? L"Move to device" : L"Move from device")
			: (is_upload ? L"Copy to device" : L"Copy from device");
		ProgressOperation op(title);
		op.GetState().file_total = totalBytes;
		op.GetState().all_total = totalBytes;
		op.GetState().count_total = totalFiles;
		op.GetState().source_path = is_upload ? StrMB2Wide(localDir) : StrMB2Wide(deviceDir);
		op.GetState().dest_path = is_upload ? StrMB2Wide(deviceDir) : StrMB2Wide(localDir);

		int overwriteMode = 0;
		const bool isMultiple = itemsCount > 1;

		op.Run([&](ProgressState& state) {
			uint64_t processedBytes = 0;
			uint64_t completedCount = 0;
			if (!adb || !adb->IsConnected()) return;

			// Worker-thread exception guard: transfer methods may throw on disconnect;
			// the lambda runs in a separate std::thread and an unhandled exception
			// there would call std::terminate and take the whole plugin/far2l down.
			try {
			for (int i = 0; i < itemsCount && !state.ShouldAbort(); i++) {
				std::string fileName = StrWide2MB(items[i].FindData.lpwszFileName);
				std::string localPath = ADBUtils::JoinPath(localDir, fileName);
				std::string devicePath = ADBUtils::JoinPath(deviceDir, fileName);
				bool isDir = (items[i].FindData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) != 0;
				const uint64_t itemSize = isDir ? 0 : items[i].FindData.nFileSize;

				// Check if destination exists and handle overwrite.
				// On confirmed Overwrite: pre-remove dst BEFORE the transfer so that
				// adb push/pull doesn't nest the source into an existing dir-with-same-name
				// (POSIX cp / adb push default is "put src INSIDE existing dst dir" → that
				// would turn an Overwrite confirmation into "ucl ended up at dst/ucl/ucl").
				if (is_upload) {
					ADBDevice::PathStat dp = adb->StatPath(devicePath);
					if (dp.exists) {
						int action = CheckOverwrite(StrMB2Wide(devicePath), isMultiple, dp.is_dir, overwriteMode, &state);
						if (action == 1 || action == 2) continue;
						int delRc = dp.is_dir ? adb->DeleteDirectory(devicePath) : adb->DeleteFile(devicePath);
						if (delRc != 0) {
							DBG("RunTransfer pre-overwrite delete (device) failed item=%s err=%d\n", fileName.c_str(), delRc);
							lastErrorCode = delRc;
							continue;
						}
					}
				} else {
					struct stat st;
					if (lstat(localPath.c_str(), &st) == 0) {
						bool dstIsDir = S_ISDIR(st.st_mode);
						int action = CheckOverwrite(StrMB2Wide(localPath), isMultiple, dstIsDir, overwriteMode, &state);
						if (action == 1 || action == 2) continue;
						int delRc = dstIsDir ? RemoveLocalPathRecursively(localPath)
						                     : (unlink(localPath.c_str()) == 0 ? 0 : (errno ? errno : EIO));
						if (delRc != 0) {
							DBG("RunTransfer pre-overwrite delete (host) failed item=%s err=%d\n", fileName.c_str(), delRc);
							lastErrorCode = delRc;
							continue;
						}
					}
				}

				if (state.ShouldAbort()) continue;

				// Look up dir size using the key that matches how it was stored
				std::string sizeKey = is_upload ? localPath : devicePath;
				auto sizeIt = dirSizesCopy.find(sizeKey);
				uint64_t dirTotalSize = isDir ? (sizeIt != dirSizesCopy.end() ? sizeIt->second : 0) : itemSize;

				{
					std::lock_guard<std::mutex> lock(state.mtx_strings);
					state.current_file = StrMB2Wide(fileName);
				}
				state.file_total = dirTotalSize;
				state.file_complete = 0;
				state.is_directory = isDir;
				state.count_complete = i;

				const uint64_t bytesBefore = processedBytes;
				int lastReportedPercent = -1;
				auto progressCallback = [&](int percent) {
					state.file_complete = percent;
					if (dirTotalSize > 0) {
						state.all_complete = bytesBefore + (dirTotalSize * percent) / 100;
					}
					if (percent != lastReportedPercent) {
						lastReportedPercent = percent;
						INPUT_RECORD ir = {};
						ir.EventType = NOOP_EVENT;
						DWORD dw = 0;
						WINPORT(WriteConsoleInput)(0, &ir, 1, &dw);
					}
				};

				auto abortChecker = [&]() { return state.ShouldAbort(); };

				int result;
				if (is_upload) {
					result = isDir
						? adb->PushDirectory(localPath, devicePath, progressCallback, abortChecker)
						: adb->PushFile(localPath, devicePath, progressCallback, abortChecker);
				} else {
					result = isDir
						? adb->PullDirectory(devicePath, localPath, progressCallback, abortChecker)
						: adb->PullFile(devicePath, localPath, progressCallback, abortChecker);
				}

				if (result == 0 && !state.ShouldAbort()) {
					if (move) {
						int delRc;
						if (is_upload) {
							delRc = isDir ? RemoveLocalPathRecursively(localPath)
							              : (unlink(localPath.c_str()) == 0 ? 0 : (errno ? errno : EIO));
						} else {
							delRc = isDir ? adb->DeleteDirectory(devicePath)
							              : adb->DeleteFile(devicePath);
						}
						if (delRc != 0) {
							DBG("RunTransfer move: source delete failed item=%s err=%d\n", fileName.c_str(), delRc);
							lastErrorCode = delRc;
						}
					}
					successCount++;
					processedBytes += dirTotalSize;
					completedCount++;
					state.file_complete = 100;
					state.all_complete = processedBytes;
					state.count_complete = completedCount;
				} else if (result != 0) {
					lastErrorCode = result;
				}
			}
			} catch (const std::exception& ex) {
				DBG("RunTransfer worker exception: %s\n", ex.what());
				lastErrorCode = EIO;
				state.SetAborting();
			} catch (...) {
				DBG("RunTransfer worker: unknown exception\n");
				lastErrorCode = EIO;
				state.SetAborting();
			}
		});
	}

	// Propagate lastError even on partial success: if move-after-transfer delete failed
	// for some items, the user must see the error (the panel selection-clear on TRUE
	// would otherwise hide the orphan source).
	if (lastErrorCode != 0) {
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

	// Filter out the parent-directory entry up front: F8 on ".." (or with ".." in
	// a multi-selection) should never produce a "Do you wish to delete?" prompt.
	std::vector<int> realIdx;
	realIdx.reserve(ItemsNumber);
	for (int i = 0; i < ItemsNumber; i++) {
		const wchar_t* nm = PanelItem[i].FindData.lpwszFileName;
		if (nm && (wcscmp(nm, L"..") == 0 || wcscmp(nm, L".") == 0)) continue;
		realIdx.push_back(i);
	}
	if (realIdx.empty()) {
		return TRUE;  // nothing to delete; consume the key without prompting
	}

	if (!(OpMode & OPM_SILENT)) {
		int fileCount = 0;
		int folderCount = 0;
		for (int idx : realIdx) {
			if (PanelItem[idx].FindData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
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
				L"Delete folder",
				L"Do you wish to delete the folder",
				std::wstring(PanelItem[0].FindData.lpwszFileName));
		} else if (ItemsNumber == 1) {
			result = ADBDialogs::Message(flags,
				L"Delete",
				L"Do you wish to delete the file",
				std::wstring(PanelItem[0].FindData.lpwszFileName));
		} else if (fileCount > 0 && folderCount > 0) {
			result = ADBDialogs::Message(flags,
				L"Delete items",
				L"Do you wish to delete",
				std::to_wstring(folderCount) + L" folders and " + std::to_wstring(fileCount) + L" files");
		} else if (folderCount > 0) {
			result = ADBDialogs::Message(flags,
				L"Delete folders",
				L"Do you wish to delete",
				std::to_wstring(folderCount) + L" folders");
		} else {
			result = ADBDialogs::Message(flags,
				L"Delete files",
				L"Do you wish to delete",
				std::to_wstring(fileCount) + L" files");
		}

		if (result != 0) {
			return -1;
		}
	}
	
	int successCount = 0;
	int lastErrorCode = 0;

	const int realCount = (int)realIdx.size();
	std::string deviceDir = GetCurrentDevicePath();
	auto adb = _adbDevice;

	if (OpMode & OPM_SILENT) {
		// Silent mode - no progress dialog
		try {
			for (int k = 0; k < realCount; k++) {
				const int i = realIdx[k];
				std::string fileName = StrWide2MB(PanelItem[i].FindData.lpwszFileName);
				std::string devicePath = ADBUtils::JoinPath(deviceDir, fileName);

				int result = 0;
				if (PanelItem[i].FindData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) {
					result = adb->DeleteDirectory(devicePath);
				} else {
					result = adb->DeleteFile(devicePath);
				}

				if (result == 0) {
					successCount++;
				} else {
					lastErrorCode = result;
				}
			}
		} catch (const std::exception& ex) {
			DBG("DeleteFiles (silent) exception: %s\n", ex.what());
			lastErrorCode = EIO;
		}
	} else {
		// Show progress dialog with worker thread
		ProgressOperation op(L"Delete from device");
		op.GetState().count_total = realCount;

	op.Run([&](ProgressState& state) {
			// Worker-thread exception guard (std::terminate on escape).
			try {
			for (int k = 0; k < realCount && !state.ShouldAbort(); k++) {
				const int i = realIdx[k];
				std::string fileName = StrWide2MB(PanelItem[i].FindData.lpwszFileName);
				std::string devicePath = ADBUtils::JoinPath(deviceDir, fileName);

				// Update state with current file
				{
					std::lock_guard<std::mutex> lock(state.mtx_strings);
					state.current_file = StrMB2Wide(fileName);
				}
				state.count_complete = k;

				int result = 0;
				if (PanelItem[i].FindData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) {
					result = adb->DeleteDirectory(devicePath);
				} else {
					result = adb->DeleteFile(devicePath);
				}

				if (result == 0 && !state.ShouldAbort()) {
					successCount++;
				} else if (result != 0) {
					lastErrorCode = result;
				}
			}

			// Final update
			state.count_complete = realCount;
			} catch (const std::exception& ex) {
				DBG("DeleteFiles worker exception: %s\n", ex.what());
				lastErrorCode = EIO;
				state.SetAborting();
			} catch (...) {
				lastErrorCode = EIO;
				state.SetAborting();
			}
		});
	}

	if (successCount == 0) {
		WINPORT(SetLastError)(lastErrorCode);
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
	
	if (!(OpMode & OPM_SILENT)) {
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
	wcscpy(_PanelTitle, panel_title.c_str());
}
