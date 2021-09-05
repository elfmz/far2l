#include <mutex>
#include <thread>
#include <atomic>

#include "WinCompat.h"
#include "WinPort.h"
#include "WinPortHandle.h"
#include "WinPortSynch.h"
#include <luck.h>


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
		
	return WAIT_OBJECT_0;
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

