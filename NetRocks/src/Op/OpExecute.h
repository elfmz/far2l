#pragma once
#include <string>
#include <memory>
#include "../Host/Host.h"
#include "Utils/ExecCommandFIFO.hpp"

class OpExecute
{
	std::shared_ptr<IHost> _host;
	std::string  _dir;
	std::string _command;
	ExecCommandFIFO _fifo;

	void CleanupFIFO();

public:
	OpExecute(std::shared_ptr<IHost> &host, const std::string &dir, const std::string &command);
	virtual ~OpExecute();

	void Do();
};
