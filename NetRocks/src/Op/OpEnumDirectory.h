#pragma once
#include "OpBase.h"

class OpEnumDirectory : protected OpBase
{
	FP_SizeItemList &_result;

	virtual void Process();

public:
	OpEnumDirectory(std::shared_ptr<IHost> &base_host, int op_mode, const std::string &base_dir, FP_SizeItemList &result);
	bool Do();
};
