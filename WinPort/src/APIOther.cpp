#include <mutex>

#include "WinPort.h"
#include "ConsoleOutput.h"
#include "ConsoleInput.h"
#include "WinPortHandle.h"
#include "PathHelpers.h"
#include <utils.h>
#include <pwd.h>

#ifndef _WIN32
# include <dlfcn.h>
#endif

extern "C" {

	/* gcc doesn't know _Thread_local from C11 yet */
	thread_local DWORD g_winport_lasterror;
	
	WINPORT(LastErrorGuard):: WINPORT(LastErrorGuard)() : value(g_winport_lasterror)
	{
	}
	
	WINPORT(LastErrorGuard)::~ WINPORT(LastErrorGuard)()
	{
		g_winport_lasterror = value;
	}
	
	WINPORT_DECL(GetLastError, DWORD, ())
	{
		return g_winport_lasterror;
	}
	WINPORT_DECL(SetLastError, VOID, (DWORD code))
	{
		g_winport_lasterror = code;
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
			WINPORT(TranslateErrno)();
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
			WINPORT(TranslateErrno)();
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
		if (!WinPortHandle_Deregister(hObject)) {
			return FALSE;
		}

		return TRUE;
	}

	DWORD  WINPORT(WSAGetLastError) ()
	{
		return errno;
	}
	
	WINPORT_DECL(TranslateErrno, VOID, ())
	{
		DWORD gle;
		switch (errno) {
			case 0: gle = 0; break;
			case ENOSPC: gle = ERROR_DISK_FULL; break;
			case EEXIST: gle = ERROR_ALREADY_EXISTS; break;
			case ENOENT: gle = ERROR_FILE_NOT_FOUND; break;
			case EACCES: case EPERM: gle = ERROR_ACCESS_DENIED; break;
			case ETXTBSY: gle = ERROR_SHARING_VIOLATION; break;
			case EINVAL: gle = ERROR_INVALID_PARAMETER; break;
			//case EROFS: gle = ; break;
			default:
				gle = 20000 + errno;
//				fprintf(stderr, "TODO: TranslateErrno - %d\n", errno );
		}
		
		WINPORT(SetLastError)(gle);
	}
}
