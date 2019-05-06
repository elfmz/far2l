#include <utils.h>
#include "Defs.h"
#include "DialogUtils.h"
#include "../lng.h"


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
		_i_dblbox = _di.Add(DI_DOUBLEBOX, 3,1,50,5, 0, MAbortTitle);
		_di.Add(DI_TEXT, 5,2,48,2, DIF_CENTERGROUP, MAbortText);
		_di.Add(DI_TEXT, 4,3,49,3, DIF_BOXCOLOR | DIF_SEPARATOR);

		_i_confirm = _di.Add(DI_BUTTON, 6,4,27,4, DIF_CENTERGROUP, MAbortConfirm, nullptr, FDIS_DEFAULT);
		_i_cancel = _di.Add(DI_BUTTON, 32,4,46,4, DIF_CENTERGROUP, MAbortNotConfirm);
	}

	bool Ask()
	{
		int reply = Show(L"AbortConfirm", 6, 2, FDLG_WARNING);
		return (reply == _i_confirm || reply == -1);
	}
};


///////////////////////////////////////////////////////////

class AbortOperationProgress : protected BaseDialog
{
	ProgressState &_state;
	const time_t _ts;
	int _i_dblbox;
	std::string _title;
	bool _finished;

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
		_i_dblbox = _di.Add(DI_DOUBLEBOX, 3, 1, 50, 3, 0, MAbortingOperationTitle);
		_di.Add(DI_BUTTON, 5,2,48,2, DIF_CENTERGROUP, MBreakConnection);
		_title = Wide2MB(_di[_i_dblbox].PtrData);
	}

	void Show()
	{
		_finished = false;
		while (!_state.finished) {
			BaseDialog::Show(L"AbortOperationProgress", 6, 2, FDLG_REGULARIDLE);
			if (_state.finished) break;
			if (_state.ao_host) {
				_state.ao_host->ForcefullyAbort();
			} else {
				fprintf(stderr, "NetRocks::AbortOperationProgress: no ao_host\n");
			}
		}
	}
};


void AbortOperationRequest(ProgressState &state)
{
	if (state.aborting)
		return;

	bool saved_paused;
	{ // pause opetation while showing UI abort confirmation
		std::lock_guard<std::mutex> locker(state.mtx);
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
	AbortOperationProgress(state).Show();
}


