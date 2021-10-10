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
#include <atomic>
#include <condition_variable>

#include "WinCompat.h"
#include "WinPort.h"
#include "WinPortHandle.h"
#include "PathHelpers.h"
#include "utils.h"
#include "Backend.h"


static IClipboardBackend *g_clipboard_backend = nullptr;
static std::mutex g_clipboard_backend_mutex;

static struct ClipboardFreePendings : std::set<PVOID> {} g_clipboard_free_pendings;
static bool s_clipboard_open_track = false;

__attribute__ ((visibility("default"))) bool WinPortClipboard_IsBusy()
{
	std::lock_guard<std::mutex> lock(g_clipboard_backend_mutex);
	return s_clipboard_open_track;
}

__attribute__ ((visibility("default"))) IClipboardBackend *WinPortClipboard_SetBackend(IClipboardBackend *clipboard_backend)
{
	std::lock_guard<std::mutex> lock(g_clipboard_backend_mutex);
	auto out = g_clipboard_backend;
	g_clipboard_backend = clipboard_backend;
	return out;
}

extern "C" {
	
	WINPORT_DECL(RegisterClipboardFormat, UINT, (LPCWSTR lpszFormat))
	{		
		std::lock_guard<std::mutex> lock(g_clipboard_backend_mutex);
		return g_clipboard_backend ? g_clipboard_backend->OnClipboardRegisterFormat(lpszFormat) : 0;
	}

	WINPORT_DECL(OpenClipboard, BOOL, (PVOID Reserved))
	{
		std::lock_guard<std::mutex> lock(g_clipboard_backend_mutex);
		if (!g_clipboard_backend) {
			fprintf(stderr, "OpenClipboard - NO BACKEND\n");
			return FALSE;
		}

		if (s_clipboard_open_track) {
			fprintf(stderr, "OpenClipboard - BUSY\n");
			return FALSE;
		}
		s_clipboard_open_track = true;

		if (!g_clipboard_backend->OnClipboardOpen()) {
			s_clipboard_open_track = false;
			return FALSE;
		}

		return TRUE;
	}

	WINPORT_DECL(CloseClipboard, BOOL, ())
	{
		std::lock_guard<std::mutex> lock(g_clipboard_backend_mutex);
		if (!s_clipboard_open_track) {
			fprintf(stderr, "Excessive CloseClipboard!\n");
		}
		s_clipboard_open_track = false;

		if (g_clipboard_backend)
			g_clipboard_backend->OnClipboardClose();

		for (auto p : g_clipboard_free_pendings)
			free(p);
		g_clipboard_free_pendings.clear();

		return TRUE;
	}

	WINPORT_DECL(EmptyClipboard, BOOL, ())
	{
		std::lock_guard<std::mutex> lock(g_clipboard_backend_mutex);
		if (!g_clipboard_backend)
			return FALSE;

		g_clipboard_backend->OnClipboardEmpty();
		return TRUE;
	}

	WINPORT_DECL(IsClipboardFormatAvailable, BOOL, (UINT format))
	{
		std::lock_guard<std::mutex> lock(g_clipboard_backend_mutex);
		return g_clipboard_backend ? g_clipboard_backend->OnClipboardIsFormatAvailable(format) : FALSE;
	}
	
	WINPORT_DECL(GetClipboardData, HANDLE, (UINT format))
	{
		std::lock_guard<std::mutex> lock(g_clipboard_backend_mutex);
		void *out = g_clipboard_backend ? g_clipboard_backend->OnClipboardGetData(format) : NULL;

		if (out) {
			g_clipboard_free_pendings.insert(out);
		}

		return out;
	}


	WINPORT_DECL(SetClipboardData, HANDLE, (UINT format, HANDLE mem))
	{
		std::lock_guard<std::mutex> lock(g_clipboard_backend_mutex);
		void *out = g_clipboard_backend ? g_clipboard_backend->OnClipboardSetData(format, mem) : NULL;

		if (out) {
			g_clipboard_free_pendings.insert(out);
		}

		return out;
	}
}
