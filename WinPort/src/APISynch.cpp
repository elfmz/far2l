#include <mutex>
#include <thread>

#include <wx/wx.h>
#include <wx/display.h>

#include "WinCompat.h"
#include "WinPort.h"
#include "WinPortHandle.h"
#include "WinPortSynch.h"


class WinPortThreadEvent : public WinPortEvent
{
	volatile DWORD _exit_code;
	std::mutex _thread_mutex;
	wxThread *_thread;
public:
	WinPortThreadEvent(wxThread *thread) 
		: WinPortEvent(true, false), _thread(thread)
	{
	}

	void OnThreadExited(DWORD exit_code)
	{
		_exit_code = exit_code;
		Set();
	}

	DWORD GetExitCode()
	{
		return _exit_code;
	}
	
	bool Resume()
	{
		std::lock_guard<std::mutex> lock(_thread_mutex);
		if (!_thread)
			return false;
			
		_thread->Run();
		return true;
	}

	void OnThreadStarted()
	{
		std::lock_guard<std::mutex> lock(_thread_mutex);
		_thread = 0;
	}
};

class WinPortThread : public wxThread
{
	WINPORT_THREAD_START_ROUTINE _lpStartAddress;
	LPVOID _lpParameter;
	WinPortThreadEvent *_event;

public:
	WinPortThread(WINPORT_THREAD_START_ROUTINE lpStartAddress, LPVOID lpParameter)
		:wxThread(wxTHREAD_DETACHED),  
		_lpStartAddress(lpStartAddress), _lpParameter(lpParameter), 
		_event(new WinPortThreadEvent(this) )
	{
		_event->Reference();
	}

	virtual ~WinPortThread()
	{
		_event->Dereference();
	}

	WinPortThreadEvent *Event()
	{
		return _event;
	}
protected:

	virtual ExitCode Entry()
	{
		_event->OnThreadStarted();
		DWORD r = _lpStartAddress(_lpParameter);
		_event->OnThreadExited(r);
		return 0;
	}
};

WINPORT_DECL(CreateThread, HANDLE, (LPSECURITY_ATTRIBUTES lpThreadAttributes, 
	SIZE_T dwStackSize, WINPORT_THREAD_START_ROUTINE lpStartAddress, LPVOID lpParameter, DWORD dwCreationFlags, LPDWORD lpThreadId))
{
	WinPortThread *wpt = new WinPortThread(lpStartAddress, lpParameter);
	HANDLE h = WinPortHandle_Register(wpt->Event());
	if ((dwCreationFlags&CREATE_SUSPENDED)!=CREATE_SUSPENDED) {
		wpt->Run();
	} else {
		fprintf(stderr, "TODO: ResumeThread\n");
	}

	return h;
}

WINPORT_DECL(GetCurrentThreadId, DWORD, ())
{
	return wxThread::GetCurrentId();
}

WINPORT_DECL(ResumeThread, DWORD, (HANDLE hThread))
{
	AutoWinPortHandle<WinPortThreadEvent> wph(hThread);
	if (!wph) {
		return WAIT_FAILED;
	}
		
	return wph->Resume() ? 1 : (DWORD)-1;
}


WINPORT_DECL(WaitForSingleObject, DWORD, (HANDLE hHandle, DWORD dwMilliseconds))
{
	AutoWinPortHandle<WinPortSynch> wps(hHandle);
	if (!wps) {
		return WAIT_FAILED;
	}
	
	WinPortSynch *synch = wps.get();
	size_t r = WinPortSynch::sWait(1, &synch, false, dwMilliseconds);
	if (r==(size_t)-1)
		return WAIT_TIMEOUT;
		
	return WAIT_OBJECT_0 ;
}


WINPORT_DECL(WaitForMultipleObjects, DWORD, (DWORD nCount, HANDLE *pHandles, BOOL bWaitAll, DWORD dwMilliseconds))
{
	AutoWinPortHandles<WinPortSynch> wps(pHandles, nCount);
	if (!wps) {
		return WAIT_FAILED;
	}
	
	size_t r = WinPortSynch::sWait(nCount, wps.get(), bWaitAll!=FALSE, dwMilliseconds);
	if (r==(size_t)-1)
		return WAIT_TIMEOUT;
		
	return WAIT_OBJECT_0 + r ;
}

WINPORT_DECL(CreateEvent, HANDLE, (LPSECURITY_ATTRIBUTES lpEventAttributes,
	BOOL bManualReset, BOOL bInitialState, LPCWSTR lpName))
{
	if (lpName) {
		fprintf(stderr, "TODO: CreateEvent lpName=" WS_FMT "\n", lpName);
	}
	return WinPortHandle_Register(
		new WinPortEvent(bManualReset!=FALSE, bInitialState!=FALSE));
}

WINPORT_DECL(SetEvent, BOOL, (HANDLE hEvent))
{
	AutoWinPortHandle<WinPortEvent> wpe(hEvent);
	if (!wpe) {
		return FALSE;
	}
	wpe->Set();
	return TRUE;
}

WINPORT_DECL(ResetEvent, BOOL, (HANDLE hEvent))
{
	AutoWinPortHandle<WinPortEvent> wpe(hEvent);
	if (!wpe) {
		return FALSE;
	}
	wpe->Reset();
	return TRUE;
}

WINPORT_DECL( CreateSemaphore, HANDLE, (LPSECURITY_ATTRIBUTES lpSemaphoreAttributes, LONG lInitialCount, LONG lMaximumCount, LPCTSTR lpName))
{
	if (lpName) {
		fprintf(stderr, "TODO: CreateSemaphore lpName=" WS_FMT "\n", lpName);
	}
	return WinPortHandle_Register(
		new WinPortSemaphore(lInitialCount, lMaximumCount));
}


WINPORT_DECL( ReleaseSemaphore, BOOL, (HANDLE hSemaphore, LONG lReleaseCount, PLONG lpPreviousCount))
{
	AutoWinPortHandle<WinPortSemaphore> wps(hSemaphore);
	if (!wps) {
		return FALSE;
	}
	
	LONG prev = wps->Increment(lReleaseCount);
	if (lpPreviousCount)
		*lpPreviousCount = prev;

	return TRUE;	
}

