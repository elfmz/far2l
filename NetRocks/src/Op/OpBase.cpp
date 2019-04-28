#include "OpBase.h"

OpBase::OpBase(std::shared_ptr<SiteConnection> connection, int op_mode, const std::string &base_dir)
	:
	_connection(connection),
	_op_mode(op_mode),
	_base_dir(base_dir)
{
}


void *OpBase::ThreadProc()
{
	void *out = nullptr;
	try {
		_state.Reset();
		Process();
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
