#pragma once
#include <vector>
#include <map>
#include <memory>
#include "Host/Host.h"

class PluginImpl
{
	friend class AllNetRocks;

	wchar_t _panel_title[64], _cur_dir[MAX_PATH];
	std::shared_ptr<IHost> _remote;
	std::shared_ptr<IHost> _local;

	std::string _remote_root_dir;

	void UpdatePanelTitle();
	bool ValidateConnection();

	std::string CurrentSiteDir(bool with_ending_slash) const;
	void FromKey_EditSiteConnection(bool create_new);
	bool FromKey_TryCrossSiteCrossload(bool mv);

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
	int MakeDirectory(const wchar_t *Name, int OpMode);
	int ProcessKey(int Key, unsigned int ControlState);
};
