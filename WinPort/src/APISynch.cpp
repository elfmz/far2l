#include <mutex>
#include <thread>

#include "WinCompat.h"
#include "WinPort.h"
#include "WinPortHandle.h"
#include "WinPortSynch.h"


static volatile LONG g_winport_thread_id = 0;

static DWORD WinPortThreadIDGenerate()
{
	for (;;) {
		DWORD out = (DWORD)WINPORT(InterlockedIncrement)(&g_winport_thread_id);
		if (out != 0)
			return out;

		fprintf(stderr, "WinPortThreadIDGenerate: over zero\n");
	}
}

static pthread_key_t WinPortThreadIDKey()
{
	static pthread_key_t s_key = 0;
	static bool s_key_ready = false;
	if (!s_key_ready) {
		s_key_ready = (pthread_key_create(&s_key, nullptr) == 0);
	}
	return s_key_ready ? s_key : 0;
}

static void WinPortThreadID_Set(DWORD tid)
{
	pthread_setspecific(WinPortThreadIDKey(), (void *)(uintptr_t)tid);
}

static DWORD WinPortThreadID_Get()
{
	DWORD tid = (DWORD)(uintptr_t)pthread_getspecific(WinPortThreadIDKey());
	if (tid == 0) {
		tid = WinPortThreadIDGenerate();
		WinPortThreadID_Set(tid);
	}
	return tid;
}

class WinPortThread : public WinPortEvent
{
	std::mutex _resume_mutex;
	volatile LONG _exit_code;
	volatile LONG _tid;
	WINPORT_THREAD_START_ROUTINE _lpStartAddress;
	LPVOID _lpParameter;
	bool _started;

	static void *sProc(void *p)
	{
		WinPortThread *it = (WinPortThread *)p;
		void * out = (void *)(uintptr_t)it->Proc();
		it->Set();
		it->Dereference();
		return out;
	}

	DWORD Proc()
	{
		WinPortThreadID_Set(_tid);
		DWORD out = _lpStartAddress(_lpParameter);
		WINPORT(InterlockedExchange)(&_exit_code, (LONG)out);
		return out;
	}

	bool InternalResume()
	{
		std::lock_guard<std::mutex> lock(_resume_mutex);
		if (!_tid || _started)
			return false;

		_started = true;
		pthread_t trd = 0;
		if (pthread_create(&trd, NULL, sProc, this) != 0) {
			_started = false;
			return false;
		}
		pthread_detach(trd);
		return true;
	}
	
public:
	WinPortThread(WINPORT_THREAD_START_ROUTINE lpStartAddress, LPVOID lpParameter, LPDWORD lpThreadId) 
		: WinPortEvent(true, false), _exit_code(0), _tid(WinPortThreadIDGenerate()),
		_lpStartAddress(lpStartAddress), _lpParameter(lpParameter), _started(false)
	{
		if (lpThreadId)
			*lpThreadId = _tid;
//		fprintf(stderr, "::WinPortThread: %p\n", this);
	}
/*
	~WinPortThread()
	{
		fprintf(stderr, "::~WinPortThread: %p\n", this);
	}
*/

	DWORD GetExitCode()
	{
		return (DWORD)WINPORT(InterlockedCompareExchange)(&_exit_code, 0, 0);
	}
	
	bool Resume()
	{
		Reference();
		if (!InternalResume()) {
			Dereference();
			return false;
		}
		return true;
	}
};


WINPORT_DECL(CreateThread, HANDLE, (LPSECURITY_ATTRIBUTES lpThreadAttributes, 
	SIZE_T dwStackSize, WINPORT_THREAD_START_ROUTINE lpStartAddress, LPVOID lpParameter, DWORD dwCreationFlags, LPDWORD lpThreadId))
{
	WinPortThread *wpt = new WinPortThread(lpStartAddress, lpParameter, lpThreadId);
	HANDLE h = WinPortHandle_Register(wpt);
	if ((dwCreationFlags&CREATE_SUSPENDED) != CREATE_SUSPENDED) {
		if (!wpt->Resume()) {
			WinPortHandle_Deregister(h);
			h = NULL;
		}
	}

	return h;
}

WINPORT_DECL(GetCurrentThreadId, DWORD, ())
{
	return WinPortThreadID_Get();
}

WINPORT_DECL(ResumeThread, DWORD, (HANDLE hThread))
{
	AutoWinPortHandle<WinPortThread> wph(hThread);
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

