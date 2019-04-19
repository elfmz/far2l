#pragma once
#include <string>
#include <mutex>
#include <windows.h>
#include "DialogUtils.h"
#include "XferDefs.h"

struct XferStateStats
{
	volatile long long total_count = 0, current_count = 0;
	volatile long long total_size = 0, current_size = 0;
	clock_t total_start = 0, current_start = 0;
	clock_t total_paused = 0, current_paused = 0;
};

struct XferState : XferStateStats
{
	std::mutex lock;
	XferDefaultOverwriteAction xdoa = XDOA_ASK;
	const std::string *name = nullptr;
	bool paused = false, aborting = false, finished = false;
};

class XferProgress : protected BaseDialog
{
	XferState &_state;
	XferStateStats _last_stats;
	std::string _last_name;
	bool _finished = false;

	int _i_dblbox = -1;
	int _i_cur_file = -1, _i_cur_file_progress_bar = -1, _i_cur_file_progress_perc = -1;
	int _i_total_progress_bar = -1, _i_total_progress_perc = -1;
	int _i_total_count = -1, _i_current_count = -1;
	int _i_total_size = -1, _i_current_size = -1;
	int _i_current_time_spent = -1, _i_current_time_remain = -1;
	int _i_total_time_spent = -1, _i_total_time_remain = -1;
	int _i_speed_current_label = -1, _i_speed_current = -1;
	int /*_i_background = -1, */_i_pause_resume = -1, _i_cancel = -1;

	virtual LONG_PTR DlgProc(HANDLE dlg, int msg, int param1, LONG_PTR param2);
	void OnIdle(HANDLE dlg);

public:
	XferProgress(XferKind xk, XferDirection xd, const std::string &destination, XferState &state);
	void Show();
};

