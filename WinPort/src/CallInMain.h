#pragma once
#include <mutex>
#include <condition_variable>

template<class RV, class FN>
	class InMainCaller
{
	FN _fn;
	std::mutex _mutex;
	std::condition_variable _cond;
	RV _result;
	bool _done;
	
	
	protected:
	void Callback()
	{
			_result = _fn();
			std::unique_lock<std::mutex> locker(_mutex);
			_done = true;
			_cond.notify_all();
	}
	
	public:
	InMainCaller(FN fn):_fn(fn) {
		
	}
	
	RV Do() {
		_done = false;
		wxTheApp->GetTopWindow()->GetEventHandler()->CallAfter(std::bind(&InMainCaller::Callback, this));
		for (;;) {
			std::unique_lock<std::mutex> locker(_mutex);
			if (_done) break;
			_cond.wait(locker);
		}
		return _result;
	}
};

template <class RV, class FN> 
	static RV CallInMain(FN fn)
{
	InMainCaller<RV, FN> c(fn);
	return c.Do();
}
