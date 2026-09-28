#pragma once
#include "OpBase.h"
#include "./Utils/Enumer.h"


class OpRemove : protected OpBase
{
	Path2FileInformation _entries;

	std::shared_ptr<Enumer> _enumer;

	virtual void Process();

public:
	OpRemove(int op_mode, std::shared_ptr<IHost> &base_host, const std::string &base_dir, struct PluginPanelItem *items, int items_count);
	bool Do();
};
