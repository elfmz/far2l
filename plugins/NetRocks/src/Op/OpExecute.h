#pragma once
#include <string>
#include <memory>
#include "../Host/Host.h"
#include "Utils/ExecCommandFIFO.hpp"
#include "OpBase.h"

class OpExecute : protected OpBase
{
	std::string _command;
	ExecCommandFIFO _fifo;
	volatile int _status = 0;

	void CleanupFIFO();

	virtual void Process();

public:
	OpExecute(int op_mode, std::shared_ptr<IHost> &host, const std::string &dir, const std::string &command);
	virtual ~OpExecute();

	void Do();
};
