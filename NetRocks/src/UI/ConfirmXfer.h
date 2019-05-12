#pragma once
#include <string>
#include <windows.h>
#include "DialogUtils.h"
#include "Defs.h"

class ConfirmXfer : protected BaseDialog
{
	int _i_destination = -1, _i_ask = -1, _i_overwrite = -1, _i_skip = -1;
	int _i_overwrite_newer = -1, _i_resume = -1, _i_create_diff_name = -1;
	int _i_proceed = -1, _i_cancel = -1;

public:
	ConfirmXfer(XferKind xk, XferDirection xd);

	bool Ask(XferOverwriteAction &default_xoa, std::string &destination);
};
