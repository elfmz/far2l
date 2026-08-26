#pragma once
#include <string>
#include <windows.h>
#include "../DialogUtils.h"
#include "../Defs.h"

class ConfirmNewServerIdentity : protected BaseDialog
{
	int _i_allow_once = -1, _i_allow_always = -1;

public:
	enum Result
	{
		R_DENY = 0,
		R_ALLOW_ONCE,
		R_ALLOW_ALWAYS
	};
	ConfirmNewServerIdentity(const std::string &site, const std::string &identity, bool may_remember);

	Result Ask();
};
