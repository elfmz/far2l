#pragma once
#include <string>
#include <windows.h>
#include "../DialogUtils.h"
#include "../Defs.h"

class ConfirmRemove : protected BaseDialog
{
	int _i_proceed = -1, _i_cancel = -1;

public:
	ConfirmRemove(const std::string &site_dir);

	bool Ask();
};

class ConfirmRemoveSites : protected BaseDialog
{
	int _i_proceed = -1, _i_cancel = -1;

public:
	ConfirmRemoveSites();

	bool Ask();
};

class ConfirmMakeDir : protected BaseDialog
{
	int _i_dir_name = -1;
	int _i_proceed = -1, _i_cancel = -1;

public:
	ConfirmMakeDir(const std::string &default_name);

	std::string Ask();
};
