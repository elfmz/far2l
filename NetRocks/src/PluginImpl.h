#pragma once
#include <vector>
#include <map>
#include <memory>
#include <all_far.h>
#include <fstdlib.h>
#include "SiteConnection.h"

class PluginImpl
{
	char _panel_title[64], _cur_dir[MAX_PATH];
	std::shared_ptr<SiteConnection> _connection;

	void UpdatePanelTitle();
	bool ValidateConnection();

	std::string CurrentSiteDir(bool with_ending_slash) const;

public:
	PluginImpl(const char *path = nullptr);
	virtual ~PluginImpl();

	int GetFindData(PluginPanelItem **pPanelItem,int *pItemsNumber,int OpMode);
	void FreeFindData(PluginPanelItem *PanelItem,int ItemsNumber);
	int SetDirectory(const char *Dir,int OpMode);
	void GetOpenPluginInfo(struct OpenPluginInfo *Info);
	int DeleteFiles(struct PluginPanelItem *PanelItem,int ItemsNumber,int OpMode);
	int GetFiles(struct PluginPanelItem *PanelItem,int ItemsNumber,
		int Move,char *DestPath,int OpMode);
	int PutFiles(struct PluginPanelItem *PanelItem,int ItemsNumber,
		int Move,int OpMode);
	int MakeDirectory(const char *Name, int OpMode);
	int ProcessKey(int Key,unsigned int ControlState);
};
