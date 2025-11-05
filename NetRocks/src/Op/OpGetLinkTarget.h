#pragma once
#include "OpBase.h"
#include "./Utils/Enumer.h"

class OpGetLinkTarget : protected OpBase
{
	virtual void Process();

	std::string _path, _result;
	bool _success{false};

	void DoInner();

public:
	OpGetLinkTarget(int op_mode, std::shared_ptr<IHost> &base_host, const std::string &base_dir, struct PluginPanelItem *item);

	bool Do(std::string &result);
	bool Do(std::wstring &result);
};
