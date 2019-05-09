#pragma once
#include <string>
#include <windows.h>
#include "DialogUtils.h"
#include "Defs.h"

class ConfirmNewServerIdentity : protected BaseDialog
{
	int _i_ok = -1;

public:
	ConfirmNewServerIdentity(const std::string &site, const std::string &identity);

	bool Ask();
};
