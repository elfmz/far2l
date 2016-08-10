#include "stdafx.h"
#include <set>
#include <mutex>
#include <chrono>
#include <thread>
#include <condition_variable>
#include "WinPortHandle.h"
#include "WinCompat.h"
#include "WinPort.h"
#include "WinPortSynch.h"


//TODO
class WinPortFSNotify : public WinPortEvent
{
public:
	WinPortFSNotify() 
		: WinPortEvent(true, false)
	{
	}

};

WINPORT_DECL(FindFirstChangeNotification, HANDLE, (LPCWSTR lpPathName, BOOL bWatchSubtree, DWORD dwNotifyFilter))
{
	WinPortFSNotify *wpn = new WinPortFSNotify;
	return WinPortHandle_Register(wpn);
}

WINPORT_DECL(FindCloseChangeNotification, BOOL, (HANDLE hChangeHandle))
{
	if (!WinPortHandle_Deregister(hChangeHandle)) {
		return FALSE;
	}
	
	return TRUE;
}

