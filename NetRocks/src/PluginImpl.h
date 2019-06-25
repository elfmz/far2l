#pragma once
#include <vector>
#include <deque>
#include <map>
#include <memory>
#include "Host/Host.h"
#include "UI/Defs.h"
#include "UI/Activities/WhatOnError.h"
#include "BackgroundTasks.h"

class PluginImpl
{
	friend class AllNetRocks;

	wchar_t _panel_title[64], _cur_dir[MAX_PATH], _mk_dir[MAX_PATH];
	bool _cur_dir_absolute = false;
	std::shared_ptr<IHost> _remote;
	std::shared_ptr<IHost> _local;

	struct StackedDir
	{
		std::shared_ptr<IHost> remote;
		std::wstring cur_dir;
		bool cur_dir_absolute;
	};

	std::deque<StackedDir> _dir_stack;
	std::shared_ptr<WhatOnErrorState> _wea_state = std::make_shared<WhatOnErrorState>();



	void UpdatePanelTitle();
	bool ValidateConnection();

	std::string CurrentSiteDir(bool with_ending_slash) const;
	void ByKey_EditSiteConnection(bool create_new);
	bool ByKey_TryCrossload(bool mv);

	BackgroundTaskStatus StartXfer(int op_mode, std::shared_ptr<IHost> &base_host, const std::string &base_dir,
		std::shared_ptr<IHost> &dst_host, const std::string &dst_dir, struct PluginPanelItem *items,
		int items_count, XferKind kind, XferDirection direction);

public:
	PluginImpl(const wchar_t *path = nullptr);
	virtual ~PluginImpl();

	int GetFindData(PluginPanelItem **pPanelItem, int *pItemsNumber, int OpMode);
	void FreeFindData(PluginPanelItem *PanelItem, int ItemsNumber);
	int SetDirectory(const wchar_t *Dir, int OpMode);
	void GetOpenPluginInfo(struct OpenPluginInfo *Info);
	int DeleteFiles(struct PluginPanelItem *PanelItem,int ItemsNumber,int OpMode);
	int GetFiles(struct PluginPanelItem *PanelItem, int ItemsNumber, int Move, const wchar_t *DestPath, int OpMode);
	int PutFiles(struct PluginPanelItem *PanelItem, int ItemsNumber, int Move, const wchar_t *SrcPath, int OpMode);
	int MakeDirectory(const wchar_t **Name, int OpMode);
	int ProcessKey(int Key, unsigned int ControlState);
	int ProcessEventCommand(const wchar_t *cmd);
};
