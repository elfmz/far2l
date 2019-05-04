#pragma once
#include "OpBase.h"

class OpMakeDirectory : protected OpBase
{
	std::string _dir_name;

	virtual void Process();

public:
	OpMakeDirectory(std::shared_ptr<IHost> &base_host, int op_mode, const std::string &base_dir, const std::string &dir_name);
	bool Do();
};
