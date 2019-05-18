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
	OpConnect(int op_mode, const std::string &protocol, const std::string &host, unsigned int port,
		const std::string &username, const std::string &password, const std::string &directory);

	std::shared_ptr<IHost> Do();
};
