#pragma once
#include <vector>
#include <deque>
#include <map>
#include <memory>
#include "Host/Host.h"
#include "UI/Defs.h"
#include "UI/Activities/WhatOnError.h"
#include "BackgroundTasks.h"
#include "Location.h"
#include "SitesConfig.h"

class PluginImpl
{
	friend class AllNetRocks;

	wchar_t _panel_title[64], _cur_dir[MAX_PATH], _mk_dir[MAX_PATH], _format[256];
	wchar_t _cur_URL[MAX_PATH];

	Location _location;
	bool _allow_remember_location_dir = false;

	SitesConfigLocation _sites_cfg_location;
	std::wstring _standalone_config;

	std::shared_ptr<IHost> _remote;
	std::shared_ptr<IHost> _local;

	struct StackedDir
	{
		std::shared_ptr<IHost> remote;
		Location location;
		SitesConfigLocation sites_cfg_location;
		std::wstring standalone_config;
	};

	std::deque<StackedDir> _dir_stack;
	std::shared_ptr<WhatOnErrorState> _wea_state = std::make_shared<WhatOnErrorState>();

	void StackedDirCapture(StackedDir &sd);
	void StackedDirApply(StackedDir &sd);

	void UpdatePathInfo();

	std::string CurrentSiteDir(bool with_ending_slash) const;
	void ByKey_EditSiteConnection(bool create_new);
	bool ByKey_TryCrossload(bool mv);
	bool ByKey_TryExecuteSelected();
	bool ByKey_TryEnterSelectedSite();
	void ByKey_EditAttributesSelected();

	BackgroundTaskStatus StartXfer(int op_mode, std::shared_ptr<IHost> &base_host, const std::string &base_dir,
		std::shared_ptr<IHost> &dst_host, const std::string &dst_dir, struct PluginPanelItem *items,
		int items_count, XferKind kind, XferDirection direction);

	BOOL SitesXfer(const char *dir, struct PluginPanelItem *PanelItem, int ItemsNumber, bool mv, bool imp);

	int SetDirectoryInternal(const wchar_t *Dir, int OpMode);

	void DismissRemoteHost();
	std::string CurrentConnectionPoolId();

	bool ValidateLocationDirectory(int OpMode);

public:
	PluginImpl(const wchar_t *path = nullptr, bool path_is_standalone_config = false, int OpMode = 0);
	virtual ~PluginImpl();

	static void sOnExiting();
	static void sOnGlobalSettingsChanged();

	int GetFindData(PluginPanelItem **pPanelItem, int *pItemsNumber, int OpMode);
	void FreeFindData(PluginPanelItem *PanelItem, int ItemsNumber);
	int SetDirectory(const wchar_t *Dir, int OpMode);
	void GetOpenPluginInfo(struct OpenPluginInfo *Info);
	int DeleteFiles(struct PluginPanelItem *PanelItem, int ItemsNumber, int OpMode);
	int GetLinkTarget(struct PluginPanelItem *PanelItem, wchar_t *Target, size_t TargetSize, int OpMode);
	int GetFiles(struct PluginPanelItem *PanelItem, int ItemsNumber, int Move, const wchar_t *DestPath, int OpMode);
	int PutFiles(struct PluginPanelItem *PanelItem, int ItemsNumber, int Move, const wchar_t *SrcPath, int OpMode);
	int MakeDirectory(const wchar_t **Name, int OpMode);
	int ProcessKey(int Key, unsigned int ControlState);
	int ProcessEventCommand(const wchar_t *cmd);
};
