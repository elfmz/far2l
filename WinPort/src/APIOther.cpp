#include <mutex>

#include "WinPort.h"
#include "WinPortHandle.h"
#include "PathHelpers.h"
#include <utils.h>
#include <pwd.h>
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

	WINPORT_DECL(GetComputerName, BOOL, (LPWSTR lpBuffer, LPDWORD nSize))
	{
		char buf[0x100] = {};
		if (gethostname(&buf[0], ARRAYSIZE(buf) - 1) != 0) {
			return FALSE;
		}
		const std::wstring &str = MB2Wide(buf);
		if (*nSize <= str.size()) {
			*nSize = (DWORD)str.size() + 1;
			WINPORT(SetLastError)(ERROR_BUFFER_OVERFLOW);
			return FALSE;
		}

		wcscpy(lpBuffer, str.c_str());
		*nSize = (DWORD)str.size();
		return TRUE;
	}

	WINPORT_DECL(GetUserName, BOOL, (LPWSTR lpBuffer, LPDWORD nSize))
	{
		struct passwd *pw = getpwuid(getuid());
		if (!pw || !pw->pw_name) {
			return FALSE;
		}

		const std::wstring &str = MB2Wide(pw->pw_name);
		if (*nSize <= str.size()) {
			*nSize = (DWORD)str.size() + 1;
			WINPORT(SetLastError)(ERROR_BUFFER_OVERFLOW);
			return FALSE;
		}

		wcscpy(lpBuffer, str.c_str());
		*nSize = (DWORD)str.size();
		return TRUE;
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
