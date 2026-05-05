#pragma once

// Standard library includes
#include <vector>
#include <memory>
#include <string>
#include <map>
#include <unordered_map>

// System includes
#include <wchar.h>

// FAR Manager includes
#include "farplug-wide.h"

// Local includes
#include "ADBDevice.h"

// Global Info pointer declaration
extern PluginStartupInfo g_Info;
extern FarStandardFunctions g_FSF;

class ADBPlugin
{
public:
	static constexpr uint64_t PLUGIN_SIGNATURE = 0x414442434C49454FULL; // "ADBCLIEF"
	uint64_t _signature = PLUGIN_SIGNATURE;

private:
	wchar_t _PanelTitle[64];
	wchar_t _mk_dir[1024];

	std::wstring _curDirW;
	std::wstring _formatW;

	bool _isConnected;               // true = connected to device (file mode), false = device selection mode
	std::string _deviceSerial;
	std::string _CurrentDir;

	std::shared_ptr<class ADBDevice> _adbDevice;
	
	// Cache for device friendly names to avoid N+1 process spawns
	std::map<std::string, std::string> _friendlyNamesCache;

	// Helper method to get current device path
	std::string GetCurrentDevicePath() const;

	// Helper method to update panel title with device serial and path
	void UpdatePanelTitle(const std::string& deviceSerial, const std::string& path);

	struct DeviceInfo {
		std::string serial;
		std::string status;
		std::string model;
		std::string name;
		std::string usb;
	};
	std::vector<DeviceInfo> EnumerateDevices();

	// F5/F6 fast path: passive panel on same adb → in-device cp/mv (no host roundtrip); returns false if not same-device.
	bool CrossPanelCopyMoveSameDevice(bool move);

	// Shift+F5 in-place copy; multi auto-suffixes ".copy"; host-mediated fallback on cp fail.
	bool ShiftF5CopyInPlace();

	// Shift+F6 rename/move with full-path prompt + per-collision aside-rename.
	bool ShiftF6Rename();

	// Per-directory metadata. file_sizes keys are paths RELATIVE to that dir (matches adb -p output).
	struct DirMeta {
		uint64_t total_size = 0;
		uint64_t file_count = 0;
		std::unordered_map<std::string, uint64_t> file_sizes;
	};

	// One shell roundtrip for all `dirs`; missing/failed dirs yield empty meta (indistinguishable from real empty dir).
	std::map<std::string, DirMeta> PrescanDeviceDirs(const std::vector<std::string>& dirs);

	// Per-WorkUnit totals + per-file size index — uses prescan for dirs, hostKnownSize for files; missing dirs yield 0/0.
	struct UnitTotals {
		uint64_t total_bytes = 0;
		uint64_t total_files = 0;
		std::unordered_map<std::string, std::vector<std::pair<std::string, uint64_t>>> idx;
	};
	UnitTotals ComputeUnitTotals(const std::string& srcPath, bool isDir,
	                             uint64_t hostKnownSize,
	                             const std::map<std::string, DirMeta>& dirInfo) const;

	// Shared transfer engine for GetFiles/PutFiles.
	int RunTransfer(PluginPanelItem *items, int itemsCount, bool is_upload, bool move,
	                const std::string& localDir, const std::string& deviceDir,
	                const std::map<std::string, DirMeta>& dirMetas,
	                uint64_t totalBytes, uint64_t totalFiles, int OpMode);

public:
	ADBPlugin();
	// Non-virtual dtor — keeps _signature at offset 0 for cross-panel memcpy sig-check (no subclasses).
	~ADBPlugin();

	// Panel operations
	int GetFindData(PluginPanelItem **pPanelItem, int *pItemsNumber, int OpMode);
	void FreeFindData(PluginPanelItem *PanelItem, int ItemsNumber);
	void GetOpenPluginInfo(OpenPluginInfo *Info);
	int SetDirectory(const wchar_t *Dir, int OpMode);
	int ProcessKey(int Key, unsigned int ControlState);
	int ProcessEventCommand(const wchar_t *cmd, HANDLE hPlugin = nullptr);
	

	int ExitDeviceFilePanel();
	int GetDeviceData(PluginPanelItem **pPanelItem, int *pItemsNumber);
	int GetFileData(PluginPanelItem **pPanelItem, int *pItemsNumber);
	
	// Device selection methods
	bool ByKey_TryEnterSelectedDevice();
	std::string GetDeviceFriendlyName(const std::string& serial);
	std::string GetCurrentPanelItemDeviceName();
	bool ConnectToDevice(const std::string &deviceSerial);

	// File transfer methods
	int GetFiles(PluginPanelItem *PanelItem, int ItemsNumber, int Move, const wchar_t **DestPath, int OpMode);
	int PutFiles(PluginPanelItem *PanelItem, int ItemsNumber, int Move, const wchar_t *SrcPath, int OpMode);
	int ProcessHostFile(PluginPanelItem *PanelItem, int ItemsNumber, int OpMode);
	int DeleteFiles(PluginPanelItem *PanelItem, int ItemsNumber, int OpMode);
	int MakeDirectory(const wchar_t **Name, int OpMode);

	// far2l API access
	static PluginStartupInfo *GetInfo();
};
