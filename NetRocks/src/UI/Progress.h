#pragma once
#include <string>
#include <windows.h>
#include "DialogUtils.h"
#include "Defs.h"

class BaseProgress : protected BaseDialog
{
	ProgressState &_state;
	ProgressStateStats _last_stats;
	std::string _last_path;
	bool _finished = false;

protected:
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
	BaseProgress(int title_lng, bool show_file_size_progress, const std::string &path, ProgressState &state);
	void Show();
};

class XferProgress : public BaseProgress
{
public:
	XferProgress(XferKind xk, XferDirection xd, const std::string &destination, ProgressState &state);
};

class RemoveProgress : public BaseProgress
{
public:
	RemoveProgress(const std::string &site_dir, ProgressState &state);
};

class DirOperationProgress : protected BaseDialog
{
public:
	enum Kind
	{
		K_ENUMDIR,
		K_CREATEDIR
	};

	DirOperationProgress(Kind kind, const std::string &object, ProgressState &state);
	void Show();

protected:
	virtual LONG_PTR DlgProc(HANDLE dlg, int msg, int param1, LONG_PTR param2);

private:
	Kind _kind;
	ProgressState &_state;
	int _finished = 0;
	int _i_dblbox = -1;
	unsigned long _last_count_complete = 0;
	std::string _title;
};
