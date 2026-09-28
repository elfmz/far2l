#pragma once
#include "OpBase.h"
#include "../PluginPanelItems.h"


class OpEnumDirectory : protected OpBase
{
	PluginPanelItems &_result;
	unsigned long long _initial_count_complete;
	int _initial_result_count;

	virtual void Process();

public:
	OpEnumDirectory(int op_mode, std::shared_ptr<IHost> &base_host, const std::string &base_dir, PluginPanelItems &result, std::shared_ptr<WhatOnErrorState> &wea_state);
	bool Do();
};
