#pragma once
#include "OpBase.h"
#include "./Utils/EnumerLocal.h"

	struct EnumStatusCallback
	{
		virtual bool OnEnumEntry() = 0;
	};

class OpEnumDirectory : protected OpBase, protected ProgressStateUpdaterCallback
{
	FP_SizeItemList &_result;

	virtual void Process();

public:
	OpEnumDirectory(std::shared_ptr<SiteConnection> &connection, int op_mode, const std::string &base_dir, FP_SizeItemList &result);
	bool Do();
};
