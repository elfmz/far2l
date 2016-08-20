#include "stdafx.h"
#include <mutex>
#include <errno.h>
#include <error.h>
#include "WinPort.h"
#include "ConsoleOutput.h"
#include "ConsoleInput.h"
#include "WinPortHandle.h"
#include "Utils.h"

#ifndef _WIN32
# include <dlfcn.h>
#endif

extern "C" {

	/* gcc doesn't know _Thread_local from C11 yet */
#ifdef __GNUC__
# define thread_local __thread
#elif __STDC_VERSION__ >= 201112L
# define thread_local _Thread_local
#elif defined(_MSC_VER)
# define thread_local __declspec(thread)
#else
# error Cannot define thread_local
#endif

	thread_local DWORD g_winport_lasterror;
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
		std::string path = UTF16to8(lpLibFileName);
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
		fprintf(stderr, "TODO: GetComputerName\n");
		return 0;
	}

	WINPORT_DECL(GetUserName, BOOL, (LPWSTR lpBuffer, LPDWORD nSize))
	{
		fprintf(stderr, "TODO: GetUserName\n");
		return 0;
	}

	WINPORT_DECL(GetEnvironmentVariable, DWORD, (LPCWSTR lpName, LPWSTR lpBuffer, DWORD nSize))
	{
#ifdef _WIN32
		return GetEnvironmentVariable(lpName, lpBuffer, nSize);
#else
		char *value = getenv(UTF16to8(lpName).c_str());
		if (!value) {
			WINPORT(SetLastError)(ERROR_ENVVAR_NOT_FOUND);
			return 0;
		}
		std::wstring wide = UTF8to16(value);
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
		std::string name = UTF16to8(lpName);
		if (lpValue) {
			std::string value = UTF16to8(lpValue);
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

	WINPORT_DECL(Sleep, VOID, (DWORD dwMilliseconds))
	{
#ifdef _WIN32
		::Sleep(dwMilliseconds);
#else
		DWORD seconds  = dwMilliseconds / 1000;
		if (seconds) {
			sleep(seconds);
			dwMilliseconds-= seconds * 1000;
		}
		usleep(dwMilliseconds * 1000);
#endif
	}

	DWORD  WINPORT(WSAGetLastError) ()
	{
		return errno;
	}
}
