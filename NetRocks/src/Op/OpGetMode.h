#pragma once
#include "OpBase.h"
#include "./Utils/EnumerLocal.h"

class OpGetMode : protected OpBase
{
	volatile mode_t _result = 0;
	volatile bool _succeed;
	virtual void Process();

public:
	OpGetMode(std::shared_ptr<SiteConnection> &connection, int op_mode, const std::string &path);
	bool Do(mode_t &result);
};
