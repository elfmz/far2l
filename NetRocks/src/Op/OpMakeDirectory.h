#pragma once
#include "OpBase.h"
#include "./Utils/EnumerLocal.h"

class OpMakeDirectory : protected OpBase, protected ProgressStateIOUpdater
{
	std::string _dir_name;

	virtual void Process();

public:
	OpMakeDirectory(std::shared_ptr<SiteConnection> &connection, int op_mode, const std::string &base_dir, const std::string &dir_name);
	bool Do();
};
