#include "ADBPlugin.h"
#include "ADBDevice.h"
#include "ADBShell.h"
#include "ADBDialogs.h"
#include "ADBLog.h"
#include <sstream>
#include <string>
#include <vector>
#include <map>
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
static int CheckOverwrite(const std::wstring& destPath, bool isMultiple, bool isDir,
                          int& overwriteMode, ProgressState& state) {
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
			state.SetAborting();
			return 2;
	}
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

static bool NoControls(unsigned int control_state) {
	return (control_state & (PKF_CONTROL | PKF_ALT | PKF_SHIFT)) == 0;
}

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

// "foo" → currentDir/foo, "../foo" → parent/foo, "/abs/foo" → /abs/foo.
static std::string ResolveAdbDestination(const std::string& currentDir,
                                         const std::string& userInput) {
	if (userInput.empty()) return userInput;
	std::string joined;
	if (userInput.front() == '/') {
		joined = userInput;
	} else {
		joined = currentDir;
		if (joined.empty() || joined.back() != '/') joined += '/';
		joined += userInput;
	}
	return NormalizePosixPath(joined);
}

// Resolver supporting folder-only inputs (".."/"./sub/"/trailing slash/
// existing-dir match); folder-only joins src_name as basename.
static std::string ResolveAdbDestinationFor(const std::string& currentDir,
                                            const std::string& userInput,
                                            const std::string& src_name,
                                            std::shared_ptr<ADBDevice> dev) {
	if (userInput.empty()) return userInput;
	const bool trailing_slash = (userInput.back() == '/');
	std::string raw = trailing_slash ? userInput.substr(0, userInput.size() - 1) : userInput;
	std::string resolved = ResolveAdbDestination(currentDir, raw);
	// Folder-only: trailing slash, or "..", or "." with no other content.
	if (trailing_slash || raw == ".." || raw == ".") {
		if (!src_name.empty()) {
			if (resolved.empty() || resolved.back() != '/') resolved += '/';
			resolved += src_name;
		}
		return resolved;
	}
	// If the user's input resolves to an EXISTING directory, treat as
	// "into that dir" with basename from src_name (mirrors mtp behavior).
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


ADBPlugin::ADBPlugin(const wchar_t *path, bool path_is_standalone_config, int OpMode)
	: _allow_remember_location_dir(false)
	, _isConnected(false)
	, _deviceSerial("")
	, _CurrentDir("/")
{
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
	if ((Key == VK_F5 || Key == VK_F6) && NoControls(ControlState)) {
		return CrossPanelCopyMoveSameDevice(Key == VK_F6) ? TRUE : FALSE;
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

	HANDLE passive = INVALID_HANDLE_VALUE;
	g_Info.Control(PANEL_PASSIVE, FCTL_GETPANELPLUGINHANDLE, 0, (LONG_PTR)(void*)&passive);
	if (passive == INVALID_HANDLE_VALUE || !passive) {
		return false;
	}

	// memcpy (not deref) to read _signature — avoids UB if passive isn't our plugin at all.
	uint64_t sig = 0;
	memcpy(&sig, passive, sizeof(sig));
	if (sig != PLUGIN_SIGNATURE) {
		return false;
	}
	auto* dst = (ADBPlugin*)passive;
	if (!dst || dst == this || !dst->_isConnected || !dst->_adbDevice) {
		return false;
	}
	if (dst->_deviceSerial != _deviceSerial) {
		DBG("CrossPanelCopyMoveSameDevice skip: different serial src=%s dst=%s\n",
			_deviceSerial.c_str(), dst->_deviceSerial.c_str());
		return false;
	}

	std::string srcDir = GetCurrentDevicePath();
	std::string dstDir = dst->GetCurrentDevicePath();
	if (srcDir.empty() || dstDir.empty()) {
		return false;
	}

	PanelInfo pi = {};
	if (g_Info.Control(PANEL_ACTIVE, FCTL_GETPANELINFO, 0, (LONG_PTR)(void*)&pi) == 0) {
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

	// Confirmation prompt; user can edit dst path ("..", "./sub/", abs).
	{
		const wchar_t* title = move ? L"Move" : L"Copy";
		std::wstring prompt;
		if (items.size() == 1) {
			prompt = (move ? std::wstring(L"Move ") : std::wstring(L"Copy "))
			       + StrMB2Wide(items[0].name) + L" to:";
		} else {
			prompt = (move ? std::wstring(L"Move ") : std::wstring(L"Copy "))
			       + std::to_wstring(items.size()) + L" items to:";
		}
		const std::string prefill = BuildPanelPathPrefix(dstDir);
		std::string user_path;
		if (!ADBDialogs::AskInput(title, prompt.c_str(), L"ADB_CopyMove",
		                           user_path, prefill)) {
			return true;  // user cancelled — claim keystroke
		}
		if (user_path.empty()) return true;
		// Resolve relative to the DESTINATION panel.
		std::string resolved = ResolveAdbDestinationFor(dstDir, user_path, "", dst->_adbDevice);
		if (!resolved.empty() && resolved.back() != '/') resolved += '/';
		dstDir = resolved;
	}

	DBG("CrossPanelCopyMoveSameDevice op=%s count=%zu src=%s dst=%s serial=%s\n",
		move ? "move" : "copy", items.size(), srcDir.c_str(), dstDir.c_str(), _deviceSerial.c_str());

	// Per-item collision check + Skip/Overwrite/All/Cancel.
	bool overwrite_all = false, skip_all = false;
	std::vector<bool> skip(items.size(), false);
	std::vector<bool> overwrite(items.size(), false);
	for (size_t i = 0; i < items.size(); ++i) {
		const auto& it = items[i];
		const std::string dstPath = ADBUtils::JoinPath(dstDir, it.name);
		if (!dst->_adbDevice->FileExists(dstPath)) continue;
		if (skip_all)      { skip[i] = true; continue; }
		if (overwrite_all) { overwrite[i] = true; continue; }
		OverwriteDialog dlg(StrMB2Wide(dstPath), items.size() > 1, it.is_dir);
		switch (dlg.Ask()) {
			case OverwriteDialog::OVERWRITE:     overwrite[i] = true; break;
			case OverwriteDialog::SKIP:          skip[i] = true; break;
			case OverwriteDialog::OVERWRITE_ALL: overwrite_all = true; overwrite[i] = true; break;
			case OverwriteDialog::SKIP_ALL:      skip_all = true; skip[i] = true; break;
			case OverwriteDialog::CANCEL:
			default:
				return true;
		}
	}

	int okCount = 0;
	int lastErr = 0;
	size_t failedIndex = items.size();
	for (size_t i = 0; i < items.size(); ++i) {
		if (skip[i]) continue;
		const auto& it = items[i];
		const std::string srcPath = ADBUtils::JoinPath(srcDir, it.name);
		const std::string dstPath = ADBUtils::JoinPath(dstDir, it.name);
		// Aside-rename atomic-ish overwrite: rename existing dest aside,
		// run op, drop aside on success / restore on failure.
		std::string aside;
		bool have_aside = false;
		if (overwrite[i]) {
			aside = AsideName(dstPath);
			if (dst->_adbDevice->MoveRemoteAs(dstPath, aside) == 0) {
				have_aside = true;
			} else {
				if (it.is_dir) dst->_adbDevice->DeleteDirectory(dstPath);
				else           dst->_adbDevice->DeleteFile(dstPath);
			}
		}
		const int rc = move ? _adbDevice->MoveRemote(srcPath, dstDir)
		                    : _adbDevice->CopyRemote(srcPath, dstDir);
		if (rc == 0) {
			++okCount;
			if (have_aside) {
				if (it.is_dir) dst->_adbDevice->DeleteDirectory(aside);
				else           dst->_adbDevice->DeleteFile(aside);
			}
		} else {
			if (have_aside) dst->_adbDevice->MoveRemoteAs(aside, dstPath);
			lastErr = rc;
			DBG("CrossPanelCopyMoveSameDevice failed op=%s item=%s err=%d\n",
				move ? "move" : "copy", it.name.c_str(), rc);
			failedIndex = i;
			break;
		}
	}

	if (failedIndex < items.size()) {
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
		DBG("CrossPanelCopyMoveSameDevice fallback via host tmp=%s from_index=%zu\n",
			tmpRoot.c_str(), failedIndex);

		// Pre-scan total bytes for the progress bar. File sizes come
		// from the panel's FindData; dir sizes are scanned via shell.
		uint64_t total_bytes = 0;
		for (size_t i = failedIndex; i < items.size(); ++i) {
			if (skip[i]) continue;
			const auto& it = items[i];
			if (it.is_dir) {
				total_bytes += _adbDevice->GetDirectoryInfo(ADBUtils::JoinPath(srcDir, it.name)).total_size;
			} else {
				total_bytes += it.size;
			}
		}

		std::wstring popTitle = move ? L"Move on device" : L"Copy on device";
		size_t fb_count = 0;
		bool fb_any_dir = false;
		for (size_t i = failedIndex; i < items.size(); ++i) {
			if (skip[i]) continue;
			++fb_count;
			if (items[i].is_dir) fb_any_dir = true;
		}
		const bool is_multi = fb_count > 1 || fb_any_dir;
		ProgressOperation pop(popTitle, is_multi);
		pop.GetState().count_total = fb_count;
		pop.GetState().all_total = total_bytes;
		pop.Run([&](ProgressState& state) {
			uint64_t bytes_done = 0;
			int last_percent = -1;
			for (size_t i = failedIndex; i < items.size(); ++i) {
				if (state.ShouldAbort()) break;
				if (skip[i]) continue;
				const auto& it = items[i];
				const std::string srcPath = ADBUtils::JoinPath(srcDir, it.name);
				const std::string tmpPath = ADBUtils::JoinPath(tmpRoot, it.name);
				const std::string dstPath = ADBUtils::JoinPath(dstDir, it.name);

				{
					std::lock_guard<std::mutex> lk(state.mtx_strings);
					state.current_file = StrMB2Wide(it.name);
				}
				state.is_directory = it.is_dir;
				state.count_complete = i - failedIndex;
				state.file_complete = 0;

				const uint64_t this_size = it.is_dir
					? _adbDevice->GetDirectoryInfo(srcPath).total_size
					: it.size;
				state.file_total = this_size;

				if (overwrite[i]) {
					if (it.is_dir) dst->_adbDevice->DeleteDirectory(dstPath);
					else           dst->_adbDevice->DeleteFile(dstPath);
				}

				const uint64_t bytes_before = bytes_done;
				last_percent = -1;
				auto onProgress = [&](int percent) {
					if (percent == last_percent) return;
					last_percent = percent;
					state.file_complete = percent;
					if (this_size > 0) {
						state.all_complete = bytes_before + (this_size * percent) / 100;
					}
					INPUT_RECORD ir = {};
					ir.EventType = NOOP_EVENT;
					DWORD dw = 0;
					WINPORT(WriteConsoleInput)(0, &ir, 1, &dw);
				};
				auto onAbort = [&]() { return state.ShouldAbort(); };

				const int pullRc = it.is_dir
					? _adbDevice->PullDirectory(srcPath, tmpPath, onProgress, onAbort)
					: _adbDevice->PullFile(srcPath, tmpPath, onProgress, onAbort);
				if (pullRc != 0) {
					lastErr = pullRc;
					DBG("CrossPanel fallback pull failed item=%s err=%d\n", it.name.c_str(), pullRc);
					break;
				}

				last_percent = -1;
				const int pushRc = it.is_dir
					? dst->_adbDevice->PushDirectory(tmpPath, dstPath, onProgress, onAbort)
					: dst->_adbDevice->PushFile(tmpPath, dstPath, onProgress, onAbort);
				if (pushRc != 0) {
					lastErr = pushRc;
					DBG("CrossPanel fallback push failed item=%s err=%d\n", it.name.c_str(), pushRc);
					break;
				}
				if (move) {
					const int delRc = it.is_dir ? _adbDevice->DeleteDirectory(srcPath)
					                            : _adbDevice->DeleteFile(srcPath);
					if (delRc != 0) {
						lastErr = delRc;
						DBG("CrossPanel fallback delete failed item=%s err=%d\n", it.name.c_str(), delRc);
						break;
					}
				}
				(void)RemoveLocalPathRecursively(tmpPath);
				bytes_done += this_size;
				state.all_complete = bytes_done;
				state.count_complete = i - failedIndex + 1;
				++okCount;
			}
		});
		(void)RemoveLocalPathRecursively(tmpRoot);
	}

	if (okCount > 0) {
		g_Info.Control(PANEL_ACTIVE, FCTL_CLEARSELECTION, 0, 0);
		g_Info.Control(PANEL_ACTIVE, FCTL_UPDATEPANEL, 0, 0);
		g_Info.Control(PANEL_ACTIVE, FCTL_REDRAWPANEL, 0, 0);
		g_Info.Control(PANEL_PASSIVE, FCTL_UPDATEPANEL, 0, 0);
		g_Info.Control(PANEL_PASSIVE, FCTL_REDRAWPANEL, 0, 0);
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
		buf.assign((size_t)size + 0x100, 0);
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

	// Single prefill "<dir>/<name>.copy"; multi prefill "<dir>/" folder-only.
	std::string typed;
	{
		std::string prefill = BuildPanelPathPrefix(dir);
		if (!multi) prefill += items[0].name + ".copy";
		if (!ADBDialogs::AskInput(L"Copy", L"Copy to:", L"ADB_Copy", typed, prefill)) {
			return true;  // user cancelled — claim keystroke
		}
		if (typed.empty()) return true;
	}

	// Folder-only: trailing slash, "..", ".", existing-dir match, or multi.
	std::string folder_target = ResolveAdbDestinationFor(dir, typed, "", _adbDevice);
	const bool folder_only = multi
		|| (typed.back() == '/')
		|| typed == ".."
		|| typed == "."
		|| _adbDevice->IsDirectory(folder_target);

	const bool same_dir_as_source = (folder_only && (folder_target == dir
		|| folder_target == BuildPanelPathPrefix(dir)
		|| folder_target + "/" == BuildPanelPathPrefix(dir)));

	std::string single_new_name;
	if (!multi && !folder_only) {
		// User typed a basename (possibly with relative path); resolver
		// places it under the right parent. Extract the basename.
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
	};

	std::vector<Plan> plan;
	plan.reserve(items.size());
	bool overwrite_all = false, skip_all = false;
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

		if (_adbDevice->FileExists(p.dst_path)) {
			if (skip_all)      { p.skip = true; }
			else if (overwrite_all) { p.overwrite = true; }
			else {
				OverwriteDialog dlg(StrMB2Wide(p.dst_path), multi, p.is_dir);
				switch (dlg.Ask()) {
					case OverwriteDialog::OVERWRITE:     p.overwrite = true; break;
					case OverwriteDialog::SKIP:          p.skip = true; break;
					case OverwriteDialog::OVERWRITE_ALL: overwrite_all = true; p.overwrite = true; break;
					case OverwriteDialog::SKIP_ALL:      skip_all = true; p.skip = true; break;
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
	for (size_t i = 0; i < plan.size(); ++i) {
		if (plan[i].skip) continue;
		// Aside-rename atomic-ish overwrite: rename existing dest aside
		// first; on success delete the aside, on failure restore.
		std::string aside;
		bool have_aside = false;
		if (plan[i].overwrite) {
			aside = AsideName(plan[i].dst_path);
			if (_adbDevice->MoveRemoteAs(plan[i].dst_path, aside) == 0) {
				have_aside = true;
			} else {
				// Fall back to delete-then-copy when rename-aside fails.
				if (plan[i].is_dir) _adbDevice->DeleteDirectory(plan[i].dst_path);
				else                _adbDevice->DeleteFile(plan[i].dst_path);
			}
		}
		const int rc = _adbDevice->CopyRemoteAs(plan[i].src_path, plan[i].dst_path);
		if (rc == 0) {
			++okCount;
			if (have_aside) {
				if (plan[i].is_dir) _adbDevice->DeleteDirectory(aside);
				else                _adbDevice->DeleteFile(aside);
			}
		} else {
			if (have_aside) _adbDevice->MoveRemoteAs(aside, plan[i].dst_path);
			lastErr = rc;
			DBG("ShiftF5: device-side copy failed item=%s err=%d, queuing fallback\n",
				plan[i].new_name.c_str(), rc);
			needFallback.push_back(i);
		}
	}

	if (!needFallback.empty()) {
		// Host-mediated fallback: pull → push. Pre-scan for total bar.
		uint64_t total_bytes = 0;
		for (size_t idx : needFallback) {
			auto& p = plan[idx];
			if (p.is_dir) {
				total_bytes += _adbDevice->GetDirectoryInfo(p.src_path).total_size;
			} else {
				total_bytes += p.size;
			}
		}

		const char* tmpDir = getenv("TMPDIR");
		if (!tmpDir) tmpDir = "/tmp";
		char tmpTpl[PATH_MAX];
		snprintf(tmpTpl, sizeof(tmpTpl), "%s/adb_shiftf5_XXXXXX", tmpDir);
		char* tmpBase = mkdtemp(tmpTpl);
		if (tmpBase) {
			const std::string tmpRoot = tmpBase;
			DBG("ShiftF5: host-mediated fallback for %zu item(s) tmp=%s\n",
				needFallback.size(), tmpRoot.c_str());

			bool fb_any_dir = false;
			for (size_t idx : needFallback) if (plan[idx].is_dir) { fb_any_dir = true; break; }
			const bool is_multi = needFallback.size() > 1 || fb_any_dir;
			ProgressOperation pop(L"Copy on device", is_multi);
			pop.GetState().count_total = needFallback.size();
			pop.GetState().all_total = total_bytes;
			pop.Run([&](ProgressState& state) {
				uint64_t bytes_done = 0;
				int last_percent = -1;
				for (size_t i = 0; i < needFallback.size(); ++i) {
					if (state.ShouldAbort()) break;
					auto& p = plan[needFallback[i]];
					{
						std::lock_guard<std::mutex> lk(state.mtx_strings);
						state.current_file = StrMB2Wide(p.new_name);
					}
					state.is_directory = p.is_dir;
					state.count_complete = i;
					state.file_complete = 0;

					const std::string staged = ADBUtils::JoinPath(tmpRoot, p.new_name);

					// Files use FindData.nFileSize; dirs walked via shell.
					uint64_t this_size = p.is_dir
						? _adbDevice->GetDirectoryInfo(p.src_path).total_size
						: p.size;
					state.file_total = this_size;

					const uint64_t bytes_before = bytes_done;
					last_percent = -1;
					auto onPullProgress = [&](int percent) {
						// Debounce: redraw only on integer-percent change.
						if (percent == last_percent) return;
						last_percent = percent;
						state.file_complete = percent;
						if (this_size > 0) {
							state.all_complete = bytes_before + (this_size * percent) / 100;
						}
						INPUT_RECORD ir = {};
						ir.EventType = NOOP_EVENT;
						DWORD dw = 0;
						WINPORT(WriteConsoleInput)(0, &ir, 1, &dw);
					};
					auto onAbort = [&]() { return state.ShouldAbort(); };

					const int pullRc = p.is_dir
						? _adbDevice->PullDirectory(p.src_path, staged, onPullProgress, onAbort)
						: _adbDevice->PullFile(p.src_path, staged, onPullProgress, onAbort);
					if (pullRc != 0) { lastErr = pullRc; continue; }

					last_percent = -1;
					auto onPushProgress = [&](int percent) {
						if (percent == last_percent) return;
						last_percent = percent;
						state.file_complete = percent;
						if (this_size > 0) {
							state.all_complete = bytes_before + (this_size * percent) / 100;
						}
						INPUT_RECORD ir = {};
						ir.EventType = NOOP_EVENT;
						DWORD dw = 0;
						WINPORT(WriteConsoleInput)(0, &ir, 1, &dw);
					};

					const int pushRc = p.is_dir
						? _adbDevice->PushDirectory(staged, p.dst_path, onPushProgress, onAbort)
						: _adbDevice->PushFile(staged, p.dst_path, onPushProgress, onAbort);
					(void)RemoveLocalPathRecursively(staged);
					if (pushRc != 0) { lastErr = pushRc; continue; }

					bytes_done += this_size;
					state.all_complete = bytes_done;
					state.count_complete = i + 1;
					++okCount;
				}
			});
			(void)RemoveLocalPathRecursively(tmpRoot);
		}
	}

	if (okCount == 0 && lastErr != 0) {
		WINPORT(SetLastError)(lastErr);
		ADBDialogs::MessageWrapped(FMSG_WARNING | FMSG_MB_OK,
		                           L"Copy failed",
		                           L"Device-side cp and host-mediated pull/push both failed.");
	}
	g_Info.Control(PANEL_ACTIVE, FCTL_UPDATEPANEL, 0, 0);
	g_Info.Control(PANEL_ACTIVE, FCTL_REDRAWPANEL, 0, 0);
	return true;
}

bool ADBPlugin::ShiftF6Rename() {
	// Single-item rename only — matches Far convention.
	if (!_isConnected || !_adbDevice || _deviceSerial.empty()) return false;

	std::vector<ADBSelected> items;
	if (!GatherSelectedItems(items)) return false;
	if (items.empty()) return false;

	const auto& it = items[0];
	const std::string dir = GetCurrentDevicePath();
	if (dir.empty()) return false;

	// Prefill: full path + name. User can rename, move, or move-with-rename.
	const std::string prefill = BuildPanelPathPrefix(dir) + it.name;
	std::string raw_input;
	if (!ADBDialogs::AskInput(L"Rename", L"Rename to:", L"ADB_Rename",
	                          raw_input, prefill)) {
		return true;
	}
	if (raw_input.empty() || raw_input == prefill) return true;

	const std::string srcPath = ADBUtils::JoinPath(dir, it.name);
	// Resolve input; folder-only forms keep src filename via src_name.
	const std::string dstPath = ResolveAdbDestinationFor(dir, raw_input, it.name, _adbDevice);
	if (dstPath == srcPath) return true;

	if (_adbDevice->FileExists(dstPath)) {
		OverwriteDialog dlg(StrMB2Wide(dstPath), false, it.is_dir);
		auto choice = dlg.Ask();
		if (choice == OverwriteDialog::SKIP || choice == OverwriteDialog::SKIP_ALL) {
			return true;
		}
		if (choice == OverwriteDialog::CANCEL) return true;
		// Atomic-ish overwrite: rename existing target aside, do the
		// rename, on success rm the aside copy. On failure restore.
		const std::string aside = AsideName(dstPath);
		const int aside_rc = _adbDevice->MoveRemoteAs(dstPath, aside);
		if (aside_rc != 0) {
			ADBDialogs::MessageWrapped(FMSG_WARNING | FMSG_MB_OK,
			                           L"Rename failed",
			                           L"Could not move existing destination aside.");
			return true;
		}
		const int rc = _adbDevice->MoveRemoteAs(srcPath, dstPath);
		if (rc != 0) {
			// Restore so user doesn't lose data.
			_adbDevice->MoveRemoteAs(aside, dstPath);
			ADBDialogs::MessageWrapped(FMSG_WARNING | FMSG_MB_OK,
			                           L"Rename failed",
			                           StrMB2Wide(strerror(rc)));
			WINPORT(SetLastError)(rc);
			return true;
		}
		if (it.is_dir) _adbDevice->DeleteDirectory(aside);
		else           _adbDevice->DeleteFile(aside);
		g_Info.Control(PANEL_ACTIVE, FCTL_UPDATEPANEL, 0, 0);
		g_Info.Control(PANEL_ACTIVE, FCTL_REDRAWPANEL, 0, 0);
		return true;
	}

	const int rc = _adbDevice->MoveRemoteAs(srcPath, dstPath);
	if (rc != 0) {
		ADBDialogs::MessageWrapped(FMSG_WARNING | FMSG_MB_OK,
		                           L"Rename failed",
		                           StrMB2Wide(strerror(rc)));
		WINPORT(SetLastError)(rc);
		return true;
	}
	g_Info.Control(PANEL_ACTIVE, FCTL_UPDATEPANEL, 0, 0);
	g_Info.Control(PANEL_ACTIVE, FCTL_REDRAWPANEL, 0, 0);
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

	if (deviceInfos.empty()) {
		DBG("No ADB devices found\n");
		wcscpy(_PanelTitle, L"ADB: No devices found");
		*pItemsNumber = 1;
		*pPanelItem = new PluginPanelItem[1];
		memset(*pPanelItem, 0, sizeof(PluginPanelItem));
		(*pPanelItem)[0].FindData.lpwszFileName = ADBDevice::AllocateItemString("<Not found>");
		(*pPanelItem)[0].FindData.dwFileAttributes = FILE_ATTRIBUTE_NORMAL;

		wchar_t **customData = new wchar_t*[3];
		customData[0] = ADBDevice::AllocateItemString("<Connect device>");  // C0: Device Name
		customData[1] = ADBDevice::AllocateItemString("");                  // C1: Model
		customData[2] = ADBDevice::AllocateItemString("");                  // C2: Port
		(*pPanelItem)[0].CustomColumnData = customData;
		(*pPanelItem)[0].CustomColumnNumber = 3;

		return 1;
	}
	
	wcscpy(_PanelTitle, deviceInfos.size() > 1 ? L"ADB - Select Device" : L"ADB");

	std::vector<PluginPanelItem> devices;
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
		
		devices.push_back(device);
	}
	
	*pItemsNumber = (int)devices.size();
	*pPanelItem = new PluginPanelItem[devices.size()];
	
	for (size_t i = 0; i < devices.size(); i++) {
		(*pPanelItem)[i] = devices[i];
	}
	
	return (int)devices.size();
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
	
	if (!(OpMode & OPM_SILENT)) {
		std::string firstName = (ItemsNumber > 0) ? StrWide2MB(PanelItem[0].FindData.lpwszFileName) : "";
		if (!ADBDialogs::AskCopyMove(Move != 0, false, destPath, firstName, ItemsNumber)) {
			return -1;
		}
	}

	// Reroute device-path inputs ("..", "./*", existing-name, mid-".."
	// patterns) to in-device cp/mv; absolute host paths fall through.
	if (!(OpMode & OPM_VIEW)) {
		const std::string srcDir = GetCurrentDevicePath();
		const std::string& s = destPath;
		bool looks_in_device =
			s == "." || s == "./" || s == ".\\" ||
			s == ".." || s == "../" || s == "..\\" ||
			s.compare(0, 2, "./") == 0 || s.compare(0, 2, ".\\") == 0 ||
			s.compare(0, 3, "../") == 0 || s.compare(0, 3, "..\\") == 0 ||
			s.find("/..") != std::string::npos;
		// Single-component "mydir" matching an existing folder. Skip
		// names with '.' to avoid an ADB shell round-trip on "foo.txt".
		if (!looks_in_device && !s.empty()
		    && s.find_first_of("/\\") == std::string::npos
		    && s.find('.') == std::string::npos) {
			std::string probe = srcDir;
			if (probe.empty() || probe.back() != '/') probe += '/';
			probe += s;
			if (_adbDevice->IsDirectory(probe)) {
				looks_in_device = true;
			}
		}
		if (looks_in_device) {
			const std::string dstDir = NormalizePosixPath(
				(!destPath.empty() && destPath.front() == '/')
					? destPath
					: (srcDir + "/" + destPath));
			DBG("GetFiles: in-device target detected, srcDir=%s dstDir=%s move=%d\n",
				srcDir.c_str(), dstDir.c_str(), Move);
			int okCount = 0;
			int lastErr = 0;
			for (int i = 0; i < ItemsNumber; ++i) {
				if (!PanelItem[i].FindData.lpwszFileName) continue;
				std::string nm = StrWide2MB(PanelItem[i].FindData.lpwszFileName);
				if (nm.empty() || nm == "." || nm == "..") continue;
				const std::string srcPath = ADBUtils::JoinPath(srcDir, nm);
				const int rc = Move ? _adbDevice->MoveRemote(srcPath, dstDir)
				                    : _adbDevice->CopyRemote(srcPath, dstDir);
				if (rc == 0) ++okCount;
				else lastErr = rc;
			}
			if (okCount == 0 && lastErr != 0) WINPORT(SetLastError)(lastErr);
			g_Info.Control(PANEL_ACTIVE, FCTL_CLEARSELECTION, 0, 0);
			g_Info.Control(PANEL_ACTIVE, FCTL_UPDATEPANEL, 0, 0);
			g_Info.Control(PANEL_ACTIVE, FCTL_REDRAWPANEL, 0, 0);
			g_Info.Control(PANEL_PASSIVE, FCTL_UPDATEPANEL, 0, 0);
			g_Info.Control(PANEL_PASSIVE, FCTL_REDRAWPANEL, 0, 0);
			return okCount > 0 ? TRUE : FALSE;
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

	// Pre-scan total size/count; wrap to swallow GetDirectoryInfo throws.
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

	std::string srcPath = StrWide2MB(SrcPath);

	// PutFiles: far2l's ShellCopy already prompts + checks collisions;
	// don't double-prompt.

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
		// Single file → one bar. Multi-item or any directory → second
		// aggregate bar showing overall % across all selected items.
		bool any_dir = false;
		for (int k = 0; k < itemsCount; ++k) {
			if (items[k].FindData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) {
				any_dir = true; break;
			}
		}
		const bool is_multi = itemsCount > 1 || any_dir;
		ProgressOperation op(title, is_multi);
		op.GetState().file_total = totalBytes;
		op.GetState().all_total = totalBytes;
		op.GetState().count_total = totalFiles;
		op.GetState().source_path = is_upload ? StrMB2Wide(localDir) : StrMB2Wide(deviceDir);
		op.GetState().dest_path = is_upload ? StrMB2Wide(deviceDir) : StrMB2Wide(localDir);

		int overwriteMode = 0;
		const bool isMultiple = itemsCount > 1;
		int* pSuccessCount = &successCount;
		int* pLastError = &lastErrorCode;

		op.Run([&](ProgressState& state) {
			uint64_t processedBytes = 0;
			uint64_t completedCount = 0;
			if (!adb || !adb->IsConnected()) return;

			// Worker-thread exception guard (escape → std::terminate).
			try {
			for (int i = 0; i < itemsCount && !state.ShouldAbort(); i++) {
				std::string fileName = StrWide2MB(items[i].FindData.lpwszFileName);
				std::string localPath = ADBUtils::JoinPath(localDir, fileName);
				std::string devicePath = ADBUtils::JoinPath(deviceDir, fileName);
				bool isDir = (items[i].FindData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) != 0;
				const uint64_t itemSize = isDir ? 0 : items[i].FindData.nFileSize;

				// We own collision UI both ways; far2l skips per-file
				// checks when dst is a plugin (flplugin.cpp:727).
				if (is_upload) {
					if (adb->FileExists(devicePath)) {
						int action = CheckOverwrite(StrMB2Wide(devicePath), isMultiple, isDir, overwriteMode, state);
						if (action == 1 || action == 2) continue;
						// Pre-delete on OVERWRITE so push doesn't merge
						// (for dirs) or fail on read-only (for files).
						if (isDir) adb->DeleteDirectory(devicePath);
						else       adb->DeleteFile(devicePath);
					}
				} else {
					struct stat st;
					if (stat(localPath.c_str(), &st) == 0) {
						int action = CheckOverwrite(StrMB2Wide(localPath), isMultiple, isDir, overwriteMode, state);
						if (action == 1 || action == 2) continue;
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
					(*pSuccessCount)++;
					processedBytes += dirTotalSize;
					completedCount++;
					state.file_complete = 100;
					state.all_complete = processedBytes;
					state.count_complete = completedCount;
				} else if (result != 0) {
					*pLastError = result;
				}
			}
			} catch (const std::exception& ex) {
				DBG("RunTransfer worker exception: %s\n", ex.what());
				*pLastError = EIO;
				state.SetAborting();
			} catch (...) {
				DBG("RunTransfer worker: unknown exception\n");
				*pLastError = EIO;
				state.SetAborting();
			}
		});
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
