#include <assert.h>
#include <vector>
#include <mutex>
#include "../WinPort/WinCompat.h"
#include "../WinPort/WinPort.h"

#include "InterThreadCall.hpp"

static struct InterThreadCallDelegates : std::vector<IInterThreadCallDelegate *> {} s_interlocked_delegates;
static std::mutex s_interlocked_delegates_mtx;
static volatile DWORD s_interlocked_delegates_tid = 0;

void StartDispatchingInterThreadCalls()
{
	std::lock_guard<std::mutex> lock(s_interlocked_delegates_mtx);
	s_interlocked_delegates_tid = WINPORT(GetCurrentThreadId)();
}

void StopDispatchingInterThreadCalls()
{
	assert((IsCurrentThreadDispatchesInterThreadCalls()));
	InterThreadCallDelegates interlocked_delegates;
	{
		std::lock_guard<std::mutex> lock(s_interlocked_delegates_mtx);
		s_interlocked_delegates_tid = 0;
		interlocked_delegates.swap(s_interlocked_delegates);
	}
	for (const auto &d : interlocked_delegates) {
		d->Process(true);
	}
}

bool IsCurrentThreadDispatchesInterThreadCalls()
{
	return LIKELY(s_interlocked_delegates_tid == WINPORT(GetCurrentThreadId)());
}

int DispatchInterThreadCalls()
{
	for (int dispatched_count = 0;;) {
		if (UNLIKELY(!IsCurrentThreadDispatchesInterThreadCalls()))
			return -1;

		InterThreadCallDelegates interlocked_delegates;
		{
			std::lock_guard<std::mutex> lock(s_interlocked_delegates_mtx);
			if (LIKELY(s_interlocked_delegates.empty())) {
				return dispatched_count;
			}
			interlocked_delegates.swap(s_interlocked_delegates);
		}
		for (const auto &d : interlocked_delegates) {
			d->Process(false);
		}
		dispatched_count+= (int)interlocked_delegates.size();
	}
}

bool EnqueueInterThreadCallDelegate(IInterThreadCallDelegate *d)
{
	std::lock_guard<std::mutex> lock(s_interlocked_delegates_mtx);
	if (UNLIKELY(s_interlocked_delegates_tid == 0))
		return false;

	s_interlocked_delegates.emplace_back(d);
	if (s_interlocked_delegates.size() == 1) {
		// write dummy console input to kick pending ReadConsoleInput
		INPUT_RECORD ir = {};
		ir.EventType = NOOP_EVENT;
		DWORD dw = 0;
		WINPORT(WriteConsoleInput)(0, &ir, 1, &dw);
	}
	return true;
}
