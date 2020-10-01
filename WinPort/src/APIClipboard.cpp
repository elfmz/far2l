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


static std::shared_ptr<IClipboardBackend> g_clipboard_backend;
static std::mutex g_clipboard_backend_mutex;

static struct ClipboardFreePendings : std::set<PVOID>,std::mutex {} g_clipboard_free_pendings;
static volatile LONG s_clipboard_open_track = 0;

bool WinPortClipboard_IsBusy()
{
	return (WINPORT(InterlockedCompareExchange)(&s_clipboard_open_track, 0, 0)!=0);
}

std::shared_ptr<IClipboardBackend> WinPortClipboard_SetBackend(std::shared_ptr<IClipboardBackend> &clipboard_backend)
{
	std::lock_guard<std::mutex> lock(g_clipboard_backend_mutex);
	auto out = g_clipboard_backend;
	g_clipboard_backend = clipboard_backend;
	return out;
}

static std::shared_ptr<IClipboardBackend> WinPortClipboard_GetBackend()
{
	std::shared_ptr<IClipboardBackend> out;
	std::lock_guard<std::mutex> lock(g_clipboard_backend_mutex);
	out = g_clipboard_backend;
	return out;
}

extern "C" {
	
	WINPORT_DECL(RegisterClipboardFormat, UINT, (LPCWSTR lpszFormat))
	{		
		auto cb = WinPortClipboard_GetBackend();
		return cb ? cb->OnClipboardRegisterFormat(lpszFormat) : 0;
	}

	WINPORT_DECL(OpenClipboard, BOOL, (PVOID Reserved))
	{
		auto cb = WinPortClipboard_GetBackend();
		if (!cb) {
			fprintf(stderr, "OpenClipboard - NO BACKEND\n");
			return FALSE;
		}

		if (WINPORT(InterlockedCompareExchange)(&s_clipboard_open_track, 1, 0) != 0) {
			fprintf(stderr, "OpenClipboard - BUSY\n");
			return FALSE;
		}
		
		if (!cb->OnClipboardOpen()) {
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

		auto cb = WinPortClipboard_GetBackend();
		if (cb)
			cb->OnClipboardClose();

		std::lock_guard<std::mutex> lock(g_clipboard_free_pendings);
		for (auto p : g_clipboard_free_pendings)
			free(p);
		g_clipboard_free_pendings.clear();

		return TRUE;
	}

	WINPORT_DECL(EmptyClipboard, BOOL, ())
	{
		auto cb = WinPortClipboard_GetBackend();
		if (!cb)
			return FALSE;

		cb->OnClipboardEmpty();
		return TRUE;
	}

	WINPORT_DECL(IsClipboardFormatAvailable, BOOL, (UINT format))
	{
		auto cb = WinPortClipboard_GetBackend();
		return cb ? cb->OnClipboardIsFormatAvailable(format) : FALSE;
	}
	
	WINPORT_DECL(GetClipboardData, HANDLE, (UINT format))
	{
		auto cb = WinPortClipboard_GetBackend();
		void *out = cb ? cb->OnClipboardGetData(format) : NULL;

		if (out) {
			std::lock_guard<std::mutex> lock(g_clipboard_free_pendings);
			g_clipboard_free_pendings.insert(out);
		}

		return out;
	}


	WINPORT_DECL(SetClipboardData, HANDLE, (UINT format, HANDLE mem))
	{
		auto cb = WinPortClipboard_GetBackend();
		void *out = cb ? cb->OnClipboardSetData(format, mem) : NULL;

		if (out) {
			std::lock_guard<std::mutex> lock(g_clipboard_free_pendings);
			g_clipboard_free_pendings.insert(out);
		}

		return out;
	}
}
