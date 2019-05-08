#pragma once
#include <string>
#include <map>
#include <windows.h>
#include "DialogUtils.h"
#include "Defs.h"

class WhatOnError : protected BaseDialog
{
	int _i_remember = -1, _i_retry = -1, _i_skip = -1;

public:
	WhatOnError(WhatOnErrorKind wek, const std::string &error, const std::string &object, const std::string &site);

	WhatOnErrorAction Ask(WhatOnErrorAction &default_wea);
};

class WhatOnErrorState
{
	std::map<std::string, WhatOnErrorAction> _default_weas[WEKS_COUNT];

	public:
	WhatOnErrorAction Query(WhatOnErrorKind wek, const std::string &error, const std::string &object, const std::string &site);
};
