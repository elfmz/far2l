#include "XferProgress.h"
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

XferProgress::XferProgress(XferKind xk, XferDirection xd, const std::string &destinationa, XferState &state)
	: _state(state)
{
//	_di_doa.Add(MXferDOAAsk, (_state.xdoa == XDOA_ASK) ? LIF_SELECTED : 0);
//	_di_doa.Add(MXferDOASkip, (_state.xdoa == XDOA_SKIP) ? LIF_SELECTED : 0);
//	_di_doa.Add(MXferDOAResume, (_state.xdoa == XDOA_RESUME) ? LIF_SELECTED : 0);
//	_di_doa.Add(MXferDOAOverwrite, (_state.xdoa == XDOA_OVERWRITE) ? LIF_SELECTED : 0);
//	_di_doa.Add(MXferDOAOverwriteIfNewer, (_state.xdoa == XDOA_OVERWRITE_IF_NEWER) ? LIF_SELECTED : 0);
//	_di_doa.Add(MXferDOACreateDifferentName, (_state.xdoa == XDOA_CREATE_DIFFERENT_NAME) ? LIF_SELECTED : 0);

	if (xk == XK_COPY) {
		_di.Add(DI_DOUBLEBOX, 3,1,64,13, 0, (xd == XK_DOWNLOAD) ? MXferCopyDownloadTitle : MXferCopyUploadTitle);
	} else {
		_di.Add(DI_DOUBLEBOX, 3,1,64,13, 0, (xd == XK_DOWNLOAD) ? MXferMoveDownloadTitle : MXferMoveUploadTitle);
	}

	_di.Add(DI_TEXT, 5,2,62,2, 0, MXferCurrentFile);
	_i_cur_file = _di.Add(DI_EDIT, 5,3,62,3, DIF_READONLY, "...");
	_di.Add(DI_TEXT, 4,4,63,4, DIF_BOXCOLOR | DIF_SEPARATOR);

	_di.Add(DI_TEXT, 5,5,20,5, 0, MXferFileSize);
	_i_file_size_complete = _di.Add(DI_TEXT, 21,5,29,5, 0, "#########");
	_di.Add(DI_TEXT, 30,5,35,5, 0, MXferOf);
	_i_file_size_total = _di.Add(DI_TEXT, 35,5,43,5, 0, "#########");
	_i_file_size_progress_bar = _di.Add(DI_TEXT, 45,5,60,5, 0, "::::::::::::::::::");

	_di.Add(DI_TEXT, 5,6,20,6, 0, MXferAllSize);
	_i_all_size_complete = _di.Add(DI_TEXT, 21,6,29,6, 0, "#########");
	_di.Add(DI_TEXT, 30,6,35,6, 0, MXferOf);
	_i_all_size_total = _di.Add(DI_TEXT, 35,6,43,6, 0, "#########");
	_i_all_size_progress_bar = _di.Add(DI_TEXT, 45,6,60,6, 0, "::::::::::::::::::");

	_di.Add(DI_TEXT, 5,7,20,7, 0, MXferCount);
	_i_count_complete = _di.Add(DI_TEXT, 21,7,29,7, 0, "#########");
	_di.Add(DI_TEXT, 30,7,35,7, 0, MXferOf);
	_i_count_total = _di.Add(DI_TEXT, 35,7,43,7, 0, "#########");
	_i_count_progress_bar = _di.Add(DI_TEXT, 45,7,60,7, 0, "::::::::::::::::::");

	_di.Add(DI_TEXT, 5,8,32,8, 0, MXferFileTimeSpent);
	_i_file_time_spent = _di.Add(DI_TEXT, 33,8,41,8, 0, "???:??.??");
	_di.Add(DI_TEXT, 44,8,53,8, 0, MXferRemain);
	_i_file_time_remain = _di.Add(DI_TEXT, 54,8,60,8, 0, "???:??.??");

	_di.Add(DI_TEXT, 5,9,32,9, 0, MXferAllTimeSpent);
	_i_all_time_spent = _di.Add(DI_TEXT, 33,9,41,9, 0, "???:??.??");
	_di.Add(DI_TEXT, 44,9,53,9, 0, MXferRemain);
	_i_all_time_remain = _di.Add(DI_TEXT, 54,9,60,9, 0, "???:??.??");

	_i_speed_current_label = _di.Add(DI_TEXT, 5,10,32,10, 0, MXferSpeedCurrent);
	_i_speed_current = _di.Add(DI_TEXT, 33,10,41,10, 0, "???");
	_di.Add(DI_TEXT, 44,10,53,10, 0, MXferAverage);
	_i_speed_average = _di.Add(DI_TEXT, 54,10,60,10, 0, "???");

	_di.Add(DI_TEXT, 4,11,63,11, DIF_BOXCOLOR | DIF_SEPARATOR);

	//_i_background = _di.Add(DI_BUTTON, 5,12,25,12, DIF_CENTERGROUP, MBackground);
	_i_pause_resume = _di.Add(DI_BUTTON, 30,12,45,12, DIF_CENTERGROUP, MPause); // MResume
	_i_cancel = _di.Add(DI_BUTTON, 48,12,60,12, DIF_CENTERGROUP, MCancel, nullptr, FDIS_DEFAULT);

}

void XferProgress::Show()
{
	_finished = false;
	do {
		BaseDialog::Show(_di[_i_dblbox].Data, 6, 2);
	} while (!_finished);
}

LONG_PTR XferProgress::DlgProc(HANDLE dlg, int msg, int param1, LONG_PTR param2)
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

void XferProgress::OnIdle(HANDLE dlg)
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
