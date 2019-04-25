#include <assert.h>
#include <vector>
#include <mutex>
#include "../WinPort/WinCompat.h"
#include "../WinPort/WinPort.h"

#include "InterlockedCall.hpp"

static struct InterlockedCallDelegates : std::vector<IInterlockedCallDelegate *> {} s_interlocked_delegates;
static std::mutex s_interlocked_delegates_mtx;
static volatile DWORD s_interlocked_delegates_tid = 0;

void StartDispatchingInterlockedCalls()
{
	std::lock_guard<std::mutex> lock(s_interlocked_delegates_mtx);
	s_interlocked_delegates_tid = WINPORT(GetCurrentThreadId)();
}

void StopDispatchingInterlockedCalls()
{
	assert((IsCurrentThreadDispatchesInterlockedCalls()));
	InterlockedCallDelegates interlocked_delegates;
	{
		std::lock_guard<std::mutex> lock(s_interlocked_delegates_mtx);
		s_interlocked_delegates_tid = 0;
		interlocked_delegates.swap(s_interlocked_delegates);
	}
	for (const auto &d : interlocked_delegates) {
		d->Process(true);
	}
}

bool IsCurrentThreadDispatchesInterlockedCalls()
{
	return LIKELY(s_interlocked_delegates_tid == WINPORT(GetCurrentThreadId)());
}

void DispatchInterlockedCalls()
{
	while (LIKELY(IsCurrentThreadDispatchesInterlockedCalls())) {
		InterlockedCallDelegates interlocked_delegates;
		{
			std::lock_guard<std::mutex> lock(s_interlocked_delegates_mtx);
			if (LIKELY(s_interlocked_delegates.empty())) {
				break;
			}
			interlocked_delegates.swap(s_interlocked_delegates);
		}
		for (const auto &d : interlocked_delegates) {
			d->Process(false);
		}
	}
}

bool EnqueueInterlockedCallDelegate(IInterlockedCallDelegate *d)
{
	std::lock_guard<std::mutex> lock(s_interlocked_delegates_mtx);
	if (UNLIKELY(s_interlocked_delegates_tid == 0))
		return false;

	s_interlocked_delegates.emplace_back(d);
	if (s_interlocked_delegates.size() == 1) {
		// write some dummy console input to kick pending ReadConsoleInput
		INPUT_RECORD ir = {0};
		ir.EventType = FOCUS_EVENT;
		ir.Event.FocusEvent.bSetFocus = TRUE;
		DWORD dw = 0;
		WINPORT(WriteConsoleInput)(0, &ir, 1, &dw);
	}
	return true;
}
