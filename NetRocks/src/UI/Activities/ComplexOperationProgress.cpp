#include <utils.h>
#include <TimeUtils.h>
#include "ComplexOperationProgress.h"
#include "Confirm.h"
#include "AbortOperationRequest.h"
#include "../../lng.h"
#include "../../Globals.h"


/*
345               21      29    35      43   48    54  58  62  64
 ====================== Download =============================
| Current file:                                              |
| [TEXTLABEL                                               ] |
|------------------------------------------------------------|
| File size:      #########  of ######### [================] |
| Total size:     #########  of ######### [================] |
| Total count:    #########  of ######### [================] |
| File time          SPENT:   ###:##.##  REMAIN:   ###:##.## |
| Total time         SPENT:   ###:##.##  REMAIN:   ###:##.## |
| Speed (?bps)     CURRENT:   [TEXT   ]  AVERAGE:  [TEXT   ] |
|------------------------------------------------------------|
| [&Background]    [&Pause]     [&Ask on error]    [&Cancel] |
 =============================================================
  5              20   25   30             45 48            60
*/

ComplexOperationProgress::ComplexOperationProgress(const std::string &path, ProgressState &state, std::shared_ptr<WhatOnErrorState> &wea_state, int title_lng, bool show_file_size_progress, bool allow_background)
	: _state(state), _wea_state(wea_state)
{
	_di.SetBoxTitleItem(title_lng);

	_di.SetLine(2);
	_di.AddAtLine(DI_TEXT, 5,62, 0, MXferCurrentFile);

	_di.NextLine();
	_i_cur_file = _di.AddAtLine(DI_TEXT, 5,62, 0, "...");

	_di.NextLine();
	_di.AddAtLine(DI_TEXT, 4,63, DIF_BOXCOLOR | DIF_SEPARATOR);


	if (show_file_size_progress) {
		_di.NextLine();
		_di.AddAtLine(DI_TEXT, 5,20, 0, MXferFileSize);
		_i_file_size_complete = _di.AddAtLine(DI_TEXT, 21,29, 0, "?????????");
		_di.AddAtLine(DI_TEXT, 30,35, 0, MXferOf);
		_i_file_size_total = _di.AddAtLine(DI_TEXT, 35,43, 0, "?????????");
		_i_file_size_progress_bar = _di.AddAtLine(DI_TEXT, 45,62, 0, "??????????????????");
	}

	_di.NextLine();
	_di.AddAtLine(DI_TEXT, 5,20, 0, MXferAllSize);
	_i_all_size_complete = _di.AddAtLine(DI_TEXT, 21,29, 0, "?????????");
	_di.AddAtLine(DI_TEXT, 30,35, 0, MXferOf);
	_i_all_size_total = _di.AddAtLine(DI_TEXT, 35,43, 0, "?????????");
	_i_all_size_progress_bar = _di.AddAtLine(DI_TEXT, 45,62, 0, "??????????????????");

	_di.NextLine();
	_di.AddAtLine(DI_TEXT, 5,20, 0, MXferCount);
	_i_count_complete = _di.AddAtLine(DI_TEXT, 21,29, 0, "?????????");
	_di.AddAtLine(DI_TEXT, 30,35, 0, MXferOf);
	_i_count_total = _di.AddAtLine(DI_TEXT, 35,43, 0, "?????????");
	_i_count_progress_bar = _di.AddAtLine(DI_TEXT, 45,62, 0, "??????????????????");

	_di.NextLine();
	_di.AddAtLine(DI_TEXT, 5,32, 0, MXferFileTimeSpent);
	_i_file_time_spent = _di.AddAtLine(DI_TEXT, 33,41, 0, "???:??.??");
	_di.AddAtLine(DI_TEXT, 44,53, 0, MXferRemain);
	_i_file_time_remain = _di.AddAtLine(DI_TEXT, 54,62, 0, "???:??.??");

	_di.NextLine();
	_di.AddAtLine(DI_TEXT, 5,32, 0, MXferAllTimeSpent);
	_i_all_time_spent = _di.AddAtLine(DI_TEXT, 33,41, 0, "???:??.??");
	_di.AddAtLine(DI_TEXT, 44,53, 0, MXferRemain);
	_i_all_time_remain = _di.AddAtLine(DI_TEXT, 54,62, 0, "???:??.??");

	_di.NextLine();
	_i_speed_current_label = _di.AddAtLine(DI_TEXT, 5,32, 0, MXferSpeedCurrent);
	_i_speed_current = _di.AddAtLine(DI_TEXT, 33,41, 0, "???");
	_di.AddAtLine(DI_TEXT, 44,53, 0, MXferAverage);
	_i_speed_average = _di.AddAtLine(DI_TEXT, 54,62, 0, "???");

	_di.NextLine();
	// this separator used to display retries/skips count
	_i_errstats_separator = _di.AddAtLine(DI_TEXT, 4,63, DIF_BOXCOLOR | DIF_SEPARATOR);

	_di.NextLine();
	if (allow_background) {
		_i_background = _di.AddAtLine(DI_BUTTON, 5,25, DIF_CENTERGROUP, MBackground);
	}
	_i_pause_resume = _di.AddAtLine(DI_BUTTON, 30,45, DIF_CENTERGROUP, _state.paused ? MResume : MPause);
	_i_erraction_reset = _di.AddAtLine(DI_BUTTON, 46,54, DIF_CENTERGROUP, MErrorActionReset);
	_i_cancel = _di.AddAtLine(DI_BUTTON, 55,60, DIF_CENTERGROUP, MCancel);

	SetFocusedDialogControl(_i_pause_resume);
	SetDefaultDialogControl(_i_pause_resume);

	TextFromDialogControl(_i_speed_current_label, _speed_current_label);
}

void ComplexOperationProgress::Show()
{
	while (!_state.finished) {
		_finished = 0;
		int r = BaseDialog::Show(L"ComplexOperationProgress", 6, 2, FDLG_REGULARIDLE);
		if (_finished || (_i_background != -1 && r == _i_background) || (r == -1 && _escape_to_background)) {
			break;

		} else if (r == _i_pause_resume) {
			bool paused;
			{
				std::lock_guard<std::mutex> locker(_state.mtx);
				paused = (_state.paused = !_state.paused);
			}
			TextToDialogControl(_i_pause_resume, paused ? MResume : MPause);

		} else if (r == _i_erraction_reset) {
			_wea_state->ResetAutoActions();

		} else {
//			fprintf(stderr, "r=%d\n", r);
			AbortOperationRequest(_state);
		}
	}
}

LONG_PTR ComplexOperationProgress::DlgProc(int msg, int param1, LONG_PTR param2)
{
	//fprintf(stderr, "%x %x\n", msg, param1);
	switch (msg) {
		case DN_INITDIALOG:
			_has_any_auto_action = _wea_state->HasAnyAutoAction();
			SetVisibleDialogControl(_i_erraction_reset, _has_any_auto_action);
		break;
		case DN_ENTERIDLE:
			OnIdle();
		break;
	}

	return BaseDialog::DlgProc(msg, param1, param2);
}

void ComplexOperationProgress::OnIdle()
{
	struct {
		bool path, file, all, count, errors;
	} changed = {};
	bool paused;
	uint64_t Colors[4];
	{
		std::lock_guard<std::mutex> locker(_state.mtx);
		if (_last_path != _state.path) {
			_last_path = _state.path;
			changed.path = true;
		}
		changed.file = (_state.stats.file_complete != _last_stats.file_complete
					|| _state.stats.file_total != _last_stats.file_total);

		changed.all = (_state.stats.all_complete != _last_stats.all_complete
					|| _state.stats.all_total != _last_stats.all_total);

		changed.count = (_state.stats.count_complete != _last_stats.count_complete
					|| _state.stats.count_total != _last_stats.count_total);

		changed.errors = (_state.stats.count_skips != _last_stats.count_skips
					|| _state.stats.count_retries != _last_stats.count_retries);

		_last_stats = _state.stats;
		paused = _state.paused;
		if (_state.finished && _finished == 0) {
			_finished = 1;
		}
	}

	if (changed.path) {
		AbbreviableTextToDialogControl(_i_cur_file, _last_path);
	}

	if (changed.file) {
		FileSizeToDialogControl(_i_file_size_complete, _last_stats.file_complete);
		FileSizeToDialogControl(_i_file_size_total, _last_stats.file_total);
		ProgressBarToDialogControl(_i_file_size_progress_bar, _last_stats.file_total
			? _last_stats.file_complete * 100 / _last_stats.file_total : -1);
	}

	if (changed.all) {
		FileSizeToDialogControl(_i_all_size_complete, _last_stats.all_complete);
		FileSizeToDialogControl(_i_all_size_total, _last_stats.all_total);
		ProgressBarToDialogControl(_i_all_size_progress_bar, _last_stats.all_total
			? _last_stats.all_complete * 100 / _last_stats.all_total : -1);
	}

	if (changed.count) {
		LongLongToDialogControlThSeparated(_i_count_complete, _last_stats.count_complete);
		LongLongToDialogControlThSeparated(_i_count_total, _last_stats.count_total);
		ProgressBarToDialogControl(_i_count_progress_bar, _last_stats.count_total
			? _last_stats.count_complete * 100 / _last_stats.count_total : -1);
	}

	if (changed.errors) {
		char sz[0x100] = {};
		snprintf(sz, sizeof(sz) - 1, G.GetMsgMB(MErrorsStatus),
			_last_stats.count_retries, _last_stats.count_skips);
		TextToDialogControl(_i_errstats_separator, sz);
		if (!_errstats_colored) {
			_errstats_colored = true;

			SendDlgMessage(DM_GETDEFAULTCOLOR, _i_errstats_separator, (LONG_PTR)Colors);
			Colors[0] &= ~(0x000000FFFF000000 | FOREGROUND_GREEN | FOREGROUND_BLUE);
			Colors[0] |= (0x0000000000FF0000 | FOREGROUND_RED);
			SendDlgMessage(DM_SETCOLOR, _i_errstats_separator, (LONG_PTR)Colors);

//			DWORD color_flags = 0;
//			SendDlgMessage(DM_GETCOLOR, _i_errstats_separator, (LONG_PTR)&color_flags);
//			color_flags&= ~(FOREGROUND_GREEN | FOREGROUND_BLUE);
//			color_flags|= DIF_SETCOLOR | FOREGROUND_RED;
//			SendDlgMessage(DM_SETCOLOR, _i_errstats_separator, color_flags);
		}
	} else if (_errstats_colored) {
		_errstats_colored = false;

		Colors[0] = 0;
		SendDlgMessage(DM_SETCOLOR, _i_errstats_separator, (LONG_PTR)Colors);

//		SendDlgMessage(DM_SETCOLOR, _i_errstats_separator, 0);
	}

	const bool has_any_auto_action = _wea_state->HasAnyAutoAction();
	if (_has_any_auto_action != has_any_auto_action) {
		_has_any_auto_action = has_any_auto_action;
		SetVisibleDialogControl(_i_erraction_reset, _has_any_auto_action);
	}

	if (_finished == 1) {
		_finished = 2;
		Close();

	} else if (!paused) {
		UpdateTimes();
	}
}

void ComplexOperationProgress::UpdateTimes()
{
	auto now = TimeMSNow();

	if (_last_stats.total_start.count()) {
		// must be first cuz it updates speeds
		UpdateTime(_last_stats.all_complete, _last_stats.all_total, _last_stats.total_start, _last_stats.total_paused,
			now, _i_all_time_spent, _i_all_time_remain, _i_speed_current_label, _i_speed_current, _i_speed_average);
	}

	if (_last_stats.current_start.count()) {
		UpdateTime(_last_stats.file_complete, _last_stats.file_total, _last_stats.current_start, _last_stats.current_paused,
			now, _i_file_time_spent, _i_file_time_remain);
	}
}

void ComplexOperationProgress::UpdateTime(unsigned long long complete, unsigned long long total,
		const std::chrono::milliseconds &start, const std::chrono::milliseconds &paused, const std::chrono::milliseconds &now,
		int i_spent_ctl, int i_remain_ctl, int i_speed_lbl_ctl, int i_speed_cur_ctl, int i_speed_avg_ctl)
{
	auto delta = now - start;//_last_stats.total_start;
	if (delta <= paused)
		return;

	delta-= paused;

	TimePeriodToDialogControl(i_spent_ctl, delta.count());

	if (i_speed_lbl_ctl == -1) {
		;
	} else if (_prev_ts.count()) {
		auto speed_delta_time = (now - _prev_ts).count();
		if (speed_delta_time >= 3000) {
			_speed_average = (complete * 1000ll / delta.count());
			_speed_current = (complete > _prev_complete) ? complete - _prev_complete : 0;
			_speed_current = (_speed_current * 1000ll / speed_delta_time);
			if (_speed_rollavg != 0) {
				if (_speed_rollavg_n < 16 && _speed_current != 0) {
					++_speed_rollavg_n;
				}
				_speed_rollavg = (_speed_current + (_speed_rollavg_n - 1) * _speed_rollavg) / _speed_rollavg_n;
			} else {
				_speed_rollavg_n = 1;
				_speed_rollavg = _speed_current;
			}
			_prev_complete = complete;
			_prev_ts = now;

			unsigned long long fraction;
			size_t p = _speed_current_label.find(L"()");
			if (p != std::string::npos) {
				fraction = _speed_current;
				std::wstring str = FileSizeToFractionAndUnits(fraction);
				str+= L"ps";
				std::wstring speed_current_label = _speed_current_label;
				speed_current_label.insert(p + 1, str);
				TextToDialogControl(i_speed_lbl_ctl, speed_current_label);
			} else
				fraction = 1;

			LongLongToDialogControlThSeparated(i_speed_cur_ctl, _speed_current / fraction);
			LongLongToDialogControlThSeparated(i_speed_avg_ctl, _speed_average / fraction);
		}
	} else {
		_prev_complete = complete;
		_prev_ts = now;
	}

	if (_speed_rollavg != 0 && complete < total) {
		TimePeriodToDialogControl(i_remain_ctl, (total - complete) * 1000ll / _speed_rollavg);

	} else {
		TextToDialogControl(i_remain_ctl, "??:??.??");
	}
}


//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

XferProgress::XferProgress(XferKind xk, XferDirection xd, const std::string &destination, ProgressState &state, std::shared_ptr<WhatOnErrorState> &wea_state, BackgroundMode bg_mode)
	: ComplexOperationProgress(destination, state, wea_state,
	(xk == XK_RENAME) ? MXferRenameTitle : ((xk == XK_COPY)
		? ((xd == XD_UPLOAD) ? MXferCopyUploadTitle : ((xd == XD_CROSSLOAD) ? MXferCopyCrossloadTitle : MXferCopyDownloadTitle))
		: ((xd == XD_UPLOAD) ? MXferMoveUploadTitle : ((xd == XD_CROSSLOAD) ? MXferMoveCrossloadTitle : MXferMoveDownloadTitle))),
	true, bg_mode != BM_DISALLOW_BACKGROUND)
{
	_escape_to_background = (bg_mode == BM_ALREADY_BACKGROUND);
	if (_escape_to_background && _i_background != -1) {
		SetFocusedDialogControl(_i_background);
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

RemoveProgress::RemoveProgress(const std::string &site_dir, ProgressState &state, std::shared_ptr<WhatOnErrorState> &wea_state)
	: ComplexOperationProgress(site_dir, state, wea_state, MRemoveTitle, false, false)
{
}

