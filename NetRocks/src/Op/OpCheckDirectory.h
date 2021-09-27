#pragma once
#include "OpBase.h"

class OpCheckDirectory : protected OpBase
{
	std::string _final_path;
	volatile bool _succeed;
	virtual void Process();

public:
	OpCheckDirectory(int op_mode, std::shared_ptr<IHost> &base_host, const std::string &path, std::shared_ptr<WhatOnErrorState> &wea_state);
	bool Do(std::string &final_path);
};
