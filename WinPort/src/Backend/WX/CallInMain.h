#pragma once
#include <mutex>
#include <condition_variable>
#include <functional>


wxEvtHandler *WinPort_EventHandler();

template <class FN>
	class InMainCallerBase
{
	std::mutex _mutex;
	std::condition_variable _cond;
	FN _fn;
	bool _done = false;

protected:
	virtual void Invoke(FN fn) = 0;

	void Callback()
	{
		try {
			Invoke(_fn);
		} catch (std::exception &e) {
			fprintf(stderr, "InMainCallerBase: '%s'\n", e.what());
		} catch (...) {
			fprintf(stderr, "InMainCallerBase: ...\n");
		}

		std::unique_lock<std::mutex> locker(_mutex);
		_done = true;
		_cond.notify_all();
	}

public:
	InMainCallerBase(FN fn):_fn(fn) { }

	void Do()
	{
		_done = false;
		WinPort_EventHandler()->CallAfter(std::bind(&InMainCallerBase::Callback, this));
		for (;;) {
			std::unique_lock<std::mutex> locker(_mutex);
			if (_done) break;
			_cond.wait(locker);
		}
	}
};

template <class RV, class FN>
	class InMainCaller : protected InMainCallerBase<FN>
{
	RV _result;

protected:
	virtual void Invoke(FN fn)
	{
		_result = fn();
	}

public:
	InMainCaller(FN fn) : InMainCallerBase<FN>(fn) { }

	RV Do()
	{
		InMainCallerBase<FN>::Do();
		return _result;
	}
};

template <class FN>
	class InMainCallerNoRet : public InMainCallerBase<FN>
{
protected:
	virtual void Invoke(FN fn)
	{
		fn();
	}

public:
	InMainCallerNoRet(FN fn) : InMainCallerBase<FN>(fn) { }
};

////////

template <class RV, class FN>
	static RV CallInMain(FN fn)
{
	InMainCaller<RV, FN> c(fn);
	return c.Do();
}

template <class FN>
	static void CallInMainNoRet(FN fn)
{
	InMainCallerNoRet<FN> c(fn);
	c.Do();
}
