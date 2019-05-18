#pragma once
#include <string>
#include <windows.h>
#include "../DialogUtils.h"

class ProtocolOptionsFile : protected BaseDialog
{
	int _i_ok = -1, _i_cancel = -1;
	int _i_command = -1, _i_extra = -1, _i_command_time_limit = -1;

public:
	ProtocolOptionsFile();

	void Ask(std::string &options);
};
