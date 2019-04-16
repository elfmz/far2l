#pragma once
#include <string>
#include <windows.h>
#include "DialogUtils.h"


class XferProgress : protected BaseDialog
{
	FarDialogItems _di;
	FarListWrapper _di_doa;

	int _i_dblbox = -1;
	int _i_cur_file = -1, _i_cur_file_progress_bar = -1, _i_cur_file_progress_perc = -1;
	int _i_total_progress_bar = -1, _i_total_progress_perc = -1;
	int _i_current_time_spent = -1, _i_current_time_remain = -1;
	int _i_total_time_spent = -1, _i_total_time_remain = -1;
	int _i_speed_current_label = -1, _i_speed_current = -1, _i_total_time_remain = -1;
	int _i_doa = -1, _i_cancel = -1, _i_pause_resume = -1, _i_cancel = -1;

public:
	XferProgress(XferKind xk, XferDirection xd, const std::string &destination);

	bool Confirm(XferDefaultOverwriteAction &xdoa);
};

