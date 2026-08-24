#pragma once
#include <string>
#include <memory>
#include <windows.h>
#include "WhatOnError.h"
#include "../DialogUtils.h"
#include "../Defs.h"

class ComplexOperationProgress : protected BaseDialog
{
	ProgressState &_state;
	std::shared_ptr<WhatOnErrorState> _wea_state;
	ProgressStateStats _last_stats;
	std::string _last_path;
	int _finished = 0;

protected:
	bool _escape_to_background = false;

	int _i_cur_file = -1;
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

	int _i_errstats_separator = -1;
	int _i_erraction_reset = -1;

	int _i_background = -1, _i_pause_resume = -1, _i_cancel = -1;

	bool _errstats_colored = false;
	bool _has_any_auto_action = false;

	std::wstring _speed_current_label;

	unsigned long long _prev_complete = 0, _speed_current = 0, _speed_average = 0;
	unsigned long long _speed_rollavg = 0, _speed_rollavg_n = 0;
	std::chrono::milliseconds _prev_ts {};

	virtual LONG_PTR DlgProc(int msg, int param1, LONG_PTR param2);
	void OnIdle();
	void UpdateTimes();
	void UpdateTime(unsigned long long complete, unsigned long long total,
		const std::chrono::milliseconds &start, const std::chrono::milliseconds &paused, const std::chrono::milliseconds &now,
		int i_spent_ctl, int i_remain_ctl, int i_speed_lbl_ctl = -1, int i_speed_cur_ctl = -1, int i_speed_avg_ctl = -1);

public:
	ComplexOperationProgress(const std::string &path, ProgressState &state, std::shared_ptr<WhatOnErrorState> &wea_state, int title_lng, bool show_file_size_progress, bool allow_background);
	void Show();
};

class XferProgress : public ComplexOperationProgress
{
public:
	enum BackgroundMode
	{
		BM_DISALLOW_BACKGROUND,
		BM_ALLOW_BACKGROUND,
		BM_ALREADY_BACKGROUND
	};

	XferProgress(XferKind xk, XferDirection xd, const std::string &destination, ProgressState &state, std::shared_ptr<WhatOnErrorState> &wea_state, BackgroundMode bg_mode);
};

class RemoveProgress : public ComplexOperationProgress
{
public:
	RemoveProgress(const std::string &site_dir, ProgressState &state, std::shared_ptr<WhatOnErrorState> &wea_state);
};
