#include "XferProgress.h"
#include "../Globals.h"


/*                                                         62
345                  24       33      41 44        54  58  60  64
 ====================== Download =============================
| Current file:                                              |
| [EDITBOX                                                 ] |
| Current file progress:       [====================]  ###%  |
| Total progress:              [====================]  ###%  |
| Total files:    #########   Transferred files:   ######### |
| Total size:     #########   Transferred size:    ######### |
| Current file time  SPENT:   ###:##.##  REMAIN:   ###:##.## |
| Total time         SPENT:   ###:##.##  REMAIN:   ###:##.## |
| Speed (?bps)     CURRENT:   [TEXT   ]  AVERAGE:  [TEXT   ] |
|------------------------------------------------------------|
| [    &Background    ]    [    &Pause    ]  [   &Cancel   ] |
 =============================================================
  5             d     25   30             45 48            60
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
		_di.Add(DI_DOUBLEBOX, 3,1,64,12, 0, (xd == XK_DOWNLOAD) ? MXferCopyDownloadTitle : MXferCopyUploadTitle);
		_di.Add(DI_TEXT, 5,2,62,2, 0, (xd == XK_DOWNLOAD) ? MXferCopyDownloadText : MXferCopyUploadText);
	} else {
		_di.Add(DI_DOUBLEBOX, 3,1,64,12, 0, (xd == XK_DOWNLOAD) ? MXferMoveDownloadTitle : MXferMoveUploadTitle);
		_di.Add(DI_TEXT, 5,2,62,2, 0, (xd == XK_DOWNLOAD) ? MXferMoveDownloadText : MXferMoveUploadText);
	}

	_di.Add(DI_TEXT, 5,3,62,3, 0, MXferCurrentFile);
	_i_cur_file = _di.Add(DI_EDIT, 5,4,62,4, DIF_READONLY, "...");

	_di.Add(DI_TEXT, 5,5,33,5, 0, MXferCurrentProgress);
	_i_cur_file_progress_bar = _di.Add(DI_TEXT, 34,5,55,5, 0, "[                    ]");
	_i_cur_file_progress_perc = _di.Add(DI_TEXT, 58,5,61,5, 0, "???%");

	_di.Add(DI_TEXT, 5,6,33,6, 0, MXferTotalProgress);
	_i_total_progress_bar = _di.Add(DI_TEXT, 34,5,55,5, 0, "[                    ]");
	_i_total_progress_perc = _di.Add(DI_TEXT, 58,5,61,5, 0, "???%");

	_di.Add(DI_TEXT, 5,7,20,7, 0, MXferTotalCount);
	_i_total_count = _di.Add(DI_TEXT, 21,7,29,7, 0, "##########");
	_di.Add(DI_TEXT, 33,7,53,7, 0, MXferCurrentCount);
	_i_current_count = _di.Add(DI_TEXT, 54,7,60,7, 0, "##########");

	_di.Add(DI_TEXT, 5,8,20,8, 0, MXferTotalSize);
	_i_total_size = _di.Add(DI_TEXT, 21,8,29,8, 0, "##########");
	_di.Add(DI_TEXT, 33,8,53,8, 0, MXferCurrentSize);
	_i_current_size = _di.Add(DI_TEXT, 54,8,60,8, 0, "##########");

	_di.Add(DI_TEXT, 5,9,32,9, 0, MXferCurrentTimeSpent);
	_i_current_time_spent = _di.Add(DI_TEXT, 33,9,41,9, 0, "???:??.??");
	_di.Add(DI_TEXT, 44,9,53,9, 0, MXferRemain);
	_i_current_time_remain = _di.Add(DI_TEXT, 54,9,60,9, 0, "???:??.??");

	_di.Add(DI_TEXT, 5,10,32,10, 0, MXferTotalTimeSpent);
	_i_total_time_spent = _di.Add(DI_TEXT, 33,10,41,10, 0, "???:??.??");
	_di.Add(DI_TEXT, 44,10,53,10, 0, MXferRemain);
	_i_total_time_remain = _di.Add(DI_TEXT, 54,10,60,10, 0, "???:??.??");

	_i_speed_current_label = _di.Add(DI_TEXT, 5,11,32,11, 0, MXferSpeedCurrent);
	_i_speed_current = _di.Add(DI_TEXT, 33,11,41,11, 0, "???");
	_di.Add(DI_TEXT, 44,11,53,11, 0, MXferAverage);
	_i_total_time_remain = _di.Add(DI_TEXT, 54,11,60,11, 0, "???");

	_di.Add(DI_TEXT, 4,12,63,12, DIF_BOXCOLOR | DIF_SEPARATOR);

	//_i_background = _di.Add(DI_BUTTON, 5,13,25,13, DIF_CENTERGROUP, MBackground);
	_i_pause_resume = _di.Add(DI_BUTTON, 30,13,45,13, DIF_CENTERGROUP, MPause); // MResume
	_i_cancel = _di.Add(DI_BUTTON, 48,13,60,13, DIF_CENTERGROUP, MCancel, nullptr, FDIS_DEFAULT);

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
				std::lock_guard<std::mutex> locker(_state.lock);
				paused = (_state.paused = !_state.paused);
			}
			TextToDialogControl(dlg, _i_pause_resume, paused ? MResume : MPause);

		} else if (param1 == _i_cancel) {
			std::lock_guard<std::mutex> locker(_state.lock);
			_state.aborting = true;
		}
		return TRUE;
	}
	
	return BaseDialog::DlgProc(dlg, msg, param1, param2);
}

void XferProgress::OnIdle(HANDLE dlg)
{
	struct {
		bool name, total_count, current_count, total_size, current_size;
	} changed = {};

	bool finished = false;

	{
		std::lock_guard<std::mutex> locker(_state.lock);
		if (_state.name != nullptr && _last_name != *_state.name) {
			_last_name = *_state.name;
			changed.name = true;
		}
		if (_state.total_count) {
			_last_stats.total_count = _state.total_count;
			changed.total_count = true;
		}
		if (_state.current_count) {
			_last_stats.current_count = _state.current_count;
			changed.current_count = true;
		}
		if (_state.total_size) {
			_last_stats.total_size = _state.total_size;
			changed.total_size = true;
		}
		if (_state.current_size) {
			_last_stats.current_size = _state.current_size;
			changed.current_size = true;
		}
		finished = _state.finished;
	}

	if (changed.name) {
		TextToDialogControl(dlg, _i_cur_file, _last_name);
	}

	if (changed.total_count) {
		LongLongToDialogControl(dlg, _i_total_count, _last_stats.total_count);
	}

	if (changed.current_count) {
		LongLongToDialogControl(dlg, _i_current_count, _last_stats.current_count);
	}

	if (changed.total_size) {
		LongLongToDialogControl(dlg, _i_total_size, _last_stats.total_size);
	}

	if (changed.current_size) {
		LongLongToDialogControl(dlg, _i_current_size, _last_stats.current_size);
	}

	if (finished && !_finished) {
		_finished = true;
		Close(dlg);
	}
}
