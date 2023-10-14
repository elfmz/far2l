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
static std::atomic<size_t> s_pending_clipboard_allocations{0};

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
			WINPORT(ClipboardFree)(p);

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

//////////////////////////////////////////////////////////////////////////////////////////////////////////

	struct ClipboardAllocHeader
	{
		DWORD size;
		DWORD padding;
		DWORD64 magic;
	};

#define CAH_ALLOCATED_MAGIC 0x0610ba10A110CED0
#define CAH_FREED_MAGIC   0x0610ba10F4EED000

	WINPORT_DECL(ClipboardAlloc, PVOID, (SIZE_T dwBytes))
	{
		if (dwBytes > 0x7fffff00 || dwBytes == 0) {
			fprintf(stderr, "%s: insane amount wanted (%lu)\n", __FUNCTION__, (unsigned long)dwBytes);
			return NULL;
		}
		// allocate by one wchar_t more to ensure NUL-termination of any sane text format if setter didnt do that properly
		const size_t payload_size = dwBytes + sizeof(wchar_t);
		ClipboardAllocHeader *hdr = (ClipboardAllocHeader *)malloc(sizeof(ClipboardAllocHeader) + payload_size);
		if (!hdr) {
			fprintf(stderr, "%s: malloc(%lu) failed\n", __FUNCTION__, (unsigned long)(sizeof(ClipboardAllocHeader) + dwBytes));
			return NULL;
		}
		hdr->magic = CAH_ALLOCATED_MAGIC;
		hdr->size = (DWORD)dwBytes;
		void *rv = hdr + 1;
		memset(rv, 0, payload_size);
		auto pending_clipboard_allocations = ++s_pending_clipboard_allocations;
		if (pending_clipboard_allocations > 10) {
			fprintf(stderr, "%s: suspicious pending_clipboard_allocations=(%lu)\n",
				__FUNCTION__, (unsigned long)pending_clipboard_allocations);
		}
		return rv;
	}

	static ClipboardAllocHeader *ClipboardAccess(PVOID hMem)
	{
		ClipboardAllocHeader *hdr = (ClipboardAllocHeader *)((char *)hMem - sizeof(ClipboardAllocHeader));
		ASSERT_MSG(hdr->magic == CAH_ALLOCATED_MAGIC, "%s magic (0x%llx)",
			(hdr->magic == CAH_FREED_MAGIC) ? "freed" : "bad", (unsigned long long)hdr->magic);
		return hdr;
	}

	WINPORT_DECL(ClipboardFree, VOID, (PVOID hMem))
	{
		if (hMem) {
			ClipboardAllocHeader *hdr = ClipboardAccess(hMem);
			hdr->magic = CAH_FREED_MAGIC;
			bzero(hdr + 1, hdr->size); // avoid sensitive data leak
			--s_pending_clipboard_allocations;
			free(hdr);
		}
	}

	WINPORT_DECL(ClipboardSize, SIZE_T, (PVOID hMem))
	{
		return hMem ? ClipboardAccess(hMem)->size : 0;
	}

}
