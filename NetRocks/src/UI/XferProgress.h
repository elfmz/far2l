#pragma once
#include <string>
#include <windows.h>
#include "DialogUtils.h"
#include "XferDefs.h"

class XferProgress : protected BaseDialog
{
	XferState &_state;
	XferStateStats _last_stats;
	std::string _last_path;
	bool _finished = false;

	int _i_dblbox = -1, _i_cur_file = -1;
	int _i_file_size_complete = -1;
	int _i_file_size_total = -1;
	int _i_file_size_progress_bar = -1;

	int _i_all_size_complete = -1;
	int _i_all_size_total = -1;
	int _i_all_size_progress_bar = -1;

	int _i_count_complete = -1;
	int _i_count_total = -1;
	int _i_count_progress_bar = -1;

	int _i_file_time_spent = -1;
	int _i_file_time_remain = -1;

	int _i_all_time_spent = -1;
	int _i_all_time_remain = -1;

	int _i_speed_current_label = -1;
	int _i_speed_current = -1;
	int _i_speed_average = -1;

	int /*_i_background = -1, */_i_pause_resume = -1, _i_cancel = -1;

	virtual LONG_PTR DlgProc(HANDLE dlg, int msg, int param1, LONG_PTR param2);
	void OnIdle(HANDLE dlg);

public:
	XferProgress(XferKind xk, XferDirection xd, const std::string &destination, XferState &state);
	void Show();
};

