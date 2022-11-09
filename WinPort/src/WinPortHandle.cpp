#include <set>
#include <mutex>
#include "WinPortHandle.h"
#include "WinCompat.h"
#include "WinPort.h"

WinPortHandle::WinPortHandle()
{
}

WinPortHandle::~WinPortHandle() 
{
}

void WinPortHandle::Reference()
{
	++_refcnt;
}

bool WinPortHandle::Dereference()
{
	if (0 == --_refcnt)
		return Cleanup();

	return true;
}

bool WinPortHandle::Cleanup()
{
	delete this;
	return true;
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
	return wph->Dereference();
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
