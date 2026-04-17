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
public:
	static constexpr uint64_t PLUGIN_SIGNATURE = 0x414442434C49454FULL; // "ADBCLIEF"
	uint64_t _signature = PLUGIN_SIGNATURE;

private:
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

	// Cursor position tracking for better UX
	std::string _lastEnteredDir;
	
	// ADBDevice for file operations
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
	bool CrossPanelCopyMoveSameDevice(bool move);

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
