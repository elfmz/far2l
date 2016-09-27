#pragma once
#include <stddef.h>
#include <mutex>
#include <condition_variable>
#include "WinCompat.h"
#include "WinPortHandle.h"


class WinPortSynch : public WinPortHandle
{
	static size_t sTryAcquireAll(WinPortSynch **synches, size_t count);
	static size_t sTryAcquireAny(WinPortSynch **synches, size_t count);
protected:
	virtual bool MayAcquire() = 0;
	virtual void Acquire() = 0;

public:
	
	static size_t sWait(size_t count, WinPortSynch **synches,  bool wait_all, DWORD msec = INFINITE);
};

class WinPortEvent : public WinPortSynch
{
	bool _manual_reset;
	bool _state;

protected:
	virtual bool MayAcquire();
	virtual void Acquire() ;
	
public:
	WinPortEvent(bool manual_reset, bool initial = false);
	void Set();
	void Reset();
};

class WinPortSemaphore : public WinPortSynch
{
	LONG _count;
	LONG _limit;

protected:
	virtual bool MayAcquire();
	virtual void Acquire() ;

public:
	WinPortSemaphore(LONG initial, LONG limit);
	
	LONG Increment(LONG count);
};

