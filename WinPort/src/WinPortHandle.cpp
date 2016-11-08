#include <set>
#include <mutex>
#include "WinPortHandle.h"
#include "WinCompat.h"
#include "WinPort.h"

WinPortHandle::WinPortHandle() : _refcnt(0)
{
}

WinPortHandle::~WinPortHandle() 
{
}

void WinPortHandle::Reference()
{
	WINPORT(InterlockedIncrement)(&_refcnt);
}

void WinPortHandle::Dereference()
{
	if (WINPORT(InterlockedDecrement)(&_refcnt)==0)
		OnReleased();
}

static struct WinPortHandles : std::set<WinPortHandle *>, std::mutex
{
} g_winport_handles;


HANDLE WinPortHandle_Register(WinPortHandle *wph)
{
	std::lock_guard<std::mutex> lock(g_winport_handles);
	wph->Reference();
	g_winport_handles.insert(wph);
	return (HANDLE)wph;
}

bool WinPortHandle_Deregister(HANDLE h)
{
	WinPortHandle *wph = reinterpret_cast<WinPortHandle *>(h);
	{
		std::lock_guard<std::mutex> lock(g_winport_handles);
		if (g_winport_handles.erase(wph)==0) {
			WINPORT(SetLastError)(ERROR_INVALID_HANDLE);
			return false;
		}
	}
	wph->Dereference();
	return true;
}



WinPortHandle *WinPortHandle_Reference(HANDLE h)
{
	WinPortHandle *wph = reinterpret_cast<WinPortHandle *>(h);
	{
		std::lock_guard<std::mutex> lock(g_winport_handles);
		if (g_winport_handles.find(wph)==g_winport_handles.end())
			return NULL;
		wph->Reference();
	}	
	return wph;
}


void WinPortHandle_FinalizeApp()
{
	std::vector<WinPortHandle *> wphv;
	{
		std::lock_guard<std::mutex> lock(g_winport_handles);
		for (auto h : g_winport_handles) {
			h->Reference();
			wphv.push_back(h);
		}
	}
	
	for (auto h : wphv) {
		h->OnFinalizeApp();
		h->Dereference();
	}
}
