#pragma once
#include <functional>
#include <mutex>
#include <condition_variable>
#include <cctweaks.h>

struct IInterThreadCallDelegate
{
	virtual ~IInterThreadCallDelegate() {}
	virtual void Process() = 0;
	virtual void Finalize(bool discarded) = 0;
};

// similar to Windows GetCurrentThreadId() but with ability to be overridden
unsigned int GetInterThreadID();

// to be used in cases where two specific threads are known-sure
// to be possible run exclusively - either one, either another
void OverrideInterThreadID(unsigned int tid);

struct InterThreadCallsDispatcherThread
{
	InterThreadCallsDispatcherThread();
	~InterThreadCallsDispatcherThread();
};

bool IsCurrentThreadDispatchesInterThreadCalls();
int DispatchInterThreadCalls();

bool EnqueueInterThreadCallDelegate(IInterThreadCallDelegate *d);

class InterThreadSyncCallDelegate : public IInterThreadCallDelegate
{
protected:
	std::mutex _mtx;
	std::condition_variable _cond;
	bool _done, _discarded;

public:
	virtual void Finalize(bool discarded);
	bool Do();
};

template <class RV, class FN>
	class InterThreadSyncCallDelegateImpl : public InterThreadSyncCallDelegate
{
	FN _fn;
	RV _rv;

protected:
	virtual void Process()
	{
		_rv = _fn();
	}

public:
	inline InterThreadSyncCallDelegateImpl(const FN &fn) : _fn(fn) { }

	inline const RV &Result() const
	{
		return _rv;
	}
};

template <class FN>
	class InterThreadAsyncCallDelegate : protected IInterThreadCallDelegate
{
	FN _fn;

protected:
	virtual void Process()
	{
		_fn();
	}

	virtual void Finalize(bool discarded)
	{
		delete this;
	}

public:
	inline InterThreadAsyncCallDelegate(const FN &fn) : _fn(fn) { }

	bool Enqueue()
	{
		if (UNLIKELY(!EnqueueInterThreadCallDelegate(this))) {
			delete this;
			return false;
		}

		return true;
	}
};


////////

template <class RV, RV FAIL_RV = (RV)0, class FN>
	static RV InterThreadCall(FN fn)
{
	if (LIKELY(IsCurrentThreadDispatchesInterThreadCalls())) {
		return fn();
	}

	InterThreadSyncCallDelegateImpl<RV, FN> c(fn);
	if (LIKELY(c.Do())) {
		return c.Result();
	}

	return FAIL_RV;
}

template <class FN>
	static void InterThreadCallAsync(FN fn)
{
	auto *c = new InterThreadAsyncCallDelegate<FN>(fn);
	c->Enqueue();
}

//////////////////////////////////////////////////


struct InterThreadLock
{
	InterThreadLock();
	void WaitForWake();

private:
	std::unique_lock<std::mutex> _locker;
};

struct InterThreadLockAndWake : InterThreadLock
{
	~InterThreadLockAndWake();
};

#define WAIT_FOR_AND_DISPATCH_INTER_THREAD_CALLS(condition) for(;;) {	\
	DispatchInterThreadCalls();	\
	InterThreadLock lock;	\
	if (condition) break;	\
	lock.WaitForWake();	\
}
