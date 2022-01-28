#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>

#include "Backend.h"
#include "wxWinTranslations.h"
#include "CallInMain.h"
#include "PathHelpers.h"
#include "Paint.h"
#include "utils.h"
#include "WinPortHandle.h"
#include "wxClipboardBackend.h"

#include <wx/wx.h>
#include <wx/display.h>
#include <wx/clipbrd.h>
#include <wx/debug.h>
#include <wx/filename.h>
#include <wx/stdpaths.h>

#include "ExclusiveHotkeys.h"
#include <set>
#include <fstream>
#include <vector>
#include <algorithm>
#include <atomic>

#ifdef __APPLE__
# include "Mac/dockicon.h"
# include "Mac/touchbar.h"
#endif


#define AREAS_REDUCTION

// If time between adhoc text copy and mouse button release less then this value then text will not be copied. Used to protect against unwanted copy-paste-s
#define QEDIT_COPY_MINIMAL_DELAY 150

IConsoleOutput *g_winport_con_out = nullptr;
IConsoleInput *g_winport_con_in = nullptr;
bool g_broadway = false, g_wayland = false, g_remote = false;
static int g_exit_code = 0;
enum
{
    TIMER_ID_PERIODIC = 10
};

bool WinPortClipboard_IsBusy();

bool non_latin_keyboard_events_propagate = false;


static void NormalizeArea(SMALL_RECT &area)
{
	if (area.Left > area.Right) std::swap(area.Left, area.Right);
	if (area.Top > area.Bottom) std::swap(area.Top, area.Bottom);	
}

class WinPortAppThread : public wxThread
{
public:
	WinPortAppThread(int argc, char **argv, int(*appmain)(int argc, char **argv))
		: wxThread(wxTHREAD_DETACHED), _backend(nullptr), _argv(argv), _argc(argc), _appmain(appmain)  {  }

	wxThreadError Start(IConsoleOutputBackend *backend)
	{
		_backend = backend;
		return Run();
	}

protected:
	virtual ExitCode Entry()
	{
		g_exit_code = _appmain(_argc, _argv);
		_backend->OnConsoleExit();
		//exit(_r);
		return 0;
	}

private:
	WinPortAppThread() = delete;
	WinPortAppThread(const WinPortAppThread&) = delete;
	IConsoleOutputBackend *_backend;
	char **_argv;
	int _argc;
	int(*_appmain)(int argc, char **argv);
} *g_winport_app_thread = NULL;

static void WinPortWxAssertHandler(const wxString& file,
		int line, const wxString& func,
		const wxString& cond, const wxString& msg)
{
	fprintf(stderr, "WinPortWxAssertHandler: file='%ls' line=%d func='%ls' cond='%ls' msg='%ls'\n",
			static_cast<const wchar_t*>(file.wc_str()), line,
			static_cast<const wchar_t*>(func.wc_str()),
			static_cast<const wchar_t*>(cond.wc_str()),
			static_cast<const wchar_t*>(msg.wc_str()));
}

static void DetectHostAbilities()
{
	const char *gdk_backend = getenv("GDK_BACKEND");
	if (gdk_backend && strcasecmp(gdk_backend, "broadway")==0) {
		g_broadway = true;
	}

	const char *xdg_st = getenv("XDG_SESSION_TYPE");
	if (xdg_st && strcasecmp(xdg_st, "wayland")==0) {
		g_wayland = true;
	}

	const char *ssh_conn = getenv("SSH_CONNECTION");
	if (ssh_conn && *ssh_conn
		&& strstr(ssh_conn, "127.0.0.") == NULL
		&& strstr(ssh_conn, "localhost") == NULL) {

		g_remote = true;
	}
}



extern "C" __attribute__ ((visibility("default"))) bool WinPortMainBackend(WinPortMainBackendArg *a)
{
	if (a->abi_version != FAR2L_BACKEND_ABI_VERSION) {
		fprintf(stderr, "This far2l_gui is not compatible and cannot be used\n");
		return false;
	}

	g_winport_con_out = a->winport_con_out;
	g_winport_con_in = a->winport_con_in;

	if (!wxInitialize())
		return false;

	wxSetAssertHandler(WinPortWxAssertHandler);

	DetectHostAbilities();

	if (!InitPalettes()) {
		uint xc,yc;
		g_winport_con_out->GetSize(xc, yc);
		g_winport_con_out->SetCursor( COORD {SHORT((xc>>1) - 5), SHORT(yc>>1)});
		WCHAR msg[] = L"ERROR IN PALETTE FILE";
		g_winport_con_out->WriteString(msg, wcslen(msg));
	}

	bool primary_selection = false;
	for (int i = 0; i < a->argc; ++i) {
		if (strcmp(a->argv[i], "--primary-selection") == 0) {
			primary_selection = true;
			break;
		}
	}
	if (primary_selection) {
		wxTheClipboard->UsePrimarySelection(true);
	}

	ClipboardBackendSetter clipboard_backend_setter;
	clipboard_backend_setter.Set<wxClipboardBackend>();
	if (a->app_main && !g_winport_app_thread) {
		g_winport_app_thread = new(std::nothrow) WinPortAppThread(a->argc, a->argv, a->app_main);
		if (!g_winport_app_thread) {
			wxUninitialize();
			return false;
		}
	}

	wxEntry(a->argc, a->argv);
	wxUninitialize();
	*a->result = g_exit_code;
	return true;
}

///////////////


static void SaveSize(unsigned int width, unsigned int height)
{
	std::ofstream os;
	os.open(InMyConfig("consolesize").c_str());
	if (os.is_open()) {
		os << width << std::endl;
		os << height << std::endl;
	}
}

static void LoadSize(unsigned int &width, unsigned int &height)
{
	std::ifstream is;
	is.open(InMyConfig("consolesize").c_str());
	if (is.is_open()) {
		std::string str;
		getline (is, str);
		if (!str.empty()) {
			width = atoi(str.c_str());
		}
		getline (is, str);
		if (!str.empty()) {
			height = atoi(str.c_str());
		}
	}
}

template <class COOKIE_T>
struct EventWith : wxCommandEvent
{
	EventWith(const COOKIE_T &cookie_, wxEventType commandType = wxEVT_NULL, int winid = 0) 
		:wxCommandEvent(commandType, winid), cookie(cookie_) { }

	virtual wxEvent *Clone() const { return new EventWith<COOKIE_T>(*this); }

	COOKIE_T cookie;
};


struct EventWithRect : EventWith<SMALL_RECT>
{
	EventWithRect(const SMALL_RECT &cookie_, wxEventType commandType = wxEVT_NULL, int winid = 0) 
		:EventWith<SMALL_RECT>(cookie_, commandType, winid)
	{
		NormalizeArea(cookie);
	}
};

typedef EventWith<bool> EventWithBool;
typedef EventWith<DWORD> EventWithDWORD;

///////////////////////////////////////////

wxDEFINE_EVENT(WX_CONSOLE_INITIALIZED, wxCommandEvent);
wxDEFINE_EVENT(WX_CONSOLE_REFRESH, wxCommandEvent);
wxDEFINE_EVENT(WX_CONSOLE_WINDOW_MOVED, wxCommandEvent);
wxDEFINE_EVENT(WX_CONSOLE_RESIZED, wxCommandEvent);
wxDEFINE_EVENT(WX_CONSOLE_TITLE_CHANGED, wxCommandEvent);
wxDEFINE_EVENT(WX_CONSOLE_SET_MAXIMIZED, wxCommandEvent);
wxDEFINE_EVENT(WX_CONSOLE_ADHOC_QEDIT, wxCommandEvent);
wxDEFINE_EVENT(WX_CONSOLE_SET_TWEAKS, wxCommandEvent);
wxDEFINE_EVENT(WX_CONSOLE_CHANGE_FONT, wxCommandEvent);
wxDEFINE_EVENT(WX_CONSOLE_EXIT, wxCommandEvent);


//////////////////////////////////////////

class WinPortApp: public wxApp
{
#ifdef __APPLE__
	std::shared_ptr<MacDockIcon> _mac_dock_icon = std::make_shared<MacDockIcon>();
#endif

public:
	virtual bool OnInit();
};

class WinPortFrame;

class WinPortPanel: public wxPanel, protected IConsoleOutputBackend
#ifdef __APPLE__
	, protected ITouchbarListener
#endif
{
public:
    WinPortPanel(WinPortFrame *frame, const wxPoint& pos, const wxSize& size);
	virtual ~WinPortPanel();
	void CompleteInitialization();
	void OnChar( wxKeyEvent& event );
	virtual void OnTouchbarKey(bool alternate, int index);

protected: 
	virtual void OnConsoleOutputUpdated(const SMALL_RECT *areas, size_t count);
	virtual void OnConsoleOutputResized();
	virtual void OnConsoleOutputTitleChanged();
	virtual void OnConsoleOutputWindowMoved(bool absolute, COORD pos);
	virtual COORD OnConsoleGetLargestWindowSize();
	virtual void OnConsoleSetMaximized(bool maximized);
	virtual void OnConsoleAdhocQuickEdit();
	virtual DWORD OnConsoleSetTweaks(DWORD tweaks);
	virtual void OnConsoleChangeFont();
	virtual void OnConsoleExit();
	virtual bool OnConsoleIsActive();
	virtual void OnConsoleDisplayNotification(const wchar_t *title, const wchar_t *text);
	virtual bool OnConsoleBackgroundMode(bool TryEnterBackgroundMode);
	virtual bool OnConsoleSetFKeyTitles(const char **titles);

private:
	void CheckForResizePending();
	void CheckPutText2CLip();
	void OnInitialized( wxCommandEvent& event );
	void OnTimerPeriodic(wxTimerEvent& event);	
	void OnWindowMovedSync( wxCommandEvent& event );
	void OnRefreshSync( wxCommandEvent& event );
	void OnConsoleResizedSync( wxCommandEvent& event );
	void OnTitleChangedSync( wxCommandEvent& event );
	void OnSetMaximizedSync( wxCommandEvent& event );
	void OnConsoleAdhocQuickEditSync( wxCommandEvent& event );
	void OnConsoleSetTweaksSync( wxCommandEvent& event );
	void OnConsoleChangeFontSync(wxCommandEvent& event);
	void OnConsoleExitSync( wxCommandEvent& event );
	void OnKeyDown( wxKeyEvent& event );
	void OnKeyUp( wxKeyEvent& event );
	void OnPaint( wxPaintEvent& event );
	void OnEraseBackground( wxEraseEvent& event );
	void OnSize(wxSizeEvent &event);
	void OnMouse( wxMouseEvent &event );
	void OnMouseNormal( wxMouseEvent &event, COORD pos_char);
	void OnMouseQEdit( wxMouseEvent &event, COORD pos_char);
	void OnSetFocus( wxFocusEvent &event );
	void OnKillFocus( wxFocusEvent &event );
	COORD TranslateMousePosition( wxMouseEvent &event );
	void DamageAreaBetween(COORD c1, COORD c2);
	int GetDisplayIndex();

	wxDECLARE_EVENT_TABLE();
	KeyTracker _key_tracker;
	
	ConsolePaintContext _paint_context;
	COORD _last_mouse_click{};
	wxMouseEvent _last_mouse_event;
	std::wstring _text2clip;
	ExclusiveHotkeys _exclusive_hotkeys;
	std::atomic<bool> _has_focus{false};
	MOUSE_EVENT_RECORD _prev_mouse_event;
	DWORD _prev_mouse_event_ts;

	WinPortFrame *_frame;
	wxTimer* _periodic_timer;
	bool _last_keydown_enqueued;
	bool _initialized;
	bool _adhoc_quickedit;
	enum
	{
		RP_NONE,
		RP_DEFER,
		RP_INSTANT
	} _resize_pending;
	DWORD _mouse_state, _mouse_qedit_start_ticks, _mouse_qedit_moved;
	COORD _mouse_qedit_start, _mouse_qedit_last;
	
	int _last_valid_display;
	DWORD _refresh_rects_throttle;
	unsigned int _pending_refreshes;
	struct RefreshRects : std::vector<SMALL_RECT>, std::mutex {} _refresh_rects;
};

///////////////////////////////////////////

class WinPortFrame: public wxFrame
{
public:
    WinPortFrame(const wxString& title, const wxPoint& pos, const wxSize& size);
	virtual ~WinPortFrame();
	
	void OnShow(wxShowEvent &show);
	
protected: 
	void OnClose(wxCloseEvent &show);
private:
	enum {
		ID_CTRL_BASE = 1,
		ID_CTRL_END = ID_CTRL_BASE + 'Z' - 'A' + 1,
		ID_CTRL_SHIFT_BASE,
		ID_CTRL_SHIFT_END = ID_CTRL_SHIFT_BASE + 'Z' - 'A' + 1,
		ID_ALT_BASE,
		ID_ALT_END = ID_ALT_BASE + 'Z' - 'A' + 1
	};
	WinPortPanel		*_panel;
	bool _shown;
	wxMenuBar *_menu_bar;
	std::vector<wxMenu *> _menus;

	void OnChar( wxKeyEvent& event )
	{
		_panel->OnChar(event);
	}
	
	void OnAccelerator(wxCommandEvent& event);
	void OnPaint( wxPaintEvent& event ) {}
	void OnEraseBackground( wxEraseEvent& event ) {}
	
    wxDECLARE_EVENT_TABLE();
};


wxBEGIN_EVENT_TABLE(WinPortFrame, wxFrame)
	EVT_PAINT(WinPortFrame::OnPaint)
	EVT_SHOW(WinPortFrame::OnShow)
	EVT_CLOSE(WinPortFrame::OnClose)
	EVT_ERASE_BACKGROUND(WinPortFrame::OnEraseBackground)
	EVT_CHAR(WinPortFrame::OnChar)
	EVT_MENU_RANGE(ID_CTRL_BASE, ID_CTRL_END, WinPortFrame::OnAccelerator)
	EVT_MENU_RANGE(ID_CTRL_SHIFT_BASE, ID_CTRL_SHIFT_END, WinPortFrame::OnAccelerator)
	EVT_MENU_RANGE(ID_ALT_BASE, ID_ALT_END, WinPortFrame::OnAccelerator)
wxEND_EVENT_TABLE()

void WinPortFrame::OnShow(wxShowEvent &show)
{
// create accelerators to handle hotkeys (like Ctrl-O) when using non-latin layouts under Linux
// under osx acceleratos seems not needed and only causes bugs
#ifndef __APPLE__
	struct stat s;
	if (stat(InMyConfig("nomenu").c_str(), &s)!=0) {
		//workaround for non-working with non-latin input language Ctrl+? hotkeys 
		_menu_bar = new wxMenuBar(wxMB_DOCKABLE);
		char str[128];
		
		wxMenu *menu = new wxMenu;
		for (char c = 'A'; c<='Z'; ++c) {
			sprintf(str, "Ctrl + %c\tCtrl+%c", c, c);
			menu->Append(ID_CTRL_BASE + (c - 'A'), wxString(str));
		}
		_menu_bar->Append(menu, _T("Ctrl + ?"));
		
		menu = new wxMenu;
		for (char c = 'A'; c <= 'Z'; ++c) {
			sprintf(str, "Ctrl + Shift + %c\tCtrl+Shift+%c", c, c);
			menu->Append(ID_CTRL_SHIFT_BASE + (c - 'A'), wxString(str));
		}
		_menu_bar->Append(menu, _T("Ctrl + Shift + ?"));

		menu = new wxMenu;
		for (char c = 'A'; c <= 'Z'; ++c) {
			sprintf(str, "Alt + %c\tAlt+%c", c, c);
			menu->Append(ID_ALT_BASE + (c - 'A'), wxString(str));
		}
		_menu_bar->Append(menu, _T("Alt + ?"));

		SetMenuBar(_menu_bar);
		
		//now hide menu bar just like it gets hidden during fullscreen transition
        //wxAcceleratorTable table(wxCreateAcceleratorTableForMenuBar(_menu_bar);
        //if (table.IsOk())
        //    SetAcceleratorTable(table);		
		_menu_bar->Show(false);
	}
#endif
	
	if (!_shown) {
		_shown = true;
		wxCommandEvent *event = new(std::nothrow) wxCommandEvent(WX_CONSOLE_INITIALIZED);
		if (event)
			wxQueueEvent(_panel, event);
	}
}	

void WinPortFrame::OnClose(wxCloseEvent &event)
{
	if (WINPORT(GenerateConsoleCtrlEvent)(CTRL_CLOSE_EVENT, 0)) {
		event.Veto();
	}
}

WinPortFrame::WinPortFrame(const wxString& title, const wxPoint& pos, const wxSize& size)
        : wxFrame(NULL, wxID_ANY, title, pos, size), _shown(false), 
		_menu_bar(nullptr)
{
	_panel = new WinPortPanel(this, wxPoint(0, 0), GetClientSize());
	_panel->SetFocus();
	SetBackgroundColour(*wxBLACK);
}

WinPortFrame::~WinPortFrame()
{
	SetMenuBar(nullptr);
	delete _menu_bar;
	delete _panel;
	_panel = NULL;
}

	
void WinPortFrame::OnAccelerator(wxCommandEvent& event)
{
	if (non_latin_keyboard_events_propagate) { return; }

	INPUT_RECORD ir = {};
	ir.EventType = KEY_EVENT;
	ir.Event.KeyEvent.bKeyDown = TRUE;
	ir.Event.KeyEvent.wRepeatCount = 1;
	if (event.GetId() >= ID_CTRL_BASE && event.GetId() < ID_CTRL_END) {
		ir.Event.KeyEvent.dwControlKeyState = LEFT_CTRL_PRESSED;
		ir.Event.KeyEvent.wVirtualKeyCode = 'A' + (event.GetId() - ID_CTRL_BASE);
		
	} else if (event.GetId() >= ID_CTRL_SHIFT_BASE && event.GetId() < ID_CTRL_SHIFT_END) {
		ir.Event.KeyEvent.dwControlKeyState = LEFT_CTRL_PRESSED | SHIFT_PRESSED;
		ir.Event.KeyEvent.wVirtualKeyCode = 'A' + (event.GetId() - ID_CTRL_SHIFT_BASE);
		
	} else if (event.GetId() >= ID_ALT_BASE && event.GetId() < ID_ALT_END) {
		ir.Event.KeyEvent.dwControlKeyState = LEFT_ALT_PRESSED;
		ir.Event.KeyEvent.wVirtualKeyCode = 'A' + (event.GetId() - ID_ALT_BASE);

	} else {
		fprintf(stderr, "OnAccelerator: bad ID=%u\n", event.GetId());
		return;
	}
	
	fprintf(stderr, "OnAccelerator: ID=%u ControlKeyState=0x%x Key=0x%x '%c'\n", 
		event.GetId(), ir.Event.KeyEvent.dwControlKeyState, ir.Event.KeyEvent.wVirtualKeyCode, ir.Event.KeyEvent.wVirtualKeyCode );
		
	g_winport_con_in->Enqueue(&ir, 1);
	ir.Event.KeyEvent.bKeyDown = FALSE;
	g_winport_con_in->Enqueue(&ir, 1);
}

////////////////////////////



wxBEGIN_EVENT_TABLE(WinPortPanel, wxPanel)
	EVT_COMMAND(wxID_ANY, WX_CONSOLE_INITIALIZED, WinPortPanel::OnInitialized)
	EVT_TIMER(TIMER_ID_PERIODIC, WinPortPanel::OnTimerPeriodic)
	EVT_COMMAND(wxID_ANY, WX_CONSOLE_REFRESH, WinPortPanel::OnRefreshSync)
	EVT_COMMAND(wxID_ANY, WX_CONSOLE_WINDOW_MOVED, WinPortPanel::OnWindowMovedSync)
	EVT_COMMAND(wxID_ANY, WX_CONSOLE_RESIZED, WinPortPanel::OnConsoleResizedSync)
	EVT_COMMAND(wxID_ANY, WX_CONSOLE_TITLE_CHANGED, WinPortPanel::OnTitleChangedSync)
	EVT_COMMAND(wxID_ANY, WX_CONSOLE_SET_MAXIMIZED, WinPortPanel::OnSetMaximizedSync)
	EVT_COMMAND(wxID_ANY, WX_CONSOLE_ADHOC_QEDIT, WinPortPanel::OnConsoleAdhocQuickEditSync)
	EVT_COMMAND(wxID_ANY, WX_CONSOLE_SET_TWEAKS, WinPortPanel::OnConsoleSetTweaksSync)
	EVT_COMMAND(wxID_ANY, WX_CONSOLE_CHANGE_FONT, WinPortPanel::OnConsoleChangeFontSync)
	EVT_COMMAND(wxID_ANY, WX_CONSOLE_EXIT, WinPortPanel::OnConsoleExitSync)
	
	EVT_KEY_DOWN(WinPortPanel::OnKeyDown)
	EVT_KEY_UP(WinPortPanel::OnKeyUp)
	EVT_CHAR(WinPortPanel::OnChar)
	EVT_PAINT(WinPortPanel::OnPaint)
	EVT_ERASE_BACKGROUND(WinPortPanel::OnEraseBackground)
	EVT_SIZE(WinPortPanel::OnSize)
	EVT_MOUSE_EVENTS(WinPortPanel::OnMouse )
	EVT_SET_FOCUS(WinPortPanel::OnSetFocus )
	EVT_KILL_FOCUS(WinPortPanel::OnKillFocus )

wxEND_EVENT_TABLE()

wxIMPLEMENT_APP_NO_MAIN(WinPortApp);


static WinPortFrame *g_winport_frame = nullptr;

wxEvtHandler *WinPort_EventHandler()
{
	if (!g_winport_frame) {
		return wxTheApp->GetTopWindow()->GetEventHandler();
	}

	return g_winport_frame->GetEventHandler();
}

bool WinPortApp::OnInit()
{
	g_winport_frame = new WinPortFrame("WinPortApp", wxDefaultPosition, wxDefaultSize );
//    WinPortFrame *frame = new WinPortFrame( "WinPortApp", wxPoint(50, 50), wxSize(800, 600) );
	g_winport_frame->Show( true );
	if (g_broadway)
		g_winport_frame->Maximize();
		
	return true;
}



///////////////////////////


WinPortPanel::WinPortPanel(WinPortFrame *frame, const wxPoint& pos, const wxSize& size)
        : wxPanel(frame, wxID_ANY, pos, size, wxWANTS_CHARS | wxNO_BORDER), 
		_paint_context(this), _has_focus(true), _prev_mouse_event_ts(0), _frame(frame), _periodic_timer(NULL),
		_last_keydown_enqueued(false), _initialized(false), _adhoc_quickedit(false),
		_resize_pending(RP_NONE),  _mouse_state(0), _mouse_qedit_start_ticks(0), _mouse_qedit_moved(false), _last_valid_display(0),
		_refresh_rects_throttle(0), _pending_refreshes(0)
{
	g_winport_con_out->SetBackend(this);
	_periodic_timer = new wxTimer(this, TIMER_ID_PERIODIC);
	_periodic_timer->Start(500);
	OnConsoleOutputTitleChanged();
}

WinPortPanel::~WinPortPanel()
{
#ifdef __APPLE__
	Touchbar_Deregister();
#endif
	delete _periodic_timer;
	g_winport_con_out->SetBackend(NULL);
}


void WinPortPanel::OnInitialized( wxCommandEvent& event )
{
	int w, h;
	GetClientSize(&w, &h);
	fprintf(stderr, "OnInitialized: client size = %u x %u\n", w, h);
	unsigned int cw, ch;
	g_winport_con_out->GetSize(cw, ch);
	LoadSize(cw, ch);
	wxDisplay disp(GetDisplayIndex());
	wxRect rc = disp.GetClientArea();
	if ((unsigned)rc.GetWidth() >= cw * _paint_context.FontWidth() 
		&& (unsigned)rc.GetHeight() >= ch * _paint_context.FontHeight()) {
		g_winport_con_out->SetSize(cw, ch);		
	}
	g_winport_con_out->GetSize(cw, ch);
	
	cw*= _paint_context.FontWidth();
	ch*= _paint_context.FontHeight();
	if ( w != (int)cw || h != (int)ch)
		_frame->SetClientSize(cw, ch);
		
	_initialized = true;

	if (g_winport_app_thread) {
#ifdef __APPLE__
		Touchbar_Register(this);
#endif
		WinPortAppThread *tmp = g_winport_app_thread;
		g_winport_app_thread = NULL;
		if (tmp->Start(this) != wxTHREAD_NO_ERROR)
			delete tmp;
	}
}

bool WinPortPanel::OnConsoleSetFKeyTitles(const char **titles)
{
	if (!wxIsMainThread()) {
		auto fn = std::bind(&WinPortPanel::OnConsoleSetFKeyTitles, this, titles);
		return CallInMain<bool>(fn);

	} else {
#ifdef __APPLE__
		return Touchbar_SetTitles(titles);
#else
		return false;
#endif
	}
}

void WinPortPanel::OnTouchbarKey(bool alternate, int index)
{
	INPUT_RECORD ir = {};
	ir.EventType = KEY_EVENT;
	ir.Event.KeyEvent.wRepeatCount = 1;

	if (!alternate) {
		ir.Event.KeyEvent.wVirtualKeyCode = VK_F1 + index;

	} else switch (index) { // "", "Ins", "Del", "",  "+", "-", "*", "/",  "Home", "End", "PageUp", "PageDown"
		case 0: return;
		case 1: ir.Event.KeyEvent.wVirtualKeyCode = VK_INSERT; break;
		case 2: ir.Event.KeyEvent.wVirtualKeyCode = VK_DELETE; break;
		case 3: return;

		case 4: ir.Event.KeyEvent.wVirtualKeyCode = VK_ADD; break;
		case 5: ir.Event.KeyEvent.wVirtualKeyCode = VK_SUBTRACT; break;
		case 6: ir.Event.KeyEvent.wVirtualKeyCode = VK_MULTIPLY; break;
		case 7: ir.Event.KeyEvent.wVirtualKeyCode = VK_DIVIDE; break;

		case 8: ir.Event.KeyEvent.wVirtualKeyCode = VK_HOME; break;
		case 9: ir.Event.KeyEvent.wVirtualKeyCode = VK_END; break;
		case 10: ir.Event.KeyEvent.wVirtualKeyCode = VK_PRIOR; break;
		case 11: ir.Event.KeyEvent.wVirtualKeyCode = VK_NEXT; break;
	}

	if (wxGetKeyState(WXK_NUMLOCK)) ir.Event.KeyEvent.dwControlKeyState|= NUMLOCK_ON;
	if (wxGetKeyState(WXK_SCROLL)) ir.Event.KeyEvent.dwControlKeyState|= SCROLLLOCK_ON;
	if (wxGetKeyState(WXK_CAPITAL)) ir.Event.KeyEvent.dwControlKeyState|= CAPSLOCK_ON;
	if (wxGetKeyState(WXK_SHIFT)) ir.Event.KeyEvent.dwControlKeyState|= SHIFT_PRESSED;
	if (wxGetKeyState(WXK_CONTROL)) ir.Event.KeyEvent.dwControlKeyState|= LEFT_CTRL_PRESSED;
	if (wxGetKeyState(WXK_ALT)) ir.Event.KeyEvent.dwControlKeyState|= LEFT_ALT_PRESSED;

	fprintf(stderr, "%s: F%d dwControlKeyState=0x%x\n", __FUNCTION__,
		index + 1, ir.Event.KeyEvent.dwControlKeyState);

	ir.Event.KeyEvent.bKeyDown = TRUE;
	g_winport_con_in->Enqueue(&ir, 1);
	ir.Event.KeyEvent.bKeyDown = FALSE;
	g_winport_con_in->Enqueue(&ir, 1);

}

void WinPortPanel::CheckForResizePending()
{
#ifndef __APPLE__
	if (_initialized && _resize_pending!=RP_NONE)
#endif
	{
#ifndef __APPLE__
		fprintf(stderr, "CheckForResizePending\n");
#endif
		DWORD conmode = 0;
		if (WINPORT(GetConsoleMode)(NULL, &conmode) && (conmode&ENABLE_WINDOW_INPUT)!=0) {
			unsigned int prev_width = 0, prev_height = 0;
			_resize_pending = RP_NONE;

			g_winport_con_out->GetSize(prev_width, prev_height);
	
			int width = 0, height = 0;
			_frame->GetClientSize(&width, &height);
#ifndef __APPLE__			
			fprintf(stderr, "Current client size: %u %u font %u %u\n", 
				width, height, _paint_context.FontWidth(), _paint_context.FontHeight());
#endif
			width/= _paint_context.FontWidth(); 
			height/= _paint_context.FontHeight();
			if (width!=(int)prev_width || height!=(int)prev_height) {
				fprintf(stderr, "Changing size: %u x %u\n", width, height);
#ifdef __APPLE__
				this->SetSize(width * _paint_context.FontWidth(), height * _paint_context.FontHeight());
#endif
				g_winport_con_out->SetSize(width, height);
				if (!_frame->IsFullScreen() && !_frame->IsMaximized() && _frame->IsShown()) {
					SaveSize(width, height);
				}
				INPUT_RECORD ir = {0};
				ir.EventType = WINDOW_BUFFER_SIZE_EVENT;
				ir.Event.WindowBufferSizeEvent.dwSize.X = width;
				ir.Event.WindowBufferSizeEvent.dwSize.Y = height;
				g_winport_con_in->Enqueue(&ir, 1);
			}
#ifndef __APPLE__
			Refresh(false);
#endif
		} else if (_resize_pending != RP_INSTANT) {
			_resize_pending = RP_INSTANT;
			//fprintf(stderr, "RP_INSTANT\n");
		}
	}	
}

void WinPortPanel::OnTimerPeriodic(wxTimerEvent& event)
{
	CheckForResizePending();
	CheckPutText2CLip();	
	if (_mouse_qedit_start_ticks != 0 && WINPORT(GetTickCount)() - _mouse_qedit_start_ticks > QEDIT_COPY_MINIMAL_DELAY) {
		DamageAreaBetween(_mouse_qedit_start, _mouse_qedit_last);
	}
	_paint_context.BlinkCursor();
}

static int ProcessAllEvents()
{
	wxApp *app  =wxTheApp;
	if (app) {
		while (app->Pending())
			app->Dispatch();
	}
	return 0;
}

void WinPortPanel::OnConsoleOutputUpdated(const SMALL_RECT *areas, size_t count)
{
	enum {
		A_NOTHING,
		A_QUEUE,
		A_THROTTLE
	} action;

	{
		std::lock_guard<std::mutex> lock(_refresh_rects);
		if (_refresh_rects.empty()) {
			action = A_QUEUE;
#ifndef __APPLE__
		} else if (_refresh_rects_throttle != (DWORD)-1 &&
				WINPORT(GetTickCount)() - _refresh_rects_throttle > 500 &&
				!wxIsMainThread()) {
			action = A_THROTTLE;
			_refresh_rects_throttle = (DWORD)-1;
#else
//TODO: fix stuck
#endif
		} else {
			action = A_NOTHING;
		}
		for (size_t i = 0; i < count; ++i) {
#ifdef AREAS_REDUCTION
			SMALL_RECT area = areas[i];
			NormalizeArea(area);
			bool add = true;
			for (auto &pending : _refresh_rects) {
/*				if (!(area.Left <= pending.Right && area.Right >= pending.Left &&
					     area.Top <= pending.Bottom && area.Bottom >= pending.Top )) {
					continue;
				}*/

				if (area.Left >= pending.Left && area.Right <= pending.Right
				&& area.Top >= pending.Top && area.Bottom <= pending.Bottom) {
					// new area completely inside of pending one
					add = false;
					break;
				}

				if (area.Left <= pending.Left && area.Right >= pending.Right
				&& area.Top <= pending.Top && area.Bottom >= pending.Bottom) {
					// pending area completely inside of new one
					pending = area;
					add = false;
					break;
				}

				if (area.Top == pending.Top && area.Bottom == pending.Bottom) {
					// left/right touch/intersect?
					if (pending.Left <= area.Right + 1 && area.Left <= pending.Right + 1) {
						if (pending.Left > area.Left) {
							pending.Left = area.Left;
						}
						if (pending.Right < area.Right) {
							pending.Right = area.Right;
						}
						add = false;
						break;
					}
				} else if (area.Left == pending.Left && area.Right == pending.Right) {
					// top/bottom touch/intersect?
					if (pending.Top <= area.Bottom + 1 && area.Top <= pending.Bottom + 1) {
						if (pending.Top > area.Top) {
							pending.Top = area.Top;
						}
						if (pending.Bottom < area.Bottom) {
							pending.Bottom = area.Bottom;
						}
						add = false;
						break;
					}
				}
			}
			if (add)
				_refresh_rects.emplace_back(area);
#else // AREAS_REDUCTION
			_refresh_rects.emplace_back(areas[i]);
			NormalizeArea(_refresh_rects.back());
#endif // AREAS_REDUCTION
		}
	}
	
	switch (action) {
		case A_QUEUE: {
			wxCommandEvent *event = new(std::nothrow) wxCommandEvent(WX_CONSOLE_REFRESH);
			if (event)
				wxQueueEvent (this, event);
		} break;
		
		case A_THROTTLE: {
			auto fn = std::bind(&ProcessAllEvents);
			CallInMain<int>(fn);
			std::lock_guard<std::mutex> lock(_refresh_rects);
			_refresh_rects_throttle = WINPORT(GetTickCount)();
			if (_refresh_rects_throttle == (DWORD)-1)
				_refresh_rects_throttle = 0;
		} break;
		
		case A_NOTHING: break;
	}
}

void WinPortPanel::OnConsoleOutputResized()
{
	wxCommandEvent *event = new(std::nothrow) wxCommandEvent(WX_CONSOLE_RESIZED);
	if (event)
		wxQueueEvent	(this, event);
}

void WinPortPanel::OnConsoleOutputTitleChanged()
{
	wxCommandEvent *event = new(std::nothrow) wxCommandEvent(WX_CONSOLE_TITLE_CHANGED);
	if (event)
		wxQueueEvent	(this, event);
}

void WinPortPanel::OnConsoleOutputWindowMoved(bool absolute, COORD pos)
{
	SMALL_RECT rect = {pos.X, pos.Y, absolute ? (SHORT)1 : (SHORT)0, 0};
	wxCommandEvent *event = new(std::nothrow) EventWithRect(rect, WX_CONSOLE_WINDOW_MOVED);
	if (event)
		wxQueueEvent	(this, event);
}

int WinPortPanel::GetDisplayIndex()
{
	int index = wxDisplay::GetFromWindow(_frame);
	if (index < 0 || index >= (int)wxDisplay::GetCount()) {
		fprintf(stderr, "OnConsoleGetLargestWindowSize: bad display %d, will use %d\n", index, _last_valid_display);
		index = _last_valid_display;
	}
	return index;
}

COORD WinPortPanel::OnConsoleGetLargestWindowSize()
{
	if (!wxIsMainThread()){
		auto fn = std::bind(&WinPortPanel::OnConsoleGetLargestWindowSize, this);
		return CallInMain<COORD>(fn);
	}
	
	wxDisplay disp(GetDisplayIndex());
	wxRect rc = disp.GetClientArea();
	wxSize outer_size = _frame->GetSize();
	wxSize inner_size = GetClientSize();
	rc.SetWidth(rc.GetWidth() - (outer_size.GetWidth() - inner_size.GetWidth()));
	rc.SetHeight(rc.GetHeight() - (outer_size.GetHeight() - inner_size.GetHeight()));
	COORD size = {(SHORT)(rc.GetWidth() / _paint_context.FontWidth()), (SHORT)(rc.GetHeight() / _paint_context.FontHeight())};
//	fprintf(stderr, "OnConsoleGetLargestWindowSize: %u x %u\n", size.X, size.Y);
	return size;
}

void WinPortPanel::OnConsoleSetMaximized(bool maximized)
{
	EventWithBool *event = new(std::nothrow) EventWithBool(maximized, WX_CONSOLE_SET_MAXIMIZED);
	if (event)
		wxQueueEvent(this, event);
}

void WinPortPanel::OnWindowMovedSync( wxCommandEvent& event )
{
	EventWithRect *e = (EventWithRect *)&event;
	int x = e->cookie.Left, y = e->cookie.Top;
	x*= _paint_context.FontWidth(); y*= _paint_context.FontHeight();
	if (!e->cookie.Right) {
		int dx = 0, dy = 0;
		GetPosition(&dx, &dy);
		x+= dx; y+= dy;
	}
	Move(x, y);
}

void WinPortPanel::OnSetMaximizedSync( wxCommandEvent& event )
{
	EventWithBool *e = (EventWithBool *)&event;
	_frame->Maximize(e->cookie);
}

void WinPortPanel::OnRefreshSync( wxCommandEvent& event )
{
	std::vector<SMALL_RECT> refresh_rects;
	{
		std::lock_guard<std::mutex> lock(_refresh_rects);
		if (_refresh_rects.empty())
			return;

		refresh_rects.swap(_refresh_rects);	
	}

	for (const auto & r : refresh_rects) {
		_paint_context.RefreshArea( r );
		// Seems there is some sort of limitation of how many fragments
		// can be pending in window refresh region on GTK.
		// Empirically found that value 400 is too high, 300 looks good,
		// so using 200 to be sure-good
		_pending_refreshes++;
		if (_pending_refreshes > 200) {
			Update();
		}
	}
}

void WinPortPanel::OnConsoleResizedSync( wxCommandEvent& event )
{
	unsigned int width = 0, height = 0;
	g_winport_con_out->GetSize(width, height);
	int prev_width = 0, prev_height = 0;
	_frame->GetClientSize(&prev_width, &prev_height);
	fprintf(stderr, "OnConsoleResized client size: %u %u\n", prev_width, prev_height);

	prev_width/= _paint_context.FontWidth();
	prev_height/= _paint_context.FontHeight();
	
	if ((int)width != prev_width || (int)height != prev_height) {
//		if ((int)width > prev_width || (int)height > prev_height) {
//			_frame->SetPosition(wxPoint(0,0 ));
//		}
		
		width*= _paint_context.FontWidth();
		height*= _paint_context.FontHeight();
		fprintf(stderr, "OnConsoleResized SET client size: %u %u\n", width, height);
		_frame->SetClientSize(width, height);		
	}
	Refresh(false);
}

void WinPortPanel::OnTitleChangedSync( wxCommandEvent& event )
{
	const std::wstring &title = g_winport_con_out->GetTitle();
	wxGetApp().SetAppDisplayName(title.c_str());
	_frame->SetTitle(title.c_str());
}


static bool IsForcedCharTranslation(int code)
{
	return (code==WXK_NUMPAD0 || code==WXK_NUMPAD1|| code==WXK_NUMPAD2 
		|| code==WXK_NUMPAD3 || code==WXK_NUMPAD4 || code==WXK_NUMPAD5
		|| code==WXK_NUMPAD6 || code==WXK_NUMPAD7 || code==WXK_NUMPAD8 
		|| code==WXK_NUMPAD9 || code==WXK_NUMPAD_DECIMAL || code==WXK_NUMPAD_SPACE
		|| code==WXK_NUMPAD_SEPARATOR || code==WXK_NUMPAD_EQUAL || code==WXK_NUMPAD_ADD
		|| code==WXK_NUMPAD_MULTIPLY || code==WXK_NUMPAD_SUBTRACT || code==WXK_NUMPAD_DIVIDE);
}

static int GTKHardwareKeyCodeToVirtualKeyCode(int code)
{
    // Returns virtual key code
    // (as defined in https://docs.microsoft.com/en-us/windows/win32/inputdev/virtual-key-codes)
    // corresponding to passed GTK hardware keycode

    // Returns zero for every key except non-numeric character keys

	switch (code) {

        // Hardware key codes are defined in /usr/share/X11/xkb/keycodes/xfree86

		case 20: return VK_OEM_MINUS;	// -
		case 21: return VK_OEM_PLUS;	// =
		//   22 is Backspace
		//   23 is Tab
		case 24: return 'Q';			// q
		case 25: return 'W';			// w
		case 26: return 'E';			// e
		case 27: return 'R';			// r
		case 28: return 'T';			// t
		case 29: return 'Y';			// y
		case 30: return 'U';			// u
		case 31: return 'I';			// i
		case 32: return 'O';			// o
		case 33: return 'P';			// p
		case 34: return VK_OEM_4;		// [
		case 35: return VK_OEM_6;		// ]
		//   36 is Enter
		//   37 is RCtrl
		case 38: return 'A';			// a
		case 39: return 'S';			// s
		case 40: return 'D';			// d
		case 41: return 'F';			// f
		case 42: return 'G';			// g
		case 43: return 'H';			// h
		case 44: return 'J';			// j
		case 45: return 'K';			// k
		case 46: return 'L';			// l
		case 47: return VK_OEM_1;		// ;
		case 48: return VK_OEM_7;		// '
		case 49: return VK_OEM_3;		// `
		//   50 is LShift
		case 51: return VK_OEM_5;		/* \ */
		case 52: return 'Z';			// z
		case 53: return 'X';			// x
		case 54: return 'C';			// c
		case 55: return 'V';			// v
		case 56: return 'B';			// b
		case 57: return 'N';			// n
		case 58: return 'M';			// m
		case 59: return VK_OEM_COMMA;	// ,
		case 60: return VK_OEM_PERIOD;	// .
		case 61: return VK_OEM_2;		// /
	}

	return 0; // not applicable
}

void WinPortPanel::OnKeyDown( wxKeyEvent& event )
{	
	DWORD now = WINPORT(GetTickCount)();
	const auto uni = event.GetUnicodeKey();
	fprintf(stderr, "OnKeyDown: raw=%x code=%x uni=%x (%lc) ts=%lu [now=%u]",
		event.GetRawKeyCode(), event.GetKeyCode(),
		uni, (uni > 0x1f) ? uni : L' ', event.GetTimestamp(), now);

	_exclusive_hotkeys.OnKeyDown(event, _frame);

	bool keystroke_doubled = !g_wayland
		&& event.GetTimestamp()
		&& _key_tracker.LastKeydown().GetKeyCode() == event.GetKeyCode()
		&& _key_tracker.LastKeydown().GetTimestamp() == event.GetTimestamp()
#ifdef __APPLE__
		// in macos under certain stars superposition all events get same timestamps (#325)
		// however vise-verse problem also can be observed, where some keystrokes get duplicated
		// last time: catalina in hackintosh, Ctrl+O works buggy
		&& now - _key_tracker.LastKeydownTicks() < 50 // so enforce extra check actual real time interval
#endif
		;

	if (event.GetSkipped() || keystroke_doubled) {
		fprintf(stderr, " SKIP\n");
		event.Skip();
		return;
	}

	_key_tracker.OnKeyDown(event, now);
	if (_key_tracker.Composing()) {
		fprintf(stderr, " COMPOSING\n");
		event.Skip();
		return;
	}

	fprintf(stderr, "\n");

	// dont check for alt key sudden keyup cuz it breaks Win key Alt behaviour
	// also it didnt cause problems yet
	if ( (_key_tracker.Shift() && !event.ShiftDown())
		|| ((_key_tracker.LeftControl() || _key_tracker.RightControl()) && !event.ControlDown())) {
		if (_key_tracker.CheckForSuddenModifiersUp()) {
			_exclusive_hotkeys.Reset();
		}
	}

	_last_keydown_enqueued = false;

	wx2INPUT_RECORD ir(TRUE, event, _key_tracker);
	const DWORD &dwMods = (ir.Event.KeyEvent.dwControlKeyState
		& (LEFT_ALT_PRESSED | SHIFT_PRESSED | LEFT_CTRL_PRESSED | RIGHT_CTRL_PRESSED));

	if (event.GetKeyCode() == WXK_RETURN && dwMods == LEFT_ALT_PRESSED) {
		_resize_pending = RP_INSTANT;
		//fprintf(stderr, "RP_INSTANT\n");
		_frame->ShowFullScreen(!_frame->IsFullScreen());
		if (_resize_pending != RP_INSTANT)
			_resize_pending = RP_DEFER;
		_last_keydown_enqueued = true;
		return;
	}

/*
    Workaround for NonLatinLetters

    In some configurations WX do not send KeyDown/KeyUp for NonLatinLetters,
    so we need to workaround this using onAccelerator

    In other configurations WX do send KeyDown/KeyUp for NonLatinLetters,
    but with empty key code. To deal with it, we detect such situations,
    and guess virtual key code by hardware key code
    (GetRawKeyFlags() returns hardware keycode under GTK).

    Also hardware keycodes allow us to determine if it is character key or not
    to apply workaround only for character keys.

    NB! Keys like "-" or "=" can also be alphabetical in layouts like Armenian.

*/
#if defined(__WXGTK__) && !defined(__APPLE__) && !defined(__FreeBSD__) // only tested on Linux
	const int vkc_from_gtk_hw_keycode =
		GTKHardwareKeyCodeToVirtualKeyCode(event.GetRawKeyFlags());
	const bool nonlatin_workaround = (true
		&& event.GetUnicodeKey() != 0    // key has unicode value => possibly character key
		&& vkc_from_gtk_hw_keycode       // but let's also check by hw keycode to be sure
		&& event.GetKeyCode() == 0       // and no keycode - so workaround is required
	);
	// for non-latin unicode keycode pressed with Alt key together
	// simulate some dummy key code for far2l to "see" keypress
	if (nonlatin_workaround) {
		// wow, we have Alt+NonLatinLetter keyboard events!
		// no need for accelerators hack anymore

		non_latin_keyboard_events_propagate = true;
		ir.Event.KeyEvent.wVirtualKeyCode = vkc_from_gtk_hw_keycode;
	}
#endif

	if ( (dwMods != 0 && event.GetUnicodeKey() < 32)
	  || (dwMods & (RIGHT_CTRL_PRESSED | LEFT_ALT_PRESSED)) != 0
	  || event.GetKeyCode() == WXK_DELETE || event.GetKeyCode() == WXK_RETURN
	  || (event.GetUnicodeKey()==WXK_NONE && !IsForcedCharTranslation(event.GetKeyCode()) )) {
		g_winport_con_in->Enqueue(&ir, 1);
		_last_keydown_enqueued = true;
	} 

#if defined(__WXGTK__) && !defined(__APPLE__) && !defined(__FreeBSD__) // only tested on Linux
	if (nonlatin_workaround) {
		ir.Event.KeyEvent.bKeyDown = TRUE;
		g_winport_con_in->Enqueue(&ir, 1);
		
		ir.Event.KeyEvent.bKeyDown = FALSE;
		g_winport_con_in->Enqueue(&ir, 1);
	}
#endif

	event.Skip();
}

void WinPortPanel::OnKeyUp( wxKeyEvent& event )
{
	const auto uni = event.GetUnicodeKey();
	fprintf(stderr, "OnKeyUp: raw=%x code=%x uni=%x (%lc) ts=%lu",
		event.GetRawKeyCode(), event.GetKeyCode(),
		uni, (uni > 0x1f) ? uni : L' ', event.GetTimestamp());

	_exclusive_hotkeys.OnKeyUp(event);

	if (event.GetSkipped()) {
		fprintf(stderr, " SKIPPED\n");
		return;
	}

	bool composing = _key_tracker.Composing();
	const bool was_pressed = _key_tracker.OnKeyUp(event);
	if (composing) {
		fprintf(stderr, " COMPOSING\n");
		event.Skip();
		return;
	}

	fprintf(stderr, was_pressed ? "\n" : " UNPAIRED\n");

#ifndef __WXOSX__ //on OSX some keyups come without corresponding keydowns
	if (was_pressed)
#endif
	{
		wx2INPUT_RECORD ir(FALSE, event, _key_tracker);

#if defined(__WXGTK__) && !defined(__APPLE__) && !defined(__FreeBSD__) // only tested on Linux
		const int vkc_from_gtk_hw_keycode =
			GTKHardwareKeyCodeToVirtualKeyCode(event.GetRawKeyFlags());
		const bool nonlatin_workaround = (true
			&& event.GetUnicodeKey() != 0    // key has unicode value => possibly character key
			&& vkc_from_gtk_hw_keycode       // but let's also check by hw keycode to be sure
			&& event.GetKeyCode() == 0       // and no keycode - so workaround is required
		);
		// for non-latin unicode keycode pressed with Alt key together
		// simulate some dummy key code for far2l to "see" keypress
		if (nonlatin_workaround) {
			// wow, we have Alt+NonLatinLetter keyboard events!
			// no need for accelerators hack anymore

			non_latin_keyboard_events_propagate = true;
			ir.Event.KeyEvent.wVirtualKeyCode = vkc_from_gtk_hw_keycode;
		}
#endif

#ifdef __WXOSX__ //on OSX some keyups come without corresponding keydowns
		if (!was_pressed) {
			ir.Event.KeyEvent.bKeyDown = FALSE;
			g_winport_con_in->Enqueue(&ir, 1);
			ir.Event.KeyEvent.bKeyDown = TRUE;
		}
#endif
		g_winport_con_in->Enqueue(&ir, 1);
	}
	if (_key_tracker.CheckForSuddenModifiersUp()) {
		_exclusive_hotkeys.Reset();
	}
	//event.Skip();
}

void WinPortPanel::OnChar( wxKeyEvent& event )
{
	const auto uni = event.GetUnicodeKey();
	fprintf(stderr, "OnChar: raw=%x code=%x uni=%x (%lc) ts=%lu lke=%u",
		event.GetRawKeyCode(), event.GetKeyCode(),
		uni, (uni > 0x1f) ? uni : L' ', event.GetTimestamp(), _last_keydown_enqueued);
	_exclusive_hotkeys.OnKeyUp(event);

	if (event.GetSkipped()) {
		fprintf(stderr, " SKIPPED\n");
		return;
	}
	fprintf(stderr, "\n");

	if (event.GetUnicodeKey() != WXK_NONE && 
		(!_last_keydown_enqueued || _key_tracker.LastKeydown().GetTimestamp() != event.GetTimestamp())) {
		INPUT_RECORD ir = {0};
		ir.EventType = KEY_EVENT;
		ir.Event.KeyEvent.wRepeatCount = 1;
		ir.Event.KeyEvent.wVirtualKeyCode = VK_OEM_PERIOD;
		if (event.GetUnicodeKey() <= 0x7f) { 
			if (_key_tracker.LastKeydown().GetTimestamp() == event.GetTimestamp()) {
				wx2INPUT_RECORD irx(TRUE, _key_tracker.LastKeydown(), _key_tracker);
				ir.Event.KeyEvent.wVirtualKeyCode = irx.Event.KeyEvent.wVirtualKeyCode;
				ir.Event.KeyEvent.dwControlKeyState = irx.Event.KeyEvent.dwControlKeyState;
			}
		}
		ir.Event.KeyEvent.uChar.UnicodeChar = event.GetUnicodeKey();

		ir.Event.KeyEvent.bKeyDown = TRUE;
		g_winport_con_in->Enqueue(&ir, 1);
		
		ir.Event.KeyEvent.bKeyDown = FALSE;
		g_winport_con_in->Enqueue(&ir, 1);
		
	}
	//event.Skip();
}


void WinPortPanel::OnPaint( wxPaintEvent& event )
{
	//fprintf(stderr, "WinPortPanel::OnPaint\n"); 
	_pending_refreshes = 0;
	if (_mouse_qedit_moved && _mouse_qedit_start_ticks != 0
	 && WINPORT(GetTickCount)() - _mouse_qedit_start_ticks > QEDIT_COPY_MINIMAL_DELAY) {
		SMALL_RECT qedit;
		qedit.Left = _mouse_qedit_start.X;
		qedit.Top = _mouse_qedit_start.Y;
		qedit.Right = _mouse_qedit_last.X;
		qedit.Bottom = _mouse_qedit_last.Y;
		NormalizeArea(qedit);
		_paint_context.OnPaint(&qedit);
	}
	else
		_paint_context.OnPaint();
	
}

void WinPortPanel::OnEraseBackground( wxEraseEvent& event )
{
}

void WinPortPanel::OnSize(wxSizeEvent &event)
{
	if (_resize_pending==RP_INSTANT) {
		CheckForResizePending();
	} else {
		_resize_pending = RP_DEFER;	
		//fprintf(stderr, "RP_DEFER\n");
	}
}

COORD WinPortPanel::TranslateMousePosition( wxMouseEvent &event )
{
	wxClientDC dc(this);
	wxPoint pos = event.GetLogicalPosition(dc);
	if (pos.x < 0) pos.x = 0;
	if (pos.y < 0) pos.y = 0;

	COORD out;
	out.X = (SHORT)(USHORT)(pos.x / _paint_context.FontWidth());
	out.Y = (SHORT)(USHORT)(pos.y / _paint_context.FontHeight());

	unsigned int width = 80, height = 25;
	g_winport_con_out->GetSize(width, height);
	
	if ( (USHORT)out.X >= width) out.X = width - 1;
	if ( (USHORT)out.Y >= height) out.Y = height - 1;
	return out;
}

void WinPortPanel::OnMouse( wxMouseEvent &event )
{
	COORD pos_char = TranslateMousePosition( event );
	
	DWORD mode = 0;
	if (!WINPORT(GetConsoleMode)(NULL, &mode))
		mode = 0;

	if ( (event.LeftDown() && !_last_mouse_event.LeftDown())
	  || (event.MiddleDown() && !_last_mouse_event.MiddleDown())
	  || (event.RightDown() && !_last_mouse_event.RightDown()) ) {
		_last_mouse_click = pos_char;
	}

	_last_mouse_event = event;

	if ((mode&ENABLE_QUICK_EDIT_MODE) || _adhoc_quickedit)
		OnMouseQEdit( event, pos_char );
	else if (mode&ENABLE_MOUSE_INPUT)
		OnMouseNormal( event, pos_char );
}

void WinPortPanel::OnMouseNormal( wxMouseEvent &event, COORD pos_char)
{
	INPUT_RECORD ir = {0};
	ir.EventType = MOUSE_EVENT;
	if (!g_broadway) {
		if (wxGetKeyState(WXK_SHIFT)) ir.Event.MouseEvent.dwControlKeyState|= SHIFT_PRESSED;
		if (wxGetKeyState(WXK_CONTROL)) ir.Event.MouseEvent.dwControlKeyState|= LEFT_CTRL_PRESSED;
		if (wxGetKeyState(WXK_ALT)) ir.Event.MouseEvent.dwControlKeyState|= LEFT_ALT_PRESSED;
	}
	if (event.LeftDown()) _mouse_state|= FROM_LEFT_1ST_BUTTON_PRESSED;
	else if (event.MiddleDown()) _mouse_state|= FROM_LEFT_2ND_BUTTON_PRESSED;
	else if (event.RightDown()) _mouse_state|=  RIGHTMOST_BUTTON_PRESSED;
	else if (event.LeftUp()) _mouse_state&= ~FROM_LEFT_1ST_BUTTON_PRESSED;
	else if (event.MiddleUp()) _mouse_state&= ~FROM_LEFT_2ND_BUTTON_PRESSED;
	else if (event.RightUp()) _mouse_state&= ~RIGHTMOST_BUTTON_PRESSED;
	else if (event.Moving() || event.Dragging()) ir.Event.MouseEvent.dwEventFlags|= MOUSE_MOVED;
	else if (event.GetWheelRotation()!=0) {
		if (event.GetWheelAxis()==wxMOUSE_WHEEL_HORIZONTAL)
			ir.Event.MouseEvent.dwEventFlags|= MOUSE_HWHEELED;
		else
			ir.Event.MouseEvent.dwEventFlags|= MOUSE_WHEELED;
		if (event.GetWheelRotation() > 0)
			ir.Event.MouseEvent.dwButtonState|= 0x00010000;
	} else if ( event.ButtonDClick() ) {
		
		if (event.ButtonDClick(wxMOUSE_BTN_LEFT))
			ir.Event.MouseEvent.dwButtonState|= FROM_LEFT_1ST_BUTTON_PRESSED;
		else if (event.ButtonDClick(wxMOUSE_BTN_MIDDLE))
			ir.Event.MouseEvent.dwButtonState|= FROM_LEFT_2ND_BUTTON_PRESSED;
		else if (event.ButtonDClick(wxMOUSE_BTN_RIGHT))
			ir.Event.MouseEvent.dwButtonState|= RIGHTMOST_BUTTON_PRESSED;
		else {
			fprintf(stderr, "Unsupported mouse double-click\n");
			return;			
		}
		ir.Event.MouseEvent.dwEventFlags|= DOUBLE_CLICK;
	} else {
		if (!event.Leaving() && !event.Entering() )
			fprintf(stderr, "Unsupported mouse event\n");
		return;
	}
	
	ir.Event.MouseEvent.dwButtonState|= _mouse_state;
	ir.Event.MouseEvent.dwMousePosition = pos_char;
	
	if (!(ir.Event.MouseEvent.dwEventFlags &MOUSE_MOVED) ) {
		fprintf(stderr, "Mouse: dwEventFlags=0x%x dwButtonState=0x%x dwControlKeyState=0x%x\n",
			ir.Event.MouseEvent.dwEventFlags, ir.Event.MouseEvent.dwButtonState, 
			ir.Event.MouseEvent.dwControlKeyState);
	}

	// GUI frontend tends to flood with duplicated mouse events,
	// cuz it reacts on screen coordinates movements, so avoid
	// excessive mouse events by skipping event if it duplicates
	// most recently queued event (fix #369)
	DWORD now = WINPORT(GetTickCount)();
	if ( (ir.Event.MouseEvent.dwEventFlags & (MOUSE_HWHEELED|MOUSE_WHEELED)) != 0
	 || _prev_mouse_event_ts + 500 <= now
	 || memcmp(&_prev_mouse_event, &ir.Event.MouseEvent, sizeof(_prev_mouse_event)) != 0) {
		memcpy(&_prev_mouse_event, &ir.Event.MouseEvent, sizeof(_prev_mouse_event));
		_prev_mouse_event_ts = now;
		g_winport_con_in->Enqueue(&ir, 1);
	}
}

void WinPortPanel::DamageAreaBetween(COORD c1, COORD c2)
{
	SMALL_RECT area = {c1.X, c1.Y, c2.X, c2.Y};
	OnConsoleOutputUpdated(&area, 1);
}

void WinPortPanel::OnMouseQEdit( wxMouseEvent &event, COORD pos_char )
{
	if (event.LeftDown()) {
		if (_mouse_qedit_start_ticks != 0) {
			DamageAreaBetween(_mouse_qedit_start, _mouse_qedit_last);
		}
		_mouse_qedit_start = _last_mouse_click;
		_mouse_qedit_last = pos_char;
		_mouse_qedit_start_ticks = WINPORT(GetTickCount)();
		if (!_mouse_qedit_start_ticks) _mouse_qedit_start_ticks = 1;
		_mouse_qedit_moved = false;
		DamageAreaBetween(_mouse_qedit_start, _mouse_qedit_last);
		
	} else if (_mouse_qedit_start_ticks != 0) {
		if (event.Moving() || event.Dragging()) {
			DamageAreaBetween(_mouse_qedit_start, _mouse_qedit_last);
			DamageAreaBetween(_mouse_qedit_start, pos_char);			
			_mouse_qedit_last = pos_char;
			_mouse_qedit_moved = true;

		} else if (event.LeftUp()) {
			if (_mouse_qedit_moved && WINPORT(GetTickCount)() - _mouse_qedit_start_ticks > QEDIT_COPY_MINIMAL_DELAY) {
				_text2clip.clear();
				USHORT y1 = _mouse_qedit_start.Y, y2 = pos_char.Y;
				USHORT x1 = _mouse_qedit_start.X, x2 = pos_char.X;
				if (y1 > y2) std::swap(y1, y2);
				if (x1 > x2) std::swap(x1, x2);

				COORD pos;
				for (pos.Y = y1; pos.Y<=y2; ++pos.Y) {
					if (!_text2clip.empty()) {
						_text2clip+= NATIVE_EOLW;
					}
					for (pos.X = x1; pos.X<=x2; ++pos.X) {
						CHAR_INFO ch;
						if (g_winport_con_out->Read(ch, pos))
							_text2clip+= ch.Char.UnicodeChar ? ch.Char.UnicodeChar : L' ';
					}
					if (y2 > y1) {
						while (!_text2clip.empty() && _text2clip[_text2clip.size() - 1] == ' ') {
							_text2clip.resize(_text2clip.size() - 1);
						}
					}
				}
				CheckPutText2CLip();
			}
			_adhoc_quickedit = false;
			_mouse_qedit_moved = false;
			_mouse_qedit_start_ticks = 0;
			DamageAreaBetween(_mouse_qedit_start, _mouse_qedit_last);
			DamageAreaBetween(_mouse_qedit_start, pos_char);
		}
	}
}


void WinPortPanel::OnConsoleAdhocQuickEditSync( wxCommandEvent& event )
{
	if (_adhoc_quickedit) {
		fprintf(stderr, "OnConsoleAdhocQuickEditSync: already\n");
		return;		
	}
	if ((_mouse_state & (FROM_LEFT_2ND_BUTTON_PRESSED | RIGHTMOST_BUTTON_PRESSED)) != 0) {
		fprintf(stderr, "OnConsoleAdhocQuickEditSync: inappropriate _mouse_state=0x%x\n", _mouse_state);
		return;
	}
	
	_adhoc_quickedit = true;
	
	if ( (_mouse_state & FROM_LEFT_1ST_BUTTON_PRESSED) != 0) {
		_mouse_state&= ~FROM_LEFT_1ST_BUTTON_PRESSED;
	
		COORD pos_char = TranslateMousePosition( _last_mouse_event );
		
		INPUT_RECORD ir = {0};
		ir.EventType = MOUSE_EVENT;
		ir.Event.MouseEvent.dwButtonState = _mouse_state;
		ir.Event.MouseEvent.dwMousePosition = pos_char;
		g_winport_con_in->Enqueue(&ir, 1);
		_last_mouse_event.SetEventType(wxEVT_LEFT_DOWN);
		_last_mouse_event.SetLeftDown(true);
		fprintf(stderr, "OnConsoleAdhocQuickEditSync: lbutton pressed, %u\n", _last_mouse_event.LeftIsDown());

		OnMouseQEdit( _last_mouse_event, pos_char );
	} else
		fprintf(stderr, "OnConsoleAdhocQuickEditSync: lbutton not pressed\n");
}

void WinPortPanel::OnConsoleAdhocQuickEdit()
{
	wxCommandEvent *event = new(std::nothrow) wxCommandEvent(WX_CONSOLE_ADHOC_QEDIT);
	if (event)
		wxQueueEvent(this, event);
}

void WinPortPanel::OnConsoleSetTweaksSync( wxCommandEvent& event )
{
	EventWithDWORD *e = (EventWithDWORD *)&event;
	_exclusive_hotkeys.SetTriggerKeys( (e->cookie & EXCLUSIVE_CTRL_LEFT) != 0,
		(e->cookie & EXCLUSIVE_CTRL_RIGHT) != 0, (e->cookie & EXCLUSIVE_ALT_LEFT) != 0,
		(e->cookie & EXCLUSIVE_ALT_RIGHT) != 0,  (e->cookie & EXCLUSIVE_WIN_LEFT) != 0,
		(e->cookie & EXCLUSIVE_WIN_RIGHT) != 0);

	_paint_context.SetSharp( (e->cookie & CONSOLE_PAINT_SHARP) != 0);
}

DWORD WinPortPanel::OnConsoleSetTweaks(DWORD tweaks)
{
	DWORD out = 0;

	if (_paint_context.IsSharpSupported())
		out|= TWEAK_STATUS_SUPPORT_PAINT_SHARP;
		
	if (_exclusive_hotkeys.Available())
		out|= TWEAK_STATUS_SUPPORT_EXCLUSIVE_KEYS;

	EventWithDWORD *event = new(std::nothrow) EventWithDWORD(tweaks, WX_CONSOLE_SET_TWEAKS);
	if (event)
		wxQueueEvent(this, event);

	return out;
}


bool WinPortPanel::OnConsoleIsActive()
{
	return _has_focus;
}

static std::string GetNotifySH()
{
	wxFileName fn(wxStandardPaths::Get().GetExecutablePath());
	wxString fn_str = fn.GetPath(wxPATH_GET_VOLUME | wxPATH_GET_SEPARATOR);
	std::string out(fn_str.mb_str());

	if (TranslateInstallPath_Bin2Share(out)) {
		out+= APP_BASENAME "/";
	}

	out+= "notify.sh";

	struct stat s;
	if (stat(out.c_str(), &s) == 0) {
		return out;
	}

	if (TranslateInstallPath_Share2Lib(out) && stat(out.c_str(), &s) == 0) {
		return out;
	}

	return std::string();
}

void WinPortPanel::OnConsoleDisplayNotification(const wchar_t *title, const wchar_t *text)
{
	static std::string s_notify_sh = GetNotifySH();
	if (s_notify_sh.empty()) {
		fprintf(stderr, "OnConsoleDisplayNotification: notify.sh not found\n");
		return;
	}

	const std::string &str_title = Wide2MB(title);
	const std::string &str_text = Wide2MB(text);

	pid_t pid = fork();
	if (pid == 0) {
		if (fork() == 0) {
			execl(s_notify_sh.c_str(), s_notify_sh.c_str(), str_title.c_str(), str_text.c_str(), NULL);
			perror("DisplayNotification - execl");
		}
		_exit(0);
		exit(0);

	} else if (pid != -1) {
		waitpid(pid, 0, 0);
	}
}

bool WinPortPanel::OnConsoleBackgroundMode(bool TryEnterBackgroundMode)
{
	return false;
}

void WinPortPanel::OnConsoleChangeFontSync(wxCommandEvent& event)
{
	_paint_context.ShowFontDialog();
	_resize_pending = RP_INSTANT;
	CheckForResizePending();
	Refresh();
}

void WinPortPanel::OnConsoleChangeFont()
{
	wxCommandEvent *event = new(std::nothrow) wxCommandEvent(WX_CONSOLE_CHANGE_FONT);
	if (event)
		wxQueueEvent(this, event);
}


void WinPortPanel::OnConsoleExitSync( wxCommandEvent& event )
{
	fprintf(stderr, "OnConsoleExitSync\n");
	wxTheApp->SetExitOnFrameDelete(true);
	_frame->Destroy();
	//wxTheApp->ExitMainLoop();
}

void WinPortPanel::OnConsoleExit()
{
	wxCommandEvent *event = new(std::nothrow) wxCommandEvent(WX_CONSOLE_EXIT);
	if (event)
		wxQueueEvent(this, event);
}

void WinPortPanel::CheckPutText2CLip()
{
	if (!_text2clip.empty())  {
		if (!WinPortClipboard_IsBusy()) {
			if (wxTheClipboard->Open()) {
				std::wstring text2clip; text2clip.swap(_text2clip);
				wxTheClipboard->SetData( new wxTextDataObject(text2clip) );
				wxTheClipboard->Close();
			}
		} else 
			fprintf(stderr, "CheckPutText2CLip: clipboard busy\n");
	}	
}

void WinPortPanel::OnSetFocus( wxFocusEvent &event )
{
	_has_focus = true;
}

void WinPortPanel::OnKillFocus( wxFocusEvent &event )
{
	fprintf(stderr, "OnKillFocus\n");
	_has_focus = false;
	_key_tracker.ForceAllUp();
	
	if (_mouse_qedit_start_ticks) {
		_mouse_qedit_start_ticks = 0;
		DamageAreaBetween(_mouse_qedit_start, _mouse_qedit_last);
	}
	
	_exclusive_hotkeys.Reset();
}
