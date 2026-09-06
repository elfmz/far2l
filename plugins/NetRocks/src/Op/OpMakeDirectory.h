#pragma once
#include "OpBase.h"

class OpMakeDirectory : protected OpBase
{
	std::string _dir_name;

	virtual void Process();

public:
	OpMakeDirectory(int op_mode, std::shared_ptr<IHost> &base_host, const std::string &base_dir, const std::string &dir_name);
	bool Do();
	const std::string &DirName() const { return _dir_name; }
};
