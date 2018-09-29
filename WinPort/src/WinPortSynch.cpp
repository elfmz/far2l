#include <assert.h>
#include "WinPortSynch.h"

static std::condition_variable s_winport_synch_cond;
static std::mutex s_winport_synch_mutex;

size_t WinPortSynch::sTryAcquireAll(WinPortSynch **synches, size_t count)
{
	size_t i;
	for ( i = 0; (i < count && synches[i]->MayAcquire()); ++i);
	if (i < count)
		return (size_t)-1;
		
	for ( i = 0; i < count; ++i) 
		synches[i]->Acquire();
	
	return 0;
}

size_t WinPortSynch::sTryAcquireAny(WinPortSynch **synches, size_t count)
{
	for (size_t i = 0; i<count; ++i) {
		if (synches[i]->MayAcquire()) {
			synches[i]->Acquire();
			return i;
		}
	}	
	return (size_t)-1;
}


size_t WinPortSynch::sWait(size_t count, WinPortSynch **synches, bool wait_all, DWORD msec)
{
	std::unique_lock<std::mutex> lock(s_winport_synch_mutex);
	for (;;) {
		size_t r = wait_all ? sTryAcquireAll(synches, count) : sTryAcquireAny(synches, count);
		if (r!=(size_t)-1)
			return r;

		if (!msec)
			return (size_t)-1;

		if (msec!=INFINITE)  {
			std::chrono::milliseconds ms_before = std::chrono::duration_cast< std::chrono::milliseconds >
				(std::chrono::steady_clock::now().time_since_epoch());
			s_winport_synch_cond.wait_for(lock, std::chrono::milliseconds(msec));
			std::chrono::milliseconds ms = std::chrono::duration_cast< std::chrono::milliseconds >
				(std::chrono::steady_clock::now().time_since_epoch());
			ms-= ms_before;

			if (ms.count()  < msec)
				msec-= ms.count();
			else
				msec = 0;

		} else
			s_winport_synch_cond.wait(lock);
	}
}


////////////////////////////////////
WinPortEvent::WinPortEvent(bool manual_reset, bool initial) 
	: _manual_reset(manual_reset), _state(initial)
{
}

bool WinPortEvent::MayAcquire()
{
	return _state;
}

void WinPortEvent::Acquire() 
{
	assert(_state);
	
	if (!_manual_reset)
		_state = false;
}


void WinPortEvent::Set()
{
	std::unique_lock<std::mutex> locker(s_winport_synch_mutex);
	if (!_state) {
		_state = true;
		s_winport_synch_cond.notify_all();
	}
}

void WinPortEvent::Reset()
{
	std::unique_lock<std::mutex> locker(s_winport_synch_mutex);
	_state = false;
}


////////////////////////////////////
WinPortSemaphore::WinPortSemaphore(LONG initial, LONG limit)
	:_count(initial), _limit(limit)
{

}

bool WinPortSemaphore::MayAcquire()
{
	return _count!=0;
}

void WinPortSemaphore::Acquire() 
{
	assert(_count>0);
		
	--_count;
}

LONG WinPortSemaphore::Increment(LONG count)
{
	std::unique_lock<std::mutex> locker(s_winport_synch_mutex);
	LONG prev = _count;
	_count+= count;
	if (_count > _limit)
		_count = _limit;
	if (!prev)
		s_winport_synch_cond.notify_all();
	return prev;
}
