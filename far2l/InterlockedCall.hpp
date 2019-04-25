#pragma once
#include <mutex>
#include <condition_variable>
#include <luck.h>

struct IInterlockedCallDelegate
{
	virtual void Process(bool stopping) = 0;
};

void StartDispatchingInterlockedCalls();
void StopDispatchingInterlockedCalls();
bool IsCurrentThreadDispatchesInterlockedCalls();
void DispatchInterlockedCalls();

bool EnqueueInterlockedCallDelegate(IInterlockedCallDelegate *d);


template <class RV, class FN>
	class InterlockedCallDelegate : protected IInterlockedCallDelegate
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
	inline InterlockedCallDelegate(FN fn):_fn(fn) { }

	bool Do()
	{
		_done = _discarded = false;

		if (UNLIKELY(!EnqueueInterlockedCallDelegate(this)))
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
	static RV InterlockedCall(FN fn)
{
	if (LIKELY(IsCurrentThreadDispatchesInterlockedCalls())) {
		return fn();
	}

	InterlockedCallDelegate<RV, FN> c(fn);
	if (LIKELY(c.Do())) {
		return c.Result();
	}

	return FAIL_RV;
}
