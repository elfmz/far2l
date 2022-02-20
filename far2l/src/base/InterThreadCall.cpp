#include <assert.h>
#include <cctweaks.h>
#include <vector>
#include <mutex>
#include <atomic>
#include <WinCompat.h>
#include <WinPort.h>

#include "InterThreadCall.hpp"

static struct InterThreadCallSynch
{
	std::condition_variable cond;
	std::mutex mtx;
} s_inter_thread_call_synch;


static struct InterThreadCallDelegates : std::vector<IInterThreadCallDelegate *> {} s_interlocked_delegates;
static volatile DWORD s_interlocked_delegates_tid = 0;

std::condition_variable g_interlocked_thread_call_cond;

static std::atomic<unsigned int> g_thread_id_counter{0};

thread_local unsigned int g_thread_id;

static unsigned int FN_NOINLINE GenerateThreadID()
{
	unsigned int tid = ++g_thread_id_counter;
	while (UNLIKELY(tid == 0)) {
		// assuming all threads after ~0x10000-th are temporary ones..
		g_thread_id_counter+= 0x10000 + (rand() % 0x100);
		tid = ++g_thread_id_counter;
		fprintf(stderr, "GenerateThreadID: wrapped around, reset to %u\n", tid);
	}
	return tid;
}

unsigned int GetInterThreadID()
{
	unsigned int tid = g_thread_id;
	if (UNLIKELY(tid == 0)) {
		tid = GenerateThreadID();
		g_thread_id = tid;
	}
	return tid;
}

void OverrideInterThreadID(unsigned int tid)
{
	g_thread_id = tid;
}

void InterThreadSyncCallDelegate::Finalize(bool discarded)
{
	std::lock_guard<std::mutex> lock(_mtx);
	_discarded = discarded;
	_done = true;
	_cond.notify_all();
}

bool InterThreadSyncCallDelegate::Do()
{
	_done = _discarded = false;

	if (UNLIKELY(!EnqueueInterThreadCallDelegate(this)))
		return false;

	std::unique_lock<std::mutex> lock(_mtx);
	while (!_done) {
		_cond.wait(lock);
	}

	return !_discarded;
}

InterThreadCallsDispatcherThread::InterThreadCallsDispatcherThread()
{
	const DWORD self_tid = GetInterThreadID();
	std::lock_guard<std::mutex> lock(s_inter_thread_call_synch.mtx);
	s_interlocked_delegates_tid = self_tid;
}

InterThreadCallsDispatcherThread::~InterThreadCallsDispatcherThread()
{
	assert((IsCurrentThreadDispatchesInterThreadCalls()));
	InterThreadCallDelegates interlocked_delegates;
	{
		std::lock_guard<std::mutex> lock(s_inter_thread_call_synch.mtx);
		s_interlocked_delegates_tid = 0;
		interlocked_delegates.swap(s_interlocked_delegates);
	}
	for (const auto &d : interlocked_delegates) {
		try {
			d->Finalize(true);
		} catch (std::exception &e) {
			fprintf(stderr, "%s - Finalize: %s\n", __FUNCTION__, e.what());
		}
	}
}

bool IsCurrentThreadDispatchesInterThreadCalls()
{
	return LIKELY(s_interlocked_delegates_tid == GetInterThreadID());
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
			bool failed = false;
			try {
				d->Process();
			} catch (std::exception &e) {
				fprintf(stderr, "%s - Process: %s\n", __FUNCTION__, e.what());
				failed = true;
			}
			try {
				d->Finalize(failed);
			} catch (std::exception &e) {
				fprintf(stderr, "%s - Finalize: %s\n", __FUNCTION__, e.what());
			}
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

