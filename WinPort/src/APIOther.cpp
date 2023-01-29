#include <mutex>

#include "WinPort.h"
#include "WinPortHandle.h"
#include "PathHelpers.h"
#include <utils.h>
#include <errno.h>

#ifndef _WIN32
# include <dlfcn.h>
#endif

extern "C" {

	WINPORT_DECL(GetLastError, DWORD, ())
	{
		return errno;
	}
	WINPORT_DECL(SetLastError, VOID, (DWORD code))
	{
		errno = code;
	}

	WINPORT_DECL(GetCurrentProcessId, DWORD, ())
	{
#ifdef _WIN32
		return ::GetCurrentProcessId();
#else
		return getpid();
#endif
	}

	WINPORT_DECL(GetDoubleClickTime, DWORD, ())
	{
		return 500;//Win's default value
	}

	BOOL WINPORT(CloseHandle)(HANDLE hObject)
	{
		if (!hObject || hObject == INVALID_HANDLE_VALUE) {
			WINPORT(SetLastError)(ERROR_INVALID_HANDLE);
			return FALSE;
		}

		if (!WinPortHandle::Deregister(hObject)) {
			return FALSE;
		}

		return TRUE;
	}

}
