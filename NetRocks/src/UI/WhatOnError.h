#pragma once
#include <string>
#include <map>
#include <mutex>
#include <sys/time.h>
#include <windows.h>
#include "DialogUtils.h"
#include "Defs.h"
#include "../Host/Host.h"

class WhatOnError : protected BaseDialog
{
	int _i_remember = -1, _i_retry = -1, _i_skip = -1;

public:
	WhatOnError(WhatOnErrorKind wek, const std::string &error, const std::string &object, const std::string &site);

	WhatOnErrorAction Ask(WhatOnErrorAction &default_wea);
};

class WhatOnErrorState
{
	std::map<std::string, WhatOnErrorAction> _default_weas[WEKS_COUNT];

	public:
	WhatOnErrorAction Query(WhatOnErrorKind wek, const std::string &error, const std::string &object, const std::string &site);
};

void WhatOnErrorWrap_DummyOnRetry();

//void (IT::*METH)(...)
template <WhatOnErrorKind WEK, class FN, class FN_ON_RETRY = void(*)()>
	static void WhatOnErrorWrap(WhatOnErrorState &wea_state,
		ProgressState &progress_state, IHost *indicted, const std::string &object, FN fn, FN_ON_RETRY fn_on_retry = WhatOnErrorWrap_DummyOnRetry)
{
	for (bool retried = false;;) try {
		fn();
		break;

	} catch (AbortError &) {
		throw;

	} catch (std::exception &ex) {
		switch (wea_state.Query(WEK, ex.what(), object, indicted->SiteName())) {
			case WEA_SKIP: {
				std::unique_lock<std::mutex> locker(progress_state.mtx);
				progress_state.stats.count_skips++;
			} return;

			case WEA_RETRY: {
				if (retried) {
					sleep(1);
				} else {
					retried = true;
				}
				indicted->ReInitialize();
				fn_on_retry();
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
