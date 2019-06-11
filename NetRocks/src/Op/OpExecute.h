#pragma once
#include <string>
#include <memory>
#include "../Host/Host.h"

class OpExecute
{
	std::shared_ptr<IHost> _host;
	std::string  _dir;
	std::string _command;
	std::string _fifo;

	void CleanupFIFO();

public:
	OpExecute(std::shared_ptr<IHost> &host, const std::string &dir, const std::string &command);
	virtual ~OpExecute();

	bool Do();
};
