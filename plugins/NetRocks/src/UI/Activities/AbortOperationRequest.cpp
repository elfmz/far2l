#include <utils.h>
#include "../Defs.h"
#include "../DialogUtils.h"
#include "../../lng.h"
#include "../../Globals.h"


/*
345                                            50
 ============= Abort operation ================
| Confirm abort current operation              |
|----------------------------------------------|
|   [ &Abort operation  ]    [ &Continue ]     |
 ==============================================
    6                   27   32           45
*/


class AbortConfirm : protected BaseDialog
{
	int _i_dblbox, _i_confirm, _i_cancel;

	LONG_PTR DlgProc(int msg, int param1, LONG_PTR param2)
	{
		if (msg == DM_KEY && param2 == 0x1b) {
			// need to distinguish escape press and exit due to far going down
			// to avoid infinite loop at far shutdown
			Close(_i_cancel);
			return TRUE;
		}
		return BaseDialog::DlgProc(msg, param1, param2);
	}

public:
	AbortConfirm()
	{
		_i_dblbox = _di.SetBoxTitleItem(MAbortTitle);
		_di.Add(DI_TEXT, 5,2,48,2, DIF_CENTERGROUP, MAbortText);
		_di.Add(DI_TEXT, 4,3,49,3, DIF_BOXCOLOR | DIF_SEPARATOR);

		_i_confirm = _di.Add(DI_BUTTON, 6,4,27,4, DIF_CENTERGROUP, MAbortConfirm);
		_i_cancel = _di.Add(DI_BUTTON, 32,4,46,4, DIF_CENTERGROUP, MAbortNotConfirm);

		SetFocusedDialogControl();
		SetDefaultDialogControl();
	}

	bool Ask()
	{
		int reply = Show(L"AbortConfirm", 6, 2, FDLG_WARNING);
		return (reply == _i_confirm || reply < 0);
	}
};


///////////////////////////////////////////////////////////

class AbortOperationProgress : protected BaseDialog
{
	ProgressState &_state;
	const time_t _ts;
	int _i_dblbox;
	std::string _title;
	bool _finished = false;

protected:
	LONG_PTR DlgProc(int msg, int param1, LONG_PTR param2)
	{
		if (msg == DN_ENTERIDLE) {
			if (_state.finished) {
				if (!_finished) {
					_finished = true;
					Close();
				}
			} else {
				const time_t secs = time(NULL) - _ts;
				if (secs) {
					char sz[0x200];
					snprintf(sz, ARRAYSIZE(sz) - 1, "%s (%lus)", _title.c_str(), (unsigned long)secs);
					TextToDialogControl(_i_dblbox, sz);
					if (secs == 60) {
						_finished = true;
						Close();
					}
				}
			}
		}
		return BaseDialog::DlgProc(msg, param1, param2);
	}

public:
	AbortOperationProgress(ProgressState &state)
		: _state(state), _ts(time(NULL))
	{
		_i_dblbox = _di.SetBoxTitleItem(MAbortingOperationTitle);
		_di.Add(DI_BUTTON, 5,2,48,2, DIF_CENTERGROUP, MBreakConnection);
		TextFromDialogControl(_i_dblbox, _title);
	}

	void Show()
	{
		_finished = false;
		while (!_state.finished) {
			if (BaseDialog::Show(L"AbortOperationProgress", 6, 2, FDLG_REGULARIDLE) == -2) {
				// seems far2l goes down, dont use all CPU in this loop
				// and dispatch interlocked thread calls to avoid deadlock
				// if this is main thread and other threads try to perform
				// interlocked operations
				usleep(1000);
				G.info.FSF->DispatchInterThreadCalls();
			}
			if (_state.finished) break;
			if (_state.ao_host) {
				_state.ao_host->ForcefullyAbort();
			} else {
				fprintf(stderr, "NetRocks::AbortOperationProgress: no ao_host\n");
			}
		}
	}
};


void AbortOperationRequest(ProgressState &state, bool force_immediate)
{
	bool saved_paused;
	{ // pause opetation while showing UI abort confirmation
		std::lock_guard<std::mutex> locker(state.mtx);
		if (state.aborting)
			return;
		saved_paused = state.paused;
		state.paused = true;
	}
	if (!AbortConfirm().Ask()) { //param1 == _i_cancel &&
		std::lock_guard<std::mutex> locker(state.mtx);
		state.paused = saved_paused;
		return;
	}
	{
		std::lock_guard<std::mutex> locker(state.mtx);
		state.aborting = true;
		state.paused = saved_paused;
	}
	if (force_immediate && state.ao_host) {
		state.ao_host->ForcefullyAbort();
	}
	AbortOperationProgress(state).Show();
}


