#pragma once
#include "OpBase.h"

class OpGetMode : protected OpBase
{
	volatile mode_t _result = 0;
	volatile bool _succeed;
	virtual void Process();

public:
	OpGetMode(int op_mode, std::shared_ptr<IHost> &base_host, const std::string &path, std::shared_ptr<WhatOnErrorState> &wea_state);
	bool Do(mode_t &result);
};
