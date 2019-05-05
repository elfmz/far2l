#include <utils.h>
#include "OpBase.h"
#include "../Globals.h"
#include "../lng.h"

OpBase::OpBase(std::shared_ptr<IHost> base_host, int op_mode, const std::string &base_dir, int op_name_lng)
	:
	_base_host(base_host),
	_op_mode(op_mode),
	_base_dir(base_dir),
	_op_name_lng(op_name_lng)
{
	_state.ao_host = this;
	_succeded = false;
}

OpBase::~OpBase()
{
	// calling DisplayNotification from here cuz calling it from thread causes deadlock due to
	// InterlockedCall waits for main thread while main thread waits for thread's completion
	if (_op_name_lng != -1) {
		std::wstring display_action = G.GetMsgWide(_succeded ? MNotificationSuccess : MNotificationFailed);
		size_t p = display_action.find(L"()");
		if (p != std::wstring::npos)
			display_action.replace(p, 2, G.GetMsgWide(_op_name_lng));

		G.info.FSF->DisplayNotification(display_action.c_str(), StrMB2Wide(_base_dir).c_str());
	}

}


void *OpBase::ThreadProc()
{
	void *out = nullptr;
	try {
		_state.Reset();
		Process();
		_succeded = true;
		fprintf(stderr,
			"NetRocks::OpBase('%s'): count=%llu all_total=%llu\n",
			_base_dir.c_str(), _state.stats.count_total, _state.stats.all_total);

	} catch (std::exception &e) {
		fprintf(stderr, "NetRocks::OpBase('%s'): ERROR='%s'\n", _base_dir.c_str(), e.what());
		out = this;
	}
	{
		std::lock_guard<std::mutex> locker(_state.mtx);
		_state.finished = true;
	}

	// NOOP_EVENT -> KEY_IDLE -> DN_ENTERIDLE
	INPUT_RECORD ir = {};
	ir.EventType = NOOP_EVENT;
	DWORD dw = 0;
	WINPORT(WriteConsoleInput)(0, &ir, 1, &dw);

	return out;
}

void OpBase::ForcefullyAbort()
{
	_base_host->Abort();
}

bool OpBase::WaitThread(unsigned int msec)
{
	for (;;) {
		unsigned int interval = (msec > 500) ? 500 : msec;
		if (Threaded::WaitThread(interval))
			return true;

		if (msec != (unsigned int)-1) {
			msec-= interval;
			if (msec == 0)
				return false;
		}
		int cow_result = G.info.FSF->CallOnWait();
		if (cow_result != 0) {
			fprintf(stderr, "NetRocks::OpBase('%s', %d): CallOnWait returned %d\n", _base_dir.c_str(), _op_name_lng, cow_result);
		}
	}
}
