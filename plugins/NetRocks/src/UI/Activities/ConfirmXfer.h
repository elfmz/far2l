#pragma once
#include <string>
#include <windows.h>
#include "../DialogUtils.h"
#include "../Defs.h"

class ConfirmXfer : protected BaseDialog
{
	XferKind _xk;
	XferDirection _xd;

	int _i_dblbox = -1, _i_text = -1;
	int _i_destination = -1, _i_ask = -1, _i_overwrite = -1, _i_skip = -1;
	int _i_overwrite_newer = -1, _i_resume = -1, _i_create_diff_name = -1;
	int _i_proceed = -1, _i_cancel = -1;

	std::string _prev_destination;

protected:
	virtual LONG_PTR DlgProc(int msg, int param1, LONG_PTR param2);

public:
	ConfirmXfer(XferKind xk, XferDirection xd);

	XferKind Ask(XferOverwriteAction &default_xoa, std::string &destination);
};
