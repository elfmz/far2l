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
#include "utils.h"

extern "C" {
	static class CustomFormats : std::map<UINT, wxDataFormat>
	{
		UINT _next_index;

	public:
		CustomFormats() :_next_index(0)
		{
		}

		UINT Register(LPCWSTR lpszFormat)
		{
			wxString format(lpszFormat);
			for (const auto &i : *this) {
				if (i.second.GetId() == format)
					return i.first;
			}

			for (;;) {
				_next_index++;
				if (_next_index < 0xC000 || _next_index > 0xFFFF)
					_next_index = 0xC000;
				if (find(_next_index)==end()) {
					insert(value_type(_next_index, wxDataFormat(format)));
					return _next_index;
				}
			}
		}
		
		const wxDataFormat *Lookup(UINT v) const
		{
			const_iterator i = find(v);
			return ( i == end() ) ? nullptr : &i->second;
		}


	} g_custom_formats;

	static std::set<PVOID> g_free_pendings;
	static wxDataObjectComposite *g_data_to_clipboard = nullptr;
	
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
		
		if (g_data_to_clipboard) {
			wxTheClipboard->SetData( g_data_to_clipboard );
			g_data_to_clipboard = nullptr;
		}
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
		delete g_data_to_clipboard;
		g_data_to_clipboard = nullptr;
		wxTheClipboard->Clear();
		return TRUE;
	}

	WINPORT_DECL(IsClipboardFormatAvailable, BOOL, (UINT format))
	{
		if (!wxIsMainThread()) {
			auto fn = std::bind(WINPORT(IsClipboardFormatAvailable), format);
			return CallInMain<BOOL>(fn);
		}
		
		if (format==CF_UNICODETEXT || format==CF_TEXT) {
			return wxTheClipboard->IsSupported( wxDF_TEXT ) ? TRUE : FALSE;
		} else {
			const wxDataFormat *data_format = g_custom_formats.Lookup(format);
			if (!data_format) {
				fprintf(stderr, "IsClipboardFormatAvailable(%u) - unrecognized format\n", format);
				return FALSE;
			}
			
			return wxTheClipboard->IsSupported(*data_format) ? TRUE : FALSE;
		}
	}
	
	WINPORT_DECL(GetClipboardData, HANDLE, (UINT format))
	{
		if (!wxIsMainThread()) {
			auto fn = std::bind(WINPORT(GetClipboardData), format);
			return CallInMain<HANDLE>(fn);
		}
		
		PVOID p = nullptr;		
		if (format==CF_UNICODETEXT || format==CF_TEXT) {
			wxTextDataObject data;
			if (!wxTheClipboard->GetData( data ))
				return nullptr;
				
			p = (format==CF_UNICODETEXT) ? 
				(void *)_wcsdup(data.GetText().wchar_str()) : 
				(void *)_strdup(data.GetText().char_str());
		} else {
			const wxDataFormat *data_format = g_custom_formats.Lookup(format);
			if (!data_format) {
				fprintf(stderr, "GetClipboardData(%u) - not registered format\n", format);
				return nullptr;
			}
			
			if (!wxTheClipboard->IsSupported(*data_format)) {
				//fprintf(stderr, "GetClipboardData(%s) - not supported format\n", 
				//	(const char *)data_format->GetId().char_str());
				return nullptr;
			}
			
			wxCustomDataObject data(*data_format);
			if (!wxTheClipboard->GetData( data )) {
				fprintf(stderr, "GetClipboardData(%s) - GetData failed\n", 
					(const char *)data.GetFormat().GetId().char_str());
				return nullptr;
			}
				
			const size_t data_size = data.GetDataSize();
			p = calloc(data_size + 1, 1); 
			if (!p) {
				fprintf(stderr, "GetClipboardData(%s) - cant alloc %u + 1\n", 
					(const char *)data_format->GetId().char_str(), (unsigned int)data_size);
				return nullptr;
			}
			data.GetDataHere(p);
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
		if (!g_data_to_clipboard) {
			g_data_to_clipboard = new wxDataObjectComposite;
		}
		if (format==CF_UNICODETEXT) {
			g_data_to_clipboard->Add(new wxTextDataObject((const wchar_t *)mem), true);
		} else if (format==CF_TEXT) {
			g_data_to_clipboard->Add(new wxTextDataObject((const char *)mem));
		} else {
			const wxDataFormat *data_format = g_custom_formats.Lookup(format);
			if (!data_format) {
				fprintf(stderr, 
					"SetClipboardData(%u, %p [%u]) - unrecognized format\n", 
					format, mem, (unsigned int)len);
			} else {
				wxCustomDataObject *dos = new wxCustomDataObject(*data_format);
				dos->SetData(len, mem);		
				g_data_to_clipboard->Add(dos);
			}
		}

		if (mem) {
			g_free_pendings.insert(mem);
		}
		return mem;
	}
}
