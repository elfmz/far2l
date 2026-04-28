#pragma once

// Standard library includes
#include <vector>
#include <memory>
#include <string>
#include <map>

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
private:
	// Identity check for opaque PANEL handles returned by far2l's FCTL_GETPANELPLUGINHANDLE.
	// A signature field at object offset 0 is unsafe (vtable lives there because of virtual ~);
	// instead we maintain a registry of live instances. Access is mutex-guarded — far2l
	// normally calls plugin entry points on the UI thread, but our progress-UI worker
	// runs operations in std::thread, so defensive locking avoids silent UB if any future
	// code path touches the registry off-UI-thread.
	static bool IsLiveInstance(const void* candidate);

	// Panel state
	wchar_t _PanelTitle[64];
	wchar_t _mk_dir[1024];
	

	// Standalone config
	std::wstring _standalone_config;
	std::wstring _curDirW;
	std::wstring _formatW;
	bool _allow_remember_location_dir;
	
	bool _isConnected;               // true = connected to device (file mode), false = device selection mode
	std::string _deviceSerial;      // Current device serial
	std::string _CurrentDir;        // Current path on device

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

	// Unified handler for F5/F6/Shift+F5/Shift+F6 on a connected ADB panel.
	//   move=true   → F6 / Shift+F6 (move/rename)
	//   move=false  → F5 / Shift+F5 (copy/duplicate)
	//   in_place=true  → Shift+F5/F6: cursor item only, default = item name in current dir
	//   in_place=false → F5/F6:       active selection, default = passive panel dir
	// Returns true to consume the key (dialog shown, op attempted, or user cancelled);
	// false to defer to Far's default — only when in_place=false AND passive isn't an
	// ADB plugin instance, so Far's standard plugin↔FS routing through GetFiles/PutFiles
	// still handles the ADB↔host case (with Far's own "Move to:" dialog).
	bool HandleCopyMove(bool move, bool in_place);

	// Shared transfer engine for GetFiles/PutFiles (DRY)
	int RunTransfer(PluginPanelItem *items, int itemsCount, bool is_upload, bool move,
	                const std::string& localDir, const std::string& deviceDir,
	                const std::map<std::string, uint64_t>& dirSizes,
	                uint64_t totalBytes, uint64_t totalFiles, int OpMode);

public:
	ADBPlugin(const wchar_t *path = nullptr, bool path_is_standalone_config = false, int OpMode = 0);
	virtual ~ADBPlugin();

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
	std::string GetFirstAvailableDevice();
	int GetAvailableDeviceCount();
	bool ByKey_TryEnterSelectedDevice();
	std::string GetDeviceFriendlyName(const std::string& serial);
	int GetHighlightedDeviceIndex();
	std::string GetCurrentPanelItemDeviceName();
	std::string GetFallbackDeviceName();
	bool ConnectToDevice(const std::string &deviceSerial);

	// File transfer methods
	int GetFiles(PluginPanelItem *PanelItem, int ItemsNumber, int Move, const wchar_t **DestPath, int OpMode);
	int PutFiles(PluginPanelItem *PanelItem, int ItemsNumber, int Move, const wchar_t *SrcPath, int OpMode);
	int ProcessHostFile(PluginPanelItem *PanelItem, int ItemsNumber, int OpMode);
	int DeleteFiles(PluginPanelItem *PanelItem, int ItemsNumber, int OpMode);
	int MakeDirectory(const wchar_t **Name, int OpMode);
	bool IsDirectoryEmpty(const std::string &devicePath);
	
	// far2l API access
	static PluginStartupInfo *GetInfo();
};
