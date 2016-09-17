#include <string>
#ifdef __APPLE__
#include <malloc/malloc.h>
#else
#include <malloc.h>
#endif
#include <locale> 
#include <map>
#include <set>
#include <vector>
#include <algorithm>
#include <iostream>
#include <fstream>
#include <mutex>
#include <condition_variable>
#include <wx/wx.h>
#include <wx/display.h>
#include <wx/clipbrd.h>

#include "WinCompat.h"
#include "WinPort.h"
#include "WinPortHandle.h"
#include "PathHelpers.h"
#include "CallInMain.h"

extern "C" {
	static class CustomFormats : std::map<UINT, std::wstring>  {
	public:
		CustomFormats() :_next_index(0)
		{
		}

		UINT Register(LPCWSTR lpszFormat)
		{
			for (auto i : *this) {
				if (i.second == lpszFormat)
					return i.first;
			}

			for (;;) {
				_next_index++;
				if (_next_index < 0xC000 || _next_index > 0xFFFF)
					_next_index = 0xC000;
				if (find(_next_index)==end()) {
					insert(value_type(_next_index, std::wstring(lpszFormat)));
					return _next_index;
				}
			}
		}


		UINT _next_index;
	} g_custom_formats;


	static struct CustomData : std::map<UINT, std::vector<char> >  {
	} g_custom_data;
	static std::set<PVOID> g_free_pendings;
	
	WINPORT_DECL(RegisterClipboardFormat, UINT, (LPCWSTR lpszFormat))
	{		
		if (!wxIsMainThread()) {
			auto fn = std::bind(WINPORT(RegisterClipboardFormat), lpszFormat);
			return CallInMain<UINT>(fn);
		}

		return g_custom_formats.Register(lpszFormat);
	}

	static volatile LONG s_clipboard_open_track = 0;
	
	bool WinPortIsClipboardBusy()
	{
		return (WINPORT(InterlockedCompareExchange)(&s_clipboard_open_track, 0, 0)!=0);
	}
	
	WINPORT_DECL(OpenClipboard, BOOL, (PVOID Reserved))
	{
		if (!wxIsMainThread()) {
			auto fn = std::bind(WINPORT(OpenClipboard), Reserved);
			return CallInMain<BOOL>(fn);
		}
		
		if (WINPORT(InterlockedCompareExchange)(&s_clipboard_open_track, 1, 0)!=0) {
			fprintf(stderr, "OpenClipboard - BUSY\n");
			return FALSE;
		}
		
		if (!wxTheClipboard->Open()) {
			WINPORT(InterlockedCompareExchange)(&s_clipboard_open_track, 0, 1);
			fprintf(stderr, "OpenClipboard - FAILED\n");
			return FALSE;
		}
		fprintf(stderr, "OpenClipboard\n");
		return TRUE;
	}

	WINPORT_DECL(CloseClipboard, BOOL, ())
	{
		if (!wxIsMainThread()) {
			auto fn = std::bind(WINPORT(CloseClipboard));
			return CallInMain<BOOL>(fn);
		}
		fprintf(stderr, "CloseClipboard\n");

		for (auto p : g_free_pendings) free(p);
		g_free_pendings.clear();
		wxTheClipboard->Flush();
		wxTheClipboard->Close();
		
		if (WINPORT(InterlockedCompareExchange)(&s_clipboard_open_track, 0, 1) !=1 ) {
			fprintf(stderr, "Excessive CloseClipboard!\n");
		}
		
		return TRUE;
	}
	WINPORT_DECL(EmptyClipboard, BOOL, ())
	{
		if (!wxIsMainThread()) {
			auto fn = std::bind(WINPORT(EmptyClipboard));
			return CallInMain<BOOL>(fn);
		}
		fprintf(stderr, "EmptyClipboard\n");
		wxTheClipboard->Clear();
		g_custom_data.clear();
		return TRUE;
	}

	WINPORT_DECL(IsClipboardFormatAvailable, BOOL, (UINT format))
	{
		if (!wxIsMainThread()) {
			auto fn = std::bind(WINPORT(IsClipboardFormatAvailable), format);
			return CallInMain<BOOL>(fn);
		}
		if (format!=CF_UNICODETEXT && format!=CF_TEXT) {
			CustomData::iterator i = g_custom_data.find(format);
			return (i==g_custom_data.end()) ? FALSE : TRUE;
		} else {
			return wxTheClipboard->IsSupported( wxDF_TEXT ) ? TRUE : FALSE;
		}
	}

	WINPORT_DECL(GetClipboardData, HANDLE, (UINT format))
	{
		if (!wxIsMainThread()) {
			auto fn = std::bind(WINPORT(GetClipboardData), format);
			return CallInMain<HANDLE>(fn);
		}
		PVOID p = NULL;		
		if (format==CF_UNICODETEXT) {
			wxTextDataObject data;
			wxTheClipboard->GetData( data );
			p = _wcsdup(data.GetText().wchar_str());
		} else if (format==CF_TEXT) {
			wxTextDataObject data;
			wxTheClipboard->GetData( data );
			p = _strdup(data.GetText().char_str());
		} else {
			CustomData::iterator i = g_custom_data.find(format);
			if (i==g_custom_data.end()) 
				return NULL;

			p = malloc(i->second.size());
			if (p) memcpy(p, &i->second[0], i->second.size());
		}

		if (p) {
			g_free_pendings.insert(p);
		}
		return p;
	}

	WINPORT_DECL(SetClipboardData, HANDLE, (UINT format, HANDLE mem))
	{
		if (!wxIsMainThread()) {
			auto fn = std::bind(WINPORT(SetClipboardData), format, mem);
			return CallInMain<HANDLE>(fn);
		}
#ifdef _WIN32
		size_t len = _msize(mem);
#elif defined(__APPLE__)
		size_t len = malloc_size(mem);
#else
		size_t len = malloc_usable_size(mem);
#endif
		fprintf(stderr, "SetClipboardData: %u\n", format);
		if (format==CF_UNICODETEXT) {
			wxTheClipboard->SetData( new wxTextDataObject((const wchar_t *)mem) );
		} else if (format==CF_TEXT) {
			wxTheClipboard->SetData( new wxTextDataObject((const char *)mem) );
		} else {
			std::vector<char> &data = g_custom_data[format];
			data.resize(len);
			if (len) memcpy(&data[0], mem, data.size());			
		}

		if (mem) {
			g_free_pendings.insert(mem);
		}
		return mem;
	}
}
