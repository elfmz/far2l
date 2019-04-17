#pragma once
#include <string>
#include <windows.h>
#include "DialogUtils.h"
#include "XferDefs.h"

class XferConfirm : protected BaseDialog
{
	int _i_dblbox = -1;
	int _i_destination = -1, _i_doa_ask = -1, _i_doa_overwrite = -1, _i_doa_skip = -1;
	int _i_doa_overwrite_newer = -1, _i_doa_resume = -1, _i_doa_create_diff_name = -1;
	int _i_proceed = -1, _i_cancel = -1;

public:
	XferConfirm(XferKind xk, XferDirection xd, const std::string &destination);

	bool Confirm(XferDefaultOverwriteAction &xdoa);
};

