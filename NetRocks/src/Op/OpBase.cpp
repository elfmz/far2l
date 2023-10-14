#include <utils.h>
#include <sudo.h>
#include "OpBase.h"
#include "../Globals.h"
#include "../lng.h"

OpBase::OpBase(int op_mode, std::shared_ptr<IHost> base_host, const std::string &base_dir, std::shared_ptr<WhatOnErrorState> wea_state)
	:
	_wea_state(wea_state),
	_op_mode(op_mode),
	_base_host(base_host),
	_base_dir(base_dir)
{
	_state.ao_host = this;
}

OpBase::~OpBase()
{
}

void OpBase::SetNotifyTitle(int title_lng)
{
	_notify_title_lng = title_lng;
}

void OpBase::ResetProgressState()
{
	std::lock_guard<std::mutex> locker(_state.mtx);
	_state.stats = ProgressStateStats();
	_state.path.clear();
	_state.paused = _state.aborting = _state.finished = false;
}

static void OpBaseExceptionMessage(const std::wstring &what)
{
	// split long string into two (for now) parts if appropriate
	size_t div_pos = what.rfind(L": "), div_len = 2;
	if (div_pos == std::string::npos) {
		div_pos = what.rfind(L" - ");
		div_len = 3;
	}
	if (what.size() > 20 && div_pos != std::string::npos && div_pos != 0 && div_pos + div_len < what.size()) {
		const auto &what1 = what.substr(0, div_pos);
		for (div_pos+= div_len; div_pos < what.size() && what[div_pos] == ' '; ++div_pos) {
		}
		const auto &what2 = what.substr(div_pos);
		const wchar_t *msg[] = { G.GetMsgWide(MOperationFailed), what1.c_str(), what2.c_str(), G.GetMsgWide(MOK)};
		G.info.Message(G.info.ModuleNumber, FMSG_WARNING, nullptr, msg, ARRAYSIZE(msg), 1);
	} else {
		const wchar_t *msg[] = { G.GetMsgWide(MOperationFailed), what.c_str(), G.GetMsgWide(MOK)};
		G.info.Message(G.info.ModuleNumber, FMSG_WARNING, nullptr, msg, ARRAYSIZE(msg), 1);
	}
}

void *OpBase::ThreadProc()
{
	void *out = this;
	try {
		ResetProgressState();
		SudoClientRegion sdc_region;
		Process();
		out = nullptr;

		std::lock_guard<std::mutex> locker(_state.mtx);
		fprintf(stderr,
			"NetRocks::OpBase('%s'): count=%llu all_total=%llu\n",
			_base_dir.c_str(), _state.stats.count_total, _state.stats.all_total);

	} catch (AbortError &) {
		fprintf(stderr, "NetRocks::OpBase('%s'): aborted\n", _base_dir.c_str());

	} catch (std::exception &e) {
		fprintf(stderr, "NetRocks::OpBase('%s'): ERROR='%s'\n", _base_dir.c_str(), e.what());

		bool aborting;
		{
			std::lock_guard<std::mutex> locker(_state.mtx);
			aborting = _state.aborting;
		}
		if (!aborting) {
			const std::wstring &what = MB2Wide(e.what());
			OpBaseExceptionMessage(what);
		}
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

	// calling DisplayNotification from here cuz calling it from thread causes deadlock due to
	// InterThreadCall waits for main thread while main thread waits for thread's completion
	if (_notify_title_lng != -1 && !IS_SILENT(_op_mode)
	 && G.GetGlobalConfigBool("EnableDesktopNotifications", true)) {
		std::wstring display_action = G.GetMsgWide((out == nullptr) ? MNotificationSuccess : MNotificationFailed);
		size_t p = display_action.find(L"()");
		if (p != std::wstring::npos)
			display_action.replace(p, 2, G.GetMsgWide(_notify_title_lng));

		G.info.FSF->DisplayNotification(display_action.c_str(), StrMB2Wide(_base_dir).c_str());
	}

	return out;
}

void OpBase::ForcefullyAbort()
{
	fprintf(stderr, "NetRocks::OpBase('%s')::ForcefullyAbort()\n", _base_dir.c_str());

	{
		std::lock_guard<std::mutex> locker(_state.mtx);
		_state.aborting = true;
	}

	_base_host->Abort();
}

bool OpBase::WaitThread(unsigned int msec)
{
	for (unsigned int sleep_limit = 200;;) {
		unsigned int interval = (msec > sleep_limit) ? sleep_limit : msec;
		if (Threaded::WaitThread(interval))
			return true;

		int cow_result = G.info.FSF->DispatchInterThreadCalls();

		if (msec != (unsigned int)-1) {
			msec-= interval;
			if (msec == 0) {
				return false;
			}
		}
		if (cow_result != 0) {
			fprintf(stderr, "NetRocks::OpBase('%s', %d): DispatchInterThreadCalls returned %d\n", _base_dir.c_str(), _notify_title_lng, cow_result);
			if (cow_result > 0) {
				sleep_limit = 1;
			}
		} else {
			sleep_limit = 500;
		}
	}
}

bool OpBase::WaitThreadBeforeShowProgress()
{
	if (WaitThread(IS_SILENT(_op_mode) ? 2000 : 500)) {
		return true;
	}

	// avoid annoying popping up of progress dialog while reading error dialog
	while (_wea_state->IsShowingUIRightNow()) {
		if (WaitThread(100)) {
			return true;
		}
	}

	return false;
}
