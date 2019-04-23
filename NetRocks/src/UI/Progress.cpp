#include "Progress.h"
#include "../Globals.h"


/*                                                         62
345               21      29    35      43   48    54  58  60  64
 ====================== Download =============================
| Current file:                                              |
| [EDITBOX                                                 ] |
|------------------------------------------------------------|
| File size:      #########  of ######### [================] |
| Total size:     #########  of ######### [================] |
| Total count:    #########  of ######### [================] |
| File time          SPENT:   ###:##.##  REMAIN:   ###:##.## |
| Total time         SPENT:   ###:##.##  REMAIN:   ###:##.## |
| Speed (?bps)     CURRENT:   [TEXT   ]  AVERAGE:  [TEXT   ] |
|------------------------------------------------------------|
| [    &Background    ]    [    &Pause    ]  [   &Cancel   ] |
 =============================================================
  5              20   25   30             45 48            60
*/

BaseProgress::BaseProgress(int title_lng, bool show_file_size_progress, const std::string &path, ProgressState &state)
	: _state(state)
{
	_di.Add(DI_DOUBLEBOX, 3, 1, 64, show_file_size_progress ? 13 : 12, 0, title_lng);

	_di.SetLine(2);
	_di.AddOnLine(DI_TEXT, 5,62, 0, MXferCurrentFile);

	_di.NextLine();
	_i_cur_file = _di.AddOnLine(DI_EDIT, 5,62, DIF_READONLY, "...");

	_di.NextLine();
	_di.AddOnLine(DI_TEXT, 4,63, DIF_BOXCOLOR | DIF_SEPARATOR);


	if (show_file_size_progress) {
		_di.NextLine();
		_di.AddOnLine(DI_TEXT, 5,20, 0, MXferFileSize);
		_i_file_size_complete = _di.AddOnLine(DI_TEXT, 21,29, 0, "#########");
		_di.AddOnLine(DI_TEXT, 30,35, 0, MXferOf);
		_i_file_size_total = _di.AddOnLine(DI_TEXT, 35,43, 0, "#########");
		_i_file_size_progress_bar = _di.AddOnLine(DI_TEXT, 45,60, 0, "::::::::::::::::::");
	}

	_di.NextLine();
	_di.AddOnLine(DI_TEXT, 5,20, 0, MXferAllSize);
	_i_all_size_complete = _di.AddOnLine(DI_TEXT, 21,29, 0, "#########");
	_di.AddOnLine(DI_TEXT, 30,35, 0, MXferOf);
	_i_all_size_total = _di.AddOnLine(DI_TEXT, 35,43, 0, "#########");
	_i_all_size_progress_bar = _di.AddOnLine(DI_TEXT, 45,60, 0, "::::::::::::::::::");

	_di.NextLine();
	_di.AddOnLine(DI_TEXT, 5,20, 0, MXferCount);
	_i_count_complete = _di.AddOnLine(DI_TEXT, 21,29, 0, "#########");
	_di.AddOnLine(DI_TEXT, 30,35, 0, MXferOf);
	_i_count_total = _di.AddOnLine(DI_TEXT, 35,43, 0, "#########");
	_i_count_progress_bar = _di.AddOnLine(DI_TEXT, 45,60, 0, "::::::::::::::::::");

	_di.NextLine();
	_di.AddOnLine(DI_TEXT, 5,32, 0, MXferFileTimeSpent);
	_i_file_time_spent = _di.AddOnLine(DI_TEXT, 33,41, 0, "???:??.??");
	_di.AddOnLine(DI_TEXT, 44,53, 0, MXferRemain);
	_i_file_time_remain = _di.AddOnLine(DI_TEXT, 54,60, 0, "???:??.??");

	_di.NextLine();
	_di.AddOnLine(DI_TEXT, 5,32, 0, MXferAllTimeSpent);
	_i_all_time_spent = _di.AddOnLine(DI_TEXT, 33,41, 0, "???:??.??");
	_di.AddOnLine(DI_TEXT, 44,53, 0, MXferRemain);
	_i_all_time_remain = _di.AddOnLine(DI_TEXT, 54,60, 0, "???:??.??");

	_di.NextLine();
	_i_speed_current_label = _di.AddOnLine(DI_TEXT, 5,32, 0, MXferSpeedCurrent);
	_i_speed_current = _di.AddOnLine(DI_TEXT, 33,41, 0, "???");
	_di.AddOnLine(DI_TEXT, 44,53, 0, MXferAverage);
	_i_speed_average = _di.AddOnLine(DI_TEXT, 54,60, 0, "???");

	_di.NextLine();
	_di.AddOnLine(DI_TEXT, 4,63, DIF_BOXCOLOR | DIF_SEPARATOR);

	_di.NextLine();
	//_i_background = _di.AddOnLine(DI_BUTTON, 5,25, DIF_CENTERGROUP, MBackground);
	_i_pause_resume = _di.AddOnLine(DI_BUTTON, 30,45, DIF_CENTERGROUP, MPause); // MResume
	_i_cancel = _di.AddOnLine(DI_BUTTON, 48,60, DIF_CENTERGROUP, MCancel, nullptr, FDIS_DEFAULT);
}

void BaseProgress::Show()
{
	_finished = false;
	do {
		BaseDialog::Show(_di[_i_dblbox].Data, 6, 2);
	} while (!_finished);
}

LONG_PTR BaseProgress::DlgProc(HANDLE dlg, int msg, int param1, LONG_PTR param2)
{
	//fprintf(stderr, "%x %x\n", msg, param1);
	if (msg == DN_ENTERIDLE) {
		OnIdle(dlg);

	} else if (msg == DN_BTNCLICK) {
		if (param1 == _i_pause_resume) {
			bool paused;
			{
				std::lock_guard<std::mutex> locker(_state.mtx);
				paused = (_state.paused = !_state.paused);
			}
			TextToDialogControl(dlg, _i_pause_resume, paused ? MResume : MPause);

		} else if (param1 == _i_cancel) {
			std::lock_guard<std::mutex> locker(_state.mtx);
			_state.aborting = true;
		}
		return TRUE;
	}
	
	return BaseDialog::DlgProc(dlg, msg, param1, param2);
}

void BaseProgress::OnIdle(HANDLE dlg)
{
	struct {
		bool path, file, all, count;
	} changed = {};

	bool finished = false;

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

		_last_stats = _state.stats;
		finished = _state.finished;
	}

	if (changed.path) {
		TextToDialogControl(dlg, _i_cur_file, _last_path);
	}

	if (changed.file) {
		FileSizeToDialogControl(dlg, _i_file_size_complete, _last_stats.file_complete);
		FileSizeToDialogControl(dlg, _i_file_size_total, _last_stats.file_total);
		ProgressBarToDialogControl(dlg, _i_file_size_progress_bar, _last_stats.file_total
			? _last_stats.file_complete * 100 / _last_stats.file_total : -1);
	}

	if (changed.all) {
		FileSizeToDialogControl(dlg, _i_all_size_complete, _last_stats.all_complete);
		FileSizeToDialogControl(dlg, _i_all_size_total, _last_stats.all_total);
		ProgressBarToDialogControl(dlg, _i_all_size_progress_bar, _last_stats.all_total
			? _last_stats.all_complete * 100 / _last_stats.all_total : -1);
	}

	if (changed.count) {
		LongLongToDialogControl(dlg, _i_count_complete, _last_stats.count_complete);
		LongLongToDialogControl(dlg, _i_count_total, _last_stats.count_total);
		ProgressBarToDialogControl(dlg, _i_count_progress_bar, _last_stats.count_total
			? _last_stats.count_complete * 100 / _last_stats.count_total : -1);
	}

	if (finished && !_finished) {
		_finished = true;
		Close(dlg);
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

XferProgress::XferProgress(XferKind xk, XferDirection xd, const std::string &destination, ProgressState &state)
	: BaseProgress((xk == XK_COPY)
	? ((xd == XK_DOWNLOAD) ? MXferCopyDownloadTitle : MXferCopyUploadTitle)
	: ((xd == XK_DOWNLOAD) ? MXferMoveDownloadTitle : MXferMoveUploadTitle),
	true, destination, state)
{
}

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

RemoveProgress::RemoveProgress(const std::string &site_dir, ProgressState &state)
	: BaseProgress(MRemoveTitle, false, site_dir, state)
{
}
