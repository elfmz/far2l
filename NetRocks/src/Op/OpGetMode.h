#pragma once
#include "OpBase.h"

class OpGetMode : protected OpBase
{
	volatile mode_t _result = 0;
	volatile bool _succeed;
	virtual void Process();

public:
	OpGetMode(std::shared_ptr<IHost> &base_host, int op_mode, const std::string &path);
	bool Do(mode_t &result);
};
