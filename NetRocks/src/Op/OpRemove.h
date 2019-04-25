#pragma once
#include "OpBase.h"
#include "./Utils/EnumerRemote.h"


class OpRemove : protected OpBase
{
	RemoteEntries _entries;

	std::shared_ptr<EnumerRemote> _enumer;

	virtual void Process();

public:
	OpRemove(std::shared_ptr<SiteConnection> &connection, int op_mode, const std::string &src_dir, struct PluginPanelItem *items, int items_count);
	bool Do();
};
