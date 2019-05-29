#pragma once
#include <functional>
#include <mutex>
#include <condition_variable>
#include <luck.h>

struct IInterThreadCallDelegate
{
	virtual void Process(bool stopping) = 0;
};

void StartDispatchingInterThreadCalls();
void StopDispatchingInterThreadCalls();
bool IsCurrentThreadDispatchesInterThreadCalls();
int DispatchInterThreadCalls();

bool EnqueueInterThreadCallDelegate(IInterThreadCallDelegate *d);

template <class RV, class FN>
	class InterThreadCallDelegate : protected IInterThreadCallDelegate
{
	std::mutex _mtx;
	std::condition_variable _cond;
	bool _done, _discarded;
	FN _fn;
	RV _rv;

protected:
	virtual void Process(bool stopping)
	{
		if (LIKELY(!stopping))
			_rv = _fn();

		std::lock_guard<std::mutex> lock(_mtx);
		_discarded = stopping;
		_done = true;
		_cond.notify_all();
	}

public:
	inline InterThreadCallDelegate(FN fn):_fn(fn) { }

	bool Do()
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

	inline RV Result() const
	{
		return _rv;
	}
};


////////

template <class RV, RV FAIL_RV = (RV)0, class FN>
	static RV InterThreadCall(FN fn)
{
	if (LIKELY(IsCurrentThreadDispatchesInterThreadCalls())) {
		return fn();
	}

	InterThreadCallDelegate<RV, FN> c(fn);
	if (LIKELY(c.Do())) {
		return c.Result();
	}

	return FAIL_RV;
}


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
