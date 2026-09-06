#pragma once
#include <string>
#include <windows.h>
#include "../DialogUtils.h"
#include "../Defs.h"

void BackgroundTasksList();

class ConfirmExitFAR : protected BaseDialog
{
	int _i_background_tasks = -1, _i_ok = -1;

	virtual LONG_PTR DlgProc(int msg, int param1, LONG_PTR param2);
public:
	ConfirmExitFAR(size_t background_ops_count);

	bool Ask();
};
