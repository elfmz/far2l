#pragma once
#include <string>
#include <map>
#include <mutex>
#include <atomic>
#include <sys/time.h>
#include <windows.h>
#include "../DialogUtils.h"
#include "../Defs.h"
#include "../../Host/Host.h"

class WhatOnError : protected BaseDialog
{
	int _i_remember = -1, _i_recovery = -1, _i_retry = -1, _i_skip = -1;

public:
	WhatOnError(WhatOnErrorKind wek, const std::string &error, const std::string &object, const std::string &site, bool may_recovery = false);

	WhatOnErrorAction Ask(WhatOnErrorAction &default_wea);
};

class WhatOnErrorState
{
	std::map<std::string, WhatOnErrorAction> _default_weas[WEKS_COUNT];
	unsigned int _auto_retry_delay = 0;
	std::atomic<int> _showing_ui{0};
	std::atomic<bool> _has_any_autoaction{false};
	std::mutex _mtx;

	public:
	WhatOnErrorAction Query(ProgressState &progress_state, WhatOnErrorKind wek, const std::string &error, const std::string &object, const std::string &site, bool may_recovery = false);
	void ResetAutoRetryDelay();

	bool IsShowingUIRightNow() const { return _showing_ui != 0; };

	bool HasAnyAutoAction() const;
	void ResetAutoActions();
};

void WhatOnErrorWrap_DummyOnRetry(bool &recovery);

//void (IT::*METH)(...)
template <WhatOnErrorKind WEK, class FN, class FN_ON_RETRY = void(*)(bool &)>
	static void WhatOnErrorWrap(std::shared_ptr<WhatOnErrorState> &wea_state,
		ProgressState &progress_state, IHost *indicted, const std::string &object, FN fn, FN_ON_RETRY fn_on_retry = WhatOnErrorWrap_DummyOnRetry, bool may_recovery = false)
{
	for (;;) try {
		fn();
		wea_state->ResetAutoRetryDelay();
		break;

	} catch (AbortError &) {
		throw;

	} catch (std::exception &ex) {
		auto wea = wea_state->Query(progress_state, WEK, ex.what(), object, indicted->SiteName(), may_recovery);
		switch (wea) {
			case WEA_SKIP: {
				std::unique_lock<std::mutex> locker(progress_state.mtx);
				progress_state.stats.count_skips++;
			} return;

			case WEA_RETRY:
				indicted->ReInitialize();

			case WEA_RECOVERY: {
				bool recovery = (wea == WEA_RECOVERY);
				fn_on_retry(recovery);
				if (!recovery && wea == WEA_RECOVERY) {
					may_recovery = false;
				}
				std::unique_lock<std::mutex> locker(progress_state.mtx);
				if (progress_state.aborting)
					throw AbortError();
				progress_state.stats.count_retries++;
			} break;

			default:
				throw AbortError(); // throw abort to avoid second error warning
		}
	}
}
