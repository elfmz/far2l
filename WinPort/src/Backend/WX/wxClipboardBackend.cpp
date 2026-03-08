#include <stdlib.h>
#include <map>
#include <algorithm>
#include <wx/wx.h>
#include <wx/display.h>
#include <wx/clipbrd.h>
#include "wx/evtloop.h"
#include <utils.h>
#include <dlfcn.h>

// #698: Mac: Copying to clipboard stopped working in wx 3.1 (not 100% sure about exact version).
// The fix is submitted, supposedly, into 3.2: https://github.com/wxWidgets/wxWidgets/pull/1623/files
// Guess the problem is only present in 3.1.
#if defined(__APPLE__) && (wxMAJOR_VERSION == 3) && (wxMINOR_VERSION == 1)
#define CLIPBOARD_HACK 1
#include "Mac/pasteboard.h"
#else
#define CLIPBOARD_HACK 0
#endif

#include "wxClipboardBackend.h"
#include "CallInMain.h"
#include "WinPort.h"

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
			if (find(_next_index) == end()) {
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

} g_wx_custom_formats;

static wxDataObjectComposite *g_wx_data_to_clipboard = nullptr;

wxClipboardBackend::wxClipboardBackend()
{
}

wxClipboardBackend::~wxClipboardBackend()
{
}

bool wxClipboardBackend::OnClipboardOpen()
{
	if (!wxIsMainThread()) {
		for (int attempt = 1; attempt <= 5; ++attempt) {
			auto fn = std::bind(&wxClipboardBackend::OnClipboardOpen, this);
			if (CallInMain<bool>(fn)) {
				return true;
			}
			usleep(20000 * attempt);
		}
		return false;
	}

	if (!wxTheClipboard->Open()) {
		fprintf(stderr, "OpenClipboard - FAILED\n");
		return false;
	}

	fprintf(stderr, "OpenClipboard\n");
	return true;
}

void wxClipboardBackend::OnClipboardClose()
{
	if (!wxIsMainThread()) {
		auto fn = std::bind(&wxClipboardBackend::OnClipboardClose, this);
		CallInMainNoRet(fn);
		return;
	}

	if (g_wx_data_to_clipboard) {
		if (wxTheClipboard->SetData( g_wx_data_to_clipboard) ) {
			fprintf(stderr, "wxTheClipboard->SetData - OK\n");

		} else {
			fprintf(stderr, "wxTheClipboard->SetData - FAILED\n");
		}
		g_wx_data_to_clipboard = nullptr;

	} else {
		fprintf(stderr, "CloseClipboard without data\n");
	}

#if !defined(__WXGTK__)
	// it never did what supposed to, and under Ubuntu 22.04/Wayland it started to kill gnome-shell
	wxTheClipboard->Flush();
#endif
/*
#if defined(__WXGTK__) && defined(__WXGTK3__) && !wxCHECK_VERSION(3, 1, 4)
	typedef void *(*gtk_clipboard_get_t)(uintptr_t);
	typedef void (*gtk_clipboard_set_can_store_t)(void *, void *, uint32_t);
	typedef void (*gtk_clipboard_store_t)(void *);

	static gtk_clipboard_get_t s_gtk_clipboard_get =
		(gtk_clipboard_get_t)dlsym(RTLD_DEFAULT, "gtk_clipboard_get");
	static gtk_clipboard_set_can_store_t s_gtk_clipboard_set_can_store =
		(gtk_clipboard_set_can_store_t)dlsym(RTLD_DEFAULT, "gtk_clipboard_set_can_store");
	static gtk_clipboard_store_t s_gtk_clipboard_store =
		(gtk_clipboard_store_t)dlsym(RTLD_DEFAULT, "gtk_clipboard_store");

	if (s_gtk_clipboard_get && s_gtk_clipboard_set_can_store && s_gtk_clipboard_store) {
		void *gcb = s_gtk_clipboard_get(69); // GDK_SELECTION_CLIPBOARD
		if (gcb) {
			s_gtk_clipboard_set_can_store(gcb, nullptr, 0);
			s_gtk_clipboard_store(gcb);
		}
	}
#endif
*/
	wxTheClipboard->Close();
}

void wxClipboardBackend::OnClipboardEmpty()
{
	if (!wxIsMainThread()) {
		auto fn = std::bind(&wxClipboardBackend::OnClipboardEmpty, this);
		CallInMainNoRet(fn);
		return;
	}

	fprintf(stderr, "EmptyClipboard\n");
	delete g_wx_data_to_clipboard;
	g_wx_data_to_clipboard = nullptr;
	wxTheClipboard->Clear();
}

bool wxClipboardBackend::OnClipboardIsFormatAvailable(UINT format)
{
	if (!wxIsMainThread()) {
		auto fn = std::bind(&wxClipboardBackend::OnClipboardIsFormatAvailable, this, format);
		return CallInMain<bool>(fn);
	}

	if (format==CF_UNICODETEXT || format==CF_TEXT) {
		return wxTheClipboard->IsSupported( wxDF_TEXT ) ? TRUE : FALSE;

	} else {
		const wxDataFormat *data_format = g_wx_custom_formats.Lookup(format);
		if (!data_format) {
			fprintf(stderr, "IsClipboardFormatAvailable(%u) - unrecognized format\n", format);
			return FALSE;
		}

		return wxTheClipboard->IsSupported(*data_format) ? TRUE : FALSE;
	}
}


class wxTextDataObjectTweaked : public wxTextDataObject
{
public:
	wxTextDataObjectTweaked(const wxString& text = wxEmptyString) : wxTextDataObject(text) { }

	virtual void GetAllFormats(wxDataFormat *formats, wxDataObjectBase::Direction dir = Get) const
	{
		// workaround for issue #1350
		// LibreOffice 7.3.5.2 seems to have bug that prevents text to be pasted if first in list format is
		// wxDF_UNICODETEXT but not wxDF_TEXT, changing order seems to resolve this weird issue
		wxTextDataObject::GetAllFormats(formats, dir);
		if (GetFormatCount() == 2 && formats[0] == wxDF_UNICODETEXT && formats[1] == wxDF_TEXT) {
			formats[0] = wxDF_TEXT;
			formats[1] = wxDF_UNICODETEXT;
		}
	}
};

// The trick below is for Wayland; X11 works well without direct gtk usage
#if defined(__WXGTK__) && defined(__WXGTK3__)

enum WxBackendType {
    NativeWayland,
    XWayland,
    NativeX11,
    Unknown,
    Undefined
};

enum WxClipboardType {
	Clipboard,
	Primary
};

static WxBackendType wxBackendType = WxBackendType::Undefined;
static WxClipboardType wxClipboardType = WxClipboardType::Clipboard;

/* Now this is a minimnal set from Gtk/Gtk */

typedef struct s_GTypeInstance { void *g_class; } GTypeInstance;
typedef unsigned long GType;
typedef int gboolean;
typedef char gchar;
typedef void* gpointer;
typedef int gint;

typedef GType (*gdk_wayland_display_get_type_t)(void);
typedef GType (*gdk_x11_display_get_type_t)(void);
typedef gboolean (*g_type_check_instance_is_a_t)(GTypeInstance*, GType);
typedef void* (*gdk_display_get_default_t)(void);

#define G_TYPE_CHECK_INSTANCE_TYPE(instance, type)	(g_type_check_instance_is_a( (GTypeInstance*)(instance), (type) ))

#define GDK_TYPE_WAYLAND_DISPLAY            (gdk_wayland_display_get_type())
#define GDK_TYPE_X11_DISPLAY                (gdk_x11_display_get_type())
#define GDK_IS_WAYLAND_DISPLAY(object)      (G_TYPE_CHECK_INSTANCE_TYPE ((object), GDK_TYPE_WAYLAND_DISPLAY))
#define GDK_IS_X11_DISPLAY(object)        	(G_TYPE_CHECK_INSTANCE_TYPE ((object), GDK_TYPE_X11_DISPLAY))

struct _GdkAtom;
struct _GtkClipboard;
typedef struct _GdkAtom *GdkAtom;
typedef struct _GtkClipboard GtkClipboard;

#define GUINT_TO_POINTER(x) ((void*)(x))
#define _GDK_MAKE_ATOM(val) ((GdkAtom)GUINT_TO_POINTER(val))
#define GDK_SELECTION_PRIMARY 		_GDK_MAKE_ATOM (1)

typedef void (*gtk_clipboard_set_text_t)(GtkClipboard *clipboard, const gchar *text, gint len);
typedef GtkClipboard* (*gtk_clipboard_get_t)(GdkAtom selection);
typedef void (*gtk_clipboard_store_t)(GtkClipboard *clipboard);

typedef void (* GtkClipboardTextReceivedFunc)(GtkClipboard *clipboard, const gchar *text, gpointer data);
typedef void (*gtk_clipboard_request_text_t)(GtkClipboard *clipboard, GtkClipboardTextReceivedFunc callback, gpointer user_data);

// the "functions" I'm using

static gdk_wayland_display_get_type_t gdk_wayland_display_get_type = 0;
static gdk_x11_display_get_type_t gdk_x11_display_get_type = 0;
static g_type_check_instance_is_a_t g_type_check_instance_is_a = 0;
static gdk_display_get_default_t gdk_display_get_default = 0;
static gtk_clipboard_request_text_t gtk_clipboard_request_text = 0;
static gtk_clipboard_set_text_t gtk_clipboard_set_text = 0;
static gtk_clipboard_get_t gtk_clipboard_get = 0;
static gtk_clipboard_store_t gtk_clipboard_store = 0;
static int gtk_loaded = 0;

static void assumeLazyLoadIsComplete() {
	if (gtk_loaded || g_type_check_instance_is_a) return;

	gdk_wayland_display_get_type = (gdk_wayland_display_get_type_t)dlsym(RTLD_DEFAULT, "gdk_wayland_display_get_type");
	gdk_x11_display_get_type = (gdk_x11_display_get_type_t)dlsym(RTLD_DEFAULT, "gdk_x11_display_get_type");
	g_type_check_instance_is_a = (g_type_check_instance_is_a_t)dlsym(RTLD_DEFAULT, "g_type_check_instance_is_a");
	gdk_display_get_default = (gdk_display_get_default_t)dlsym(RTLD_DEFAULT, "gdk_display_get_default");
	gtk_clipboard_request_text = (gtk_clipboard_request_text_t)dlsym(RTLD_DEFAULT, "gtk_clipboard_request_text");
	gtk_clipboard_set_text = (gtk_clipboard_set_text_t)dlsym(RTLD_DEFAULT, "gtk_clipboard_set_text");
	gtk_clipboard_store = (gtk_clipboard_store_t)dlsym(RTLD_DEFAULT, "gtk_clipboard_store");
	gtk_clipboard_get = (gtk_clipboard_get_t)dlsym(RTLD_DEFAULT, "gtk_clipboard_get");

	gtk_loaded = 1;
}

static WxBackendType detectWxBackend()
{
	if (wxBackendType == WxBackendType::Undefined) 
	{
		assumeLazyLoadIsComplete();

        void* display = gdk_display_get_default();

        const bool isWayland = GDK_IS_WAYLAND_DISPLAY(display);
        const bool isX11     = GDK_IS_X11_DISPLAY(display);
        const bool hasWaylandEnv = getenv("WAYLAND_DISPLAY") != NULL;

        if (isWayland)
            wxBackendType = WxBackendType::NativeWayland;
        else if (isX11 && hasWaylandEnv)
            wxBackendType = WxBackendType::XWayland;
        else if (isX11)
            wxBackendType = WxBackendType::NativeX11;
        else
			wxBackendType = WxBackendType::Unknown;
    }
    return wxBackendType;
}

static void setTextAsPrimarySelection(const wxString& text)
{
    GtkClipboard* primary = gtk_clipboard_get(GDK_SELECTION_PRIMARY);
    gtk_clipboard_set_text(primary, text.utf8_str(), -1);
    gtk_clipboard_store(primary);
}

static void getTextFromPrimarySelectionStart(std::function<void(const wxString&)> callback)
{
    GtkClipboard* primary = gtk_clipboard_get(GDK_SELECTION_PRIMARY);

    // Allocate callback on heap so GTK can call it later
    auto* cb = new std::function<void(const wxString&)>(callback);

    gtk_clipboard_request_text(
        primary,
        [](GtkClipboard* clipboard, const gchar* text, gpointer user_data)
        {
            auto* cb = static_cast<std::function<void(const wxString&)>*>(user_data);
            if (text)
                (*cb)(wxString::FromUTF8(text));
            else
                (*cb)(wxString());
            delete cb;
        },
        cb
    );
}

static wxString getTextFromPrimarySelection()
{
    wxString result;
    bool done = false;

    getTextFromPrimarySelectionStart([&](const wxString& text) {
        result = text;
        done = true;
    });

    wxEventLoop loop;
    while (!done)
        loop.Dispatch();

    return result;
}

#endif

void *wxClipboardBackend::OnClipboardSetData(UINT format, void *data)
{
	if (!wxIsMainThread()) {
		auto fn = std::bind(&wxClipboardBackend::OnClipboardSetData, this, format, data);
		return CallInMain<void *>(fn);
	}

#if defined(__WXGTK__) && defined(__WXGTK3__)
	if (wxClipboardType == WxClipboardType::Primary && detectWxBackend() == WxBackendType::NativeWayland) {
		wxString wx_str((const wchar_t *)data);
		setTextAsPrimarySelection(wx_str);
		return data;
	}
#endif

	size_t len = WINPORT(ClipboardSize)(data);
	fprintf(stderr, "SetClipboardData: format=%u len=%lu\n", format, (unsigned long)len);
	if (!g_wx_data_to_clipboard) {
		g_wx_data_to_clipboard = new wxDataObjectComposite;
	}

	if (format==CF_UNICODETEXT) {

		wxString wx_str((const wchar_t *)data);

		g_wx_data_to_clipboard->Add(new wxTextDataObjectTweaked(wx_str));

		wxCustomDataObject *cdo = new wxCustomDataObject(wxT("text/plain;charset=utf-8"));
		const std::string &tmp = wx_str.ToStdString();
		cdo->SetData(tmp.size(), tmp.c_str()); // not including ending NUL char
		g_wx_data_to_clipboard->Add(cdo);


#if (CLIPBOARD_HACK)
		CopyToPasteboard((const wchar_t *)data);
#endif

	} else if (format==CF_TEXT) {

		g_wx_data_to_clipboard->Add(new wxTextDataObjectTweaked(wxString::FromUTF8((const char *)data)));

		wxCustomDataObject *cdo = new wxCustomDataObject(wxT("text/plain;charset=utf-8"));
		cdo->SetData(strlen((const char *)data), data); // not including ending NUL char
		g_wx_data_to_clipboard->Add(cdo);

#if (CLIPBOARD_HACK)
		CopyToPasteboard((const char *)data);
#endif

	} else {
		const wxDataFormat *data_format = g_wx_custom_formats.Lookup(format);
		if (!data_format) {
			fprintf(stderr,
				"SetClipboardData(%u, %p [%lu]) - unrecognized format\n",
				format, data, (unsigned long)len);
		} else {
			wxCustomDataObject *dos = new wxCustomDataObject(*data_format);
			dos->SetData(len, data);
			g_wx_data_to_clipboard->Add(dos);
		}
	}

	return data;
}

void *wxClipboardBackend::OnClipboardGetData(UINT format)
{
	if (!wxIsMainThread()) {
		auto fn = std::bind(&wxClipboardBackend::OnClipboardGetData, this, format);
		return CallInMain<void *>(fn);
	}

#if defined(__WXGTK__) && defined(__WXGTK3__)
	if (wxClipboardType == WxClipboardType::Primary && detectWxBackend() == WxBackendType::NativeWayland) {
		wxString wx_str = getTextFromPrimarySelection();
		const auto &wc = wx_str.wc_str();
		return ClipboardAllocFromZeroTerminatedString<wchar_t>(wc);
	}
#endif

	PVOID p = nullptr;
	if (format==CF_UNICODETEXT || format==CF_TEXT) {

		wxString wx_str;
		bool data_found = false;

		wxTextDataObject data;
		if (wxTheClipboard->GetData( data )) {
			fprintf(stderr, "OnClipboardGetData(%u) - found wx-compatible text format\n", format);
			wx_str = data.GetText();
			data_found = true;
		}

		wxCustomDataObject cdo_utf8(wxT("text/plain;charset=utf-8"));
		if (!data_found && wxTheClipboard->GetData(cdo_utf8)) {
			const void* cdo_data = cdo_utf8.GetData();
			size_t cdo_size = cdo_utf8.GetSize();
			if (cdo_size > 0) {
				fprintf(stderr, "OnClipboardGetData(%u) - found MIME-compatible text format\n", format);
				const char *str = static_cast<const char*>(cdo_data);
				wx_str = wxString::FromUTF8(str, strnlen(str, cdo_size));
				data_found = true;
			}
		}

		if (!data_found) {
			fprintf(stderr, "OnClipboardGetData(%u) - no supported text format found\n", format);
			return nullptr;
		}

		if (format == CF_UNICODETEXT) {
			const auto &wc = wx_str.wc_str();
			p = ClipboardAllocFromZeroTerminatedString<wchar_t>(wc);

		} else {
			const auto &utf8 = wx_str.utf8_str();
			p = ClipboardAllocFromZeroTerminatedString<char>(utf8);
		}

	} else {
		const wxDataFormat *data_format = g_wx_custom_formats.Lookup(format);
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
		p = WINPORT(ClipboardAlloc)(data_size);
		if (!p) {
			fprintf(stderr, "GetClipboardData(%s) - cant alloc %u\n",
				(const char *)data_format->GetId().char_str(), (unsigned int)data_size);
			return nullptr;
		}

		if (data_size) {
			const void *data_buf = data.GetData();
			if (!data_buf) {
				fprintf(stderr, "GetClipboardData(%s) - cant get\n",
					(const char *)data_format->GetId().char_str());
				WINPORT(ClipboardFree)(p);
				return nullptr;
			}
			memcpy(p, data_buf, data_size);
		}
	}

	return p;
}

UINT wxClipboardBackend::OnClipboardRegisterFormat(const wchar_t *lpszFormat)
{
	if (!wxIsMainThread()) {
		auto fn = std::bind(&wxClipboardBackend::OnClipboardRegisterFormat, this, lpszFormat);
		return CallInMain<UINT>(fn);
	}

	return g_wx_custom_formats.Register(lpszFormat);
}

INT wxClipboardBackend::ChooseClipboard(INT format)
{
	if (!wxIsMainThread()) {
		auto fn = std::bind(&wxClipboardBackend::ChooseClipboard, this, format);
		return CallInMain<INT>(fn);
	}

	bool now = wxTheClipboard->IsUsingPrimarySelection();
	bool need = format > 0;
	if (now != need) wxTheClipboard->UsePrimarySelection(need);

#if defined(__WXGTK__) && defined(__WXGTK3__)
	::wxClipboardType = need ? WxClipboardType::Primary : WxClipboardType::Clipboard;
#endif
	return need ? 1 : 0;
}
