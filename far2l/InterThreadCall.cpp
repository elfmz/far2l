#include <assert.h>
#include <vector>
#include <mutex>
#include "../WinPort/WinCompat.h"
#include "../WinPort/WinPort.h"

#include "InterThreadCall.hpp"

static struct InterThreadCallSynch
{
	std::condition_variable cond;
	std::mutex mtx;
} s_inter_thread_call_synch;


static struct InterThreadCallDelegates : std::vector<IInterThreadCallDelegate *> {} s_interlocked_delegates;
static volatile DWORD s_interlocked_delegates_tid = 0;

std::condition_variable g_interlocked_thread_call_cond;

void StartDispatchingInterThreadCalls()
{
	const DWORD self_tid = WINPORT(GetCurrentThreadId)();
	std::lock_guard<std::mutex> lock(s_inter_thread_call_synch.mtx);
	s_interlocked_delegates_tid = self_tid;
}

void StopDispatchingInterThreadCalls()
{
	assert((IsCurrentThreadDispatchesInterThreadCalls()));
	InterThreadCallDelegates interlocked_delegates;
	{
		std::lock_guard<std::mutex> lock(s_inter_thread_call_synch.mtx);
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
			std::lock_guard<std::mutex> lock(s_inter_thread_call_synch.mtx);
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
	std::lock_guard<std::mutex> lock(s_inter_thread_call_synch.mtx);
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

	s_inter_thread_call_synch.cond.notify_all();
	return true;
}

////////////////
InterThreadLock::InterThreadLock()
	: _locker(s_inter_thread_call_synch.mtx)
{
}

void InterThreadLock::WaitForWake()
{
	s_inter_thread_call_synch.cond.wait_for(_locker, std::chrono::milliseconds(5000));
}

InterThreadLockAndWake::~InterThreadLockAndWake()
{
	s_inter_thread_call_synch.cond.notify_all();
}

