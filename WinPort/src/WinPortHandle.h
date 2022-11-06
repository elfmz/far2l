#pragma once
#include "WinCompat.h"
#include "WinPort.h"
#include <vector>
#include <atomic>

class WinPortHandle;

HANDLE WinPortHandle_Register(WinPortHandle *wph);
bool WinPortHandle_Deregister(HANDLE h);

WinPortHandle *WinPortHandle_Reference(HANDLE h);

class WinPortHandle
{
	std::atomic<unsigned int> _refcnt{0};

protected:
	virtual bool Cleanup();

public:
	WinPortHandle();
	virtual ~WinPortHandle();
	
	virtual bool WaitHandle(DWORD msec) {return true; }

	void Reference();
	bool Dereference();
};

template <class T> 
	class AutoWinPortHandle
{
	WinPortHandle *_wph;
	T *_p;

public:
	AutoWinPortHandle(HANDLE h) : _wph(WinPortHandle_Reference(h))
	{
		_p = dynamic_cast<T *>(_wph);
		if (!_p) {
			WINPORT(SetLastError)(ERROR_INVALID_HANDLE);
		}
	}
	~AutoWinPortHandle()
	{
		if (_wph)
			_wph->Dereference();
	}

	operator bool()
	{
		return _p!=nullptr;
	}

	T * operator -> ()
	{
		return _p;
	}
	
	T * get()
	{
		return _p;
	}
};



template <class T> 
	class AutoWinPortHandles
{
	std::vector<WinPortHandle *> _wphs;
	std::vector<T *> _ps;
	bool _good;
public:
	AutoWinPortHandles(HANDLE *handles, size_t count) : _good(count > 0)
	{
		_wphs.resize(count);
		_ps.resize(count);
		for (size_t i = 0; i < count; ++i) {
			WinPortHandle *wph;
			_wphs[i] = wph = WinPortHandle_Reference(handles[i]);
			if (wph) {
				T *p;
				_ps[i] = p = dynamic_cast<T *>(wph);
				if (!p) {
					_good = false;
					WINPORT(SetLastError)(ERROR_INVALID_HANDLE);
					//break;
				}
			}
		}
	}
	~AutoWinPortHandles()
	{
		for (auto wph : _wphs) {
			if (wph)
				wph->Dereference();
		}
	}

	operator bool()
	{
		return _good;
	}
	
	T ** get()
	{
		return &_ps[0];
	}
};
