#include <stdlib.h>
#include <string>
#include <locale> 
#include <map>
#include <set>
#include <vector>
#include <algorithm>
#include <iostream>
#include <fstream>
#include <mutex>
#include <condition_variable>

#include "WinCompat.h"
#include "WinPort.h"
#include "WinPortHandle.h"
#include "PathHelpers.h"
#include "utils.h"
#include "Backend.h"


static IClipboardBackend *g_clipboard_backend = nullptr;
static struct ClipboardFreePendings : std::set<PVOID>,std::mutex {} g_clipboard_free_pendings;
static volatile LONG s_clipboard_open_track = 0;

bool WinPortClipboard_IsBusy()
{
	return (WINPORT(InterlockedCompareExchange)(&s_clipboard_open_track, 0, 0)!=0);
}

IClipboardBackend *WinPortClipboard_SetBackend(IClipboardBackend *clipboard_backend)
{
	// NB: currently it called once, before everything started up,
	// so g_clipboard_backend is not synchronized
	IClipboardBackend *prev = g_clipboard_backend;
	g_clipboard_backend = clipboard_backend;
	return prev;
}

extern "C" {
	
	WINPORT_DECL(RegisterClipboardFormat, UINT, (LPCWSTR lpszFormat))
	{		
		return g_clipboard_backend ? g_clipboard_backend->OnClipboardRegisterFormat(lpszFormat) : 0;
	}

	WINPORT_DECL(OpenClipboard, BOOL, (PVOID Reserved))
	{
		if (!g_clipboard_backend) {
			fprintf(stderr, "OpenClipboard - NO BACKEND\n");
			return FALSE;
		}
		if (WINPORT(InterlockedCompareExchange)(&s_clipboard_open_track, 1, 0) != 0) {
			fprintf(stderr, "OpenClipboard - BUSY\n");
			return FALSE;
		}
		
		if (!g_clipboard_backend->OnClipboardOpen()) {
			WINPORT(InterlockedCompareExchange)(&s_clipboard_open_track, 0, 1);
			return FALSE;
		}

		return TRUE;
	}

	WINPORT_DECL(CloseClipboard, BOOL, ())
	{
		if (WINPORT(InterlockedCompareExchange)(&s_clipboard_open_track, 0, 1) !=1 ) {
			fprintf(stderr, "Excessive CloseClipboard!\n");
		}

		if (g_clipboard_backend)
			g_clipboard_backend->OnClipboardClose();

		std::unique_lock<std::mutex> lock(g_clipboard_free_pendings);
		for (auto p : g_clipboard_free_pendings)
			free(p);
		g_clipboard_free_pendings.clear();

		return TRUE;
	}

	WINPORT_DECL(EmptyClipboard, BOOL, ())
	{
		if (!g_clipboard_backend)
			return FALSE;

		g_clipboard_backend->OnClipboardEmpty();
		return TRUE;
	}

	WINPORT_DECL(IsClipboardFormatAvailable, BOOL, (UINT format))
	{
		return g_clipboard_backend ? g_clipboard_backend->OnClipboardIsFormatAvailable(format) : FALSE;
	}
	
	WINPORT_DECL(GetClipboardData, HANDLE, (UINT format))
	{
		void *out = g_clipboard_backend ? g_clipboard_backend->OnClipboardGetData(format) : NULL;

		if (out) {
			std::unique_lock<std::mutex> lock(g_clipboard_free_pendings);
			g_clipboard_free_pendings.insert(out);
		}

		return out;
	}


	WINPORT_DECL(SetClipboardData, HANDLE, (UINT format, HANDLE mem))
	{
		void *out = g_clipboard_backend ? g_clipboard_backend->OnClipboardSetData(format, mem) : NULL;

		if (out) {
			std::unique_lock<std::mutex> lock(g_clipboard_free_pendings);
			g_clipboard_free_pendings.insert(out);
		}

		return out;
	}
}
