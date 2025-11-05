#include <utils.h>
#include <stdexcept>
#include <TimeUtils.h>
#include "SimpleOperationProgress.h"
#include "Confirm.h"
#include "AbortOperationRequest.h"
#include "../../lng.h"
#include "../../Globals.h"

/*
345               21      29    35      43   48
 ========== Creating directory ...=============
|[TEXTBOX                                     ]|
|----------------------------------------------|
|             [   &Abort   ]                   |
 ==============================================
  5              20   25   30             45 48
*/

SimpleOperationProgress::SimpleOperationProgress(Kind kind, const std::string &object, ProgressState &state)
	: _kind(kind), _state(state)
{
	unsigned int title_lng;
	switch (kind) {
		case K_CONNECT: title_lng = MConnectProgressTitle; break;
		case K_GETMODE: title_lng = MGetModeProgressTitle; break;
		case K_CHANGEMODE: title_lng = MChangeModeProgressTitle; break;
		case K_ENUMDIR: title_lng = MEnumDirProgressTitle; break;
		case K_CREATEDIR: title_lng = MCreateDirProgressTitle; break;
		case K_EXECUTE: title_lng = MExecuteProgressTitle; break;
		case K_GETLINK: title_lng = MGetLinkProgressTitle; break;
		default:
			throw std::runtime_error("Unexpected kind");
	}
	_i_dblbox = _di.SetBoxTitleItem(title_lng);
	_di.Add(DI_TEXT, 5,2,48,2, 0, object.c_str());
	// this separator used to display retries/skips count
	_i_errstats_separator = _di.Add(DI_TEXT, 4,3,49,3, DIF_BOXCOLOR | DIF_SEPARATOR);
	_di.Add(DI_BUTTON, 16,4,32,4, DIF_CENTERGROUP, MCancel);

	TextFromDialogControl(_i_dblbox, _title);
}

void SimpleOperationProgress::Show()
{
	while (!_state.finished) {
		_finished = 0;
		BaseDialog::Show(L"SimpleOperationProgress", 6, 2, FDLG_REGULARIDLE);
		if (_finished) break;
		AbortOperationRequest(_state, _kind == K_CONNECT);
	}
}

LONG_PTR SimpleOperationProgress::DlgProc(int msg, int param1, LONG_PTR param2)
{
	uint64_t Colors[4];

	//fprintf(stderr, "%x %x\n", msg, param1);
	if (msg == DN_ENTERIDLE) {
		bool count_complete_changed = false, errors_changed = false;
		{
			std::lock_guard<std::mutex> locker(_state.mtx);
			if (_last_stats.count_complete != _state.stats.count_complete) {
				count_complete_changed = true;
			}

			if (_last_stats.count_retries != _state.stats.count_retries
			 || _last_stats.count_skips != _state.stats.count_skips) {
				errors_changed = true;
			}

			if (_state.finished && _finished == 0) {
				_finished = 1;
			}

			_last_stats = _state.stats;
		}

		if (errors_changed) {
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

//				DWORD color_flags = 0;
//				SendDlgMessage(DM_GETCOLOR, _i_errstats_separator, (LONG_PTR)&color_flags);
//				color_flags&= ~(FOREGROUND_GREEN | FOREGROUND_BLUE);
//				color_flags|= DIF_SETCOLOR | FOREGROUND_RED;
//				SendDlgMessage(DM_SETCOLOR, _i_errstats_separator, color_flags);

			}
		} else if (_errstats_colored) {
			_errstats_colored = false;

			Colors[0] = 0;
			SendDlgMessage(DM_SETCOLOR, _i_errstats_separator, (LONG_PTR)Colors);

//			SendDlgMessage(DM_SETCOLOR, _i_errstats_separator, 0);
		}


		if (_finished == 1) {
			_finished = 2;
			Close();

		} else if (count_complete_changed && _finished == 0) {
			std::string title = _title;
			char sz[64] = {};
			snprintf(sz, sizeof(sz) - 1, " (%llu)", _last_stats.count_complete);
			title+= sz;
			TextToDialogControl(_i_dblbox, title);
		}

	}/* else if (msg == DN_BTNCLICK && AbortConfirm().Ask()) {
		std::lock_guard<std::mutex> locker(_state.mtx);
		_state.aborting = true;
		return TRUE;
	}*/

	return BaseDialog::DlgProc(msg, param1, param2);
}
