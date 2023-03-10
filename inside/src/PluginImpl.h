#pragma once
#include <vector>
#include <map>
#include <memory>
#include <string>
#include <all_far.h>

struct ELFInfo;

class PluginImpl
{
	char _panel_title[512];

protected:
	std::string _name, _dir;

	static bool AddUnsized(FP_SizeItemList &il, const char *name, DWORD attrs);

	virtual bool OnGetFindData(FP_SizeItemList &il, int OpMode) = 0;
	virtual bool OnGetFile(const char *item_file, const char *data_path, uint64_t len) = 0;
	virtual bool OnPutFile(const char *item_file, const char *data_path) = 0;
	virtual bool OnDeleteFile(const char *item_file) = 0;

public:
	PluginImpl(const char *name);
	virtual ~PluginImpl();

	int GetFindData(PluginPanelItem **pPanelItem,int *pItemsNumber,int OpMode);
	void FreeFindData(PluginPanelItem *PanelItem,int ItemsNumber);
	int SetDirectory(const char *Dir,int OpMode);
	void GetOpenPluginInfo(struct OpenPluginInfo *Info);
	int DeleteFiles(struct PluginPanelItem *PanelItem,int ItemsNumber,int OpMode);
	int ProcessHostFile(struct PluginPanelItem *PanelItem,int ItemsNumber,int OpMode);
	int GetFiles(struct PluginPanelItem *PanelItem,int ItemsNumber,
		int Move,char *DestPath,int OpMode);
	int PutFiles(struct PluginPanelItem *PanelItem,int ItemsNumber,
		int Move,int OpMode);
	int ProcessKey(int Key,unsigned int ControlState);
};
