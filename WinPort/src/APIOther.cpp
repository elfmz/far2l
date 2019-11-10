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
	WINPORT_DECL(InterlockedIncrement, LONG, (LONG volatile *Value))
	{
#ifdef _WIN32
		return _InterlockedIncrement(Value);
#else
		return __sync_add_and_fetch ( Value, 1);
#endif
	}

	WINPORT_DECL(InterlockedDecrement, LONG, (LONG volatile *Value))
	{
#ifdef _WIN32
		return _InterlockedDecrement(Value);
#else
		return __sync_sub_and_fetch ( Value, 1);
#endif
	}

	WINPORT_DECL(InterlockedExchange, LONG, (LONG volatile *Value, LONG NewValue))
	{
#ifdef _WIN32
		return _InterlockedExchange(Value, NewValue);
#else
		return __sync_lock_test_and_set(Value, NewValue);
#endif
	}

	WINPORT_DECL(InterlockedCompareExchange, LONG, (LONG volatile *Value, LONG NewValue, LONG CompareValue))
	{
#ifdef _WIN32
		return _InterlockedCompareExchange(Value, NewValue, CompareValue);
#else
		return __sync_val_compare_and_swap(Value, CompareValue, NewValue);
#endif
	}

	WINPORT_DECL(GetCurrentProcessId, DWORD, ())
	{
#ifdef _WIN32
		return ::GetCurrentProcessId();
#else
		return getpid();
#endif
	}

	WINPORT_DECL(LoadLibraryEx, PVOID, (LPCWSTR lpLibFileName, HANDLE hFile, DWORD dwFlags))
	{
#ifdef _WIN32
		return LoadLibraryEx(lpLibFileName, hFile, dwFlags);
#else
		std::string path = Wide2MB(lpLibFileName);
		PVOID rv = (PVOID)dlopen(path.c_str(), RTLD_LOCAL|RTLD_LAZY);
		if (rv) {
			typedef VOID (*tWINPORT_DllStartup)(const char *path);
			tWINPORT_DllStartup pStart = (tWINPORT_DllStartup)dlsym(rv, "WINPORT_DllStartup");
			if (pStart) pStart(path.c_str());
			//return 0;
			fprintf(stderr, "WINPORT: LoadLibraryEx(%ls):  %p\n", lpLibFileName, rv);
		} else {
			fprintf(stderr, "WINPORT: LoadLibraryEx(%ls): dlopen error %s\n", lpLibFileName, dlerror());
		}
		return rv;
#endif
	}

	WINPORT_DECL(FreeLibrary, BOOL, (HMODULE hModule))
	{
#ifdef _WIN32
		return FreeLibrary(hModule);
#else
		dlclose((void *)hModule);
		return TRUE;
#endif
	}

	WINPORT_DECL(GetProcAddress, PVOID, (HMODULE hModule, LPCSTR lpProcName))
	{
		PVOID rv;
#ifdef _WIN32
		rv = GetProcAddress(hModule, lpProcName);
#else
		rv = (PVOID)dlsym((void *)hModule, lpProcName);
#endif
		if (!rv) fprintf(stderr, "GetProcAddress(%p, %s) - no such symbol\n", hModule, lpProcName);
		return rv;
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

	static char *GetHostNameCached()
	{
		static char s_out[0x100] = {0};
		if (!s_out[0]) {
			gethostname(&s_out[1], 0xfe);
			s_out[0] = 1;
		}
		return &s_out[1];
	}


	WINPORT_DECL(GetEnvironmentVariable, DWORD, (LPCWSTR lpName, LPWSTR lpBuffer, DWORD nSize))
	{
#ifdef _WIN32
		return GetEnvironmentVariable(lpName, lpBuffer, nSize);
#else
		char *value = getenv(Wide2MB(lpName).c_str());
		if (!value) {
			if (lpName && wcscmp(lpName, L"HOSTNAME") == 0)
				value = GetHostNameCached();
			if (!value) {
				WINPORT(SetLastError)(ERROR_ENVVAR_NOT_FOUND);
				return 0;
			}
		}
		std::wstring wide = MB2Wide(value);
		if (wide.size() >= nSize)
			return (DWORD)wide.size() + 1;

		wcscpy(lpBuffer, wide.c_str());
		return (DWORD)wide.size();
#endif
	}

	WINPORT_DECL(SetEnvironmentVariable, BOOL, (LPCWSTR lpName, LPCWSTR lpValue))
	{
#ifdef _WIN32
		return SetEnvironmentVariable(lpName, lpValue);
#else
		int r;
		std::string name = Wide2MB(lpName);
		if (lpValue) {
			std::string value = Wide2MB(lpValue);
			r = setenv(name.c_str(), value.c_str(), 1);
		} else 
			r = unsetenv(name.c_str());
		return ( r < 0 ) ? FALSE : TRUE;
#endif
	}

	WINPORT_DECL(ExpandEnvironmentStrings, DWORD, (LPCWSTR lpSrc, LPWSTR lpDst, DWORD nSize))
	{
		std::wstring result;
		
		for (LPCWSTR tmp = lpSrc, perc = NULL; *tmp; ++tmp) {
			if (*tmp==L'%') {
				if (perc==NULL) {
					perc = tmp;
				} else if (tmp==(perc+1)) {
					result+= L'%';
					perc = NULL;
				} else {
					std::wstring var(perc + 1, tmp - perc - 1);
					perc = NULL;
					std::wstring val;val.resize(128);
					for (;;) {
						DWORD vallen = WINPORT(GetEnvironmentVariable)(var.c_str(), &val[0], val.size() - 1);
						if (vallen < val.size()) {
							val.resize(vallen);
							break;
						}
						val.resize(val.size() + val.size()/2 + 128);
					}
					result+= val;
				}
			} else if (perc==NULL)
				result+= *tmp;
		}
		if (result.size() < nSize) {
			wcscpy(lpDst, result.c_str());
		}
		
		//fprintf(stderr, "TODO: ExpandEnvironmentStrings(" WS_FMT ") -> " WS_FMT "\n", lpSrc,  result.c_str());
		return (DWORD)result.size() + 1;
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
