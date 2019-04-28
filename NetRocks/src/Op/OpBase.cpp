#include "OpBase.h"
#include "../Globals.h"
#include "../lng.h"

OpBase::OpBase(std::shared_ptr<SiteConnection> connection, int op_mode, const std::string &base_dir, int op_name_lng)
	:
	_connection(connection),
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
		std::string display_action = G.GetMsg(_succeded ? MNotificationSuccess : MNotificationFailed);
		size_t p = display_action.find("{}");
		if (p != std::string::npos)
			display_action.replace(p, 2, G.GetMsg(_op_name_lng));

		G.info.FSF->DisplayNotification(display_action.c_str(), _base_dir.c_str());
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
	_connection->Abort();
}
