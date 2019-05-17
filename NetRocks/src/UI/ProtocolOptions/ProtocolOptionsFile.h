#pragma once
#include <string>
#include <windows.h>
#include "../DialogUtils.h"

class ProtocolOptionsFile : protected BaseDialog
{
	int _i_ok = -1, _i_cancel = -1;

public:
	ProtocolOptionsFile();

	void Ask(std::string &options);
};
