#pragma once
#include "OpBase.h"
#include "../PluginPanelItems.h"

class OpEnumDirectory : protected OpBase
{
	PluginPanelItems &_result;

	virtual void Process();

public:
	OpEnumDirectory(int op_mode, std::shared_ptr<IHost> &base_host, const std::string &base_dir, PluginPanelItems &result);
	bool Do();
};
