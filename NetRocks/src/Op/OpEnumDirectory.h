#pragma once
#include "OpBase.h"
#include "../PluginPanelItems.h"

class OpEnumDirectory : protected OpBase
{
	PluginPanelItems &_result;

	virtual void Process();

public:
	OpEnumDirectory(std::shared_ptr<IHost> &base_host, int op_mode, const std::string &base_dir, PluginPanelItems &result);
	bool Do();
};
