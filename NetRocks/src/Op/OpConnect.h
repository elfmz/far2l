#pragma once
#include <string>
#include <Threaded.h>
#include "OpBase.h"

class OpConnect : protected OpBase
{
	volatile bool _succeed;

	virtual void Process();

public:
	OpConnect(int op_mode, const std::string &display_name);
	std::shared_ptr<IHost> Do();
};
