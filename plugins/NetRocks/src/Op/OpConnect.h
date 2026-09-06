#pragma once
#include <string>
#include <Threaded.h>
#include "OpBase.h"
#include "../Location.h"

class OpConnect : protected OpBase
{
	volatile bool _succeed;

	virtual void Process();

public:
	OpConnect(int op_mode, const std::string &standalone_config, const Location &location);

	std::shared_ptr<IHost> Do();
};
