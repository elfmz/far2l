# include "wxMain.h"

#define AREAS_REDUCTION

#define TIMER_ID     10

// interval of timer that used to blink cursor and do some other things
#define TIMER_PERIOD 500             // 0.5 second

// time interval that used for deferred extra refresh after last title update
// see comment on WinPortPanel::OnTitleChangedSync
#define TIMER_EXTRA_REFRESH 100   // 0.1 second

// how many timer ticks may pass since last input activity
// before timer will be stopped until restarted by some activity
#define TIMER_IDLING_CYCLES 60  // 0.5 second * 60 = 30 seconds

// If time between adhoc text copy and mouse button release less then this value then text will not be copied. Used to protect against unwanted copy-paste-s
#define QEDIT_COPY_MINIMAL_DELAY 150

#if (wxCHECK_VERSION(3, 0, 5) || (wxCHECK_VERSION(3, 0, 4) && WX304PATCH)) && !(wxCHECK_VERSION(3, 1, 0) && !wxCHECK_VERSION(3, 1, 3))
    // wx version is greater than 3.0.5 (3.0.4 on Ubuntu 20) and not in 3.1.0-3.1.2
    #define WX_ALT_NONLATIN
#endif

IConsoleOutput *g_winport_con_out = nullptr;
IConsoleInput *g_winport_con_in = nullptr;
bool g_broadway = false, g_wayland = false, g_remote = false;
static int g_exit_code = 0;
static int g_maximize = 0;
static WinPortAppThread *g_winport_app_thread = NULL;
static WinPortFrame *g_winport_frame = nullptr;

bool WinPortClipboard_IsBusy();


static void NormalizeArea(SMALL_RECT &area)
{
	if (area.Left > area.Right) std::swap(area.Left, area.Right);
	if (area.Top > area.Bottom) std::swap(area.Top, area.Bottom);	
}

WinPortAppThread::WinPortAppThread(int argc, char **argv, int(*appmain)(int argc, char **argv))
	: wxThread(wxTHREAD_DETACHED), _backend(nullptr), _argv(argv), _argc(argc), _appmain(appmain)
{
}

wxThreadError WinPortAppThread::Start(IConsoleOutputBackend *backend)
{
	_backend = backend;
	return Run();
}

wxThread::ExitCode WinPortAppThread::Entry()
{
	g_exit_code = _appmain(_argc, _argv);
	_backend->OnConsoleExit();
	//exit(_r);
	return 0;
}

///////////

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

	const char *xrdp = getenv("XRDP_SESSION");
	if (xrdp) {
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

	bool primary_selection = false;
	for (int i = 0; i < a->argc; ++i) {
		if (strcmp(a->argv[i], "--primary-selection") == 0) {
			primary_selection = true;

		} else if (strcmp(a->argv[i], "--maximize") == 0) {
			g_maximize = 1;

		} else if (strcmp(a->argv[i], "--nomaximize") == 0) {
			g_maximize = -1;
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

struct WinState
{
	wxPoint pos {40, 40};
	wxSize size {600, 440};
	bool maximized{false};
	bool fullscreen{false};

	WinState()
	{
		std::ifstream is;
		is.open(InMyConfig("winstate").c_str());
		if (!is.is_open()) {
			fprintf(stderr, "WinState: can't open\n");
			return;
		}
		std::string str;
		getline (is, str);
		int i = atoi(str.c_str());
		if ((i & 1) == 0) {
			fprintf(stderr, "WinState: bad flags field [%d]\n", i);
			return;
		}
		maximized = (i & 2) != 0;
		fullscreen = (i & 4) != 0;

		getline(is, str);
		i = atoi(str.c_str());
		if (i >= 100) {
			size.SetWidth(i);
		}

		getline(is, str);
		i = atoi(str.c_str());
		if (i >= 100) {
			size.SetHeight(i);
		}

		getline(is, str);
		pos.x = atoi(str.c_str());
		getline(is, str);
		pos.y = atoi(str.c_str());
	}

	void Save()
	{
		std::ofstream os;
		os.open(InMyConfig("winstate").c_str());
		if (!os.is_open()) {
			fprintf(stderr, "WinState: can't create\n");
		}
		int flags = 1;
		if (maximized) flags|= 2;
		if (fullscreen) flags|= 4;
		os << flags << std::endl;
		os << size.GetWidth() << std::endl;
		os << size.GetHeight() << std::endl;
		os << pos.x << std::endl;
		os << pos.y << std::endl;
		fprintf(stderr, "WinState: saved flags=%d size={%d, %d} pos={%d, %d}\n",
			flags, size.GetWidth(), size.GetHeight(), pos.x, pos.y);
	}
};


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
typedef EventWith<DWORD64> EventWithDWORD64;

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
wxDEFINE_EVENT(WX_CONSOLE_SAVE_WIN_STATE, wxCommandEvent);
wxDEFINE_EVENT(WX_CONSOLE_EXIT, wxCommandEvent);


////////////////////////////////////////// app

wxIMPLEMENT_APP_NO_MAIN(WinPortApp);

wxEvtHandler *WinPort_EventHandler()
{
	if (!g_winport_frame) {
		return wxTheApp->GetTopWindow()->GetEventHandler();
	}

	return g_winport_frame->GetEventHandler();
}

bool WinPortApp::OnInit()
{
	g_winport_frame = new WinPortFrame("WinPortApp");
//    WinPortFrame *frame = new WinPortFrame( "WinPortApp", wxPoint(50, 50), wxSize(800, 600) );
	g_winport_frame->Show( true );
	if (g_broadway)
		g_winport_frame->Maximize();
		
	return true;
}

////////////////////////////////////////// frame

wxBEGIN_EVENT_TABLE(WinPortFrame, wxFrame)
	EVT_ERASE_BACKGROUND(WinPortFrame::OnEraseBackground)
	EVT_PAINT(WinPortFrame::OnPaint)
	EVT_SHOW(WinPortFrame::OnShow)
	EVT_CLOSE(WinPortFrame::OnClose)
	EVT_CHAR(WinPortFrame::OnChar)
	EVT_MENU_RANGE(ID_CTRL_BASE, ID_CTRL_END, WinPortFrame::OnAccelerator)
	EVT_MENU_RANGE(ID_CTRL_SHIFT_BASE, ID_CTRL_SHIFT_END, WinPortFrame::OnAccelerator)
	EVT_MENU_RANGE(ID_ALT_BASE, ID_ALT_END, WinPortFrame::OnAccelerator)
wxEND_EVENT_TABLE()

WinPortFrame::WinPortFrame(const wxString& title)
        : _shown(false),  _menu_bar(nullptr)
{
	WinState ws;

	long style = wxDEFAULT_FRAME_STYLE;
	if (g_maximize >= 0 && (ws.maximized || g_maximize > 0)) {
		style|= wxMAXIMIZE;
	}

	int disp_index = wxDisplay::GetFromPoint(ws.pos);
	if (disp_index < 0 || disp_index >= (int)wxDisplay::GetCount()) {
		disp_index = 0;
	}

	wxDisplay disp(disp_index);
	wxRect rc = disp.GetClientArea();
	fprintf(stderr, "WinPortFrame: display %d from %d.%d area %d.%d - %d.%d\n",
		disp_index, ws.pos.x, ws.pos.y, rc.GetLeft(), rc.GetTop(), rc.GetRight(), rc.GetBottom());

	if (ws.size.GetWidth() > rc.GetWidth()) {
		ws.size.SetWidth(rc.GetWidth());
	}
	if (ws.size.GetHeight() > rc.GetHeight()) {
		ws.size.SetHeight(rc.GetHeight());
	}
	if (ws.pos.x + ws.size.GetWidth() > rc.GetRight()) {
		ws.pos.x = rc.GetRight() - ws.size.GetWidth();
	}
	if (ws.pos.y + ws.size.GetHeight() > rc.GetBottom()) {
		ws.pos.y = rc.GetBottom() - ws.size.GetHeight();
	}
	if (ws.pos.x < rc.GetLeft()) {
		ws.pos.x = rc.GetLeft();
	}
	if (ws.pos.y < rc.GetTop()) {
		ws.pos.y = rc.GetTop();
	}

	// far2l doesn't need special erase background
	SetBackgroundStyle(wxBG_STYLE_PAINT);
	Create(NULL, wxID_ANY, title, ws.pos, ws.size, style);
	SetBackgroundColour(*wxBLACK);

	_panel = new WinPortPanel(this, wxPoint(0, 0), GetClientSize());
	_panel->SetFocus();

#ifdef __APPLE__
	if (style & wxMAXIMIZE) { // under MacOS wxMAXIMIZE doesn't do what should do...
		Maximize();
	}
#endif
	if (ws.fullscreen && g_maximize >= 0) {
		ShowFullScreen(true);
	}
}

WinPortFrame::~WinPortFrame()
{
	SetMenuBar(nullptr);
	delete _menu_bar;
	delete _panel;
	_panel = NULL;
}

void WinPortFrame::OnEraseBackground(wxEraseEvent &event)
{
}

void WinPortFrame::OnPaint(wxPaintEvent &event)
{
	wxPaintDC dc(this);
}

void WinPortFrame::OnChar(wxKeyEvent &event)
{
	_panel->OnChar(event);
}

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

#ifndef WX_ALT_NONLATIN
		menu = new wxMenu;
		for (char c = 'A'; c <= 'Z'; ++c) {
			sprintf(str, "Alt + %c\tAlt+%c", c, c);
			menu->Append(ID_ALT_BASE + (c - 'A'), wxString(str));
		}
		_menu_bar->Append(menu, _T("Alt + ?"));
#endif
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

void WinPortFrame::OnAccelerator(wxCommandEvent& event)
{
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

////////////////////////////////////////// panel

wxBEGIN_EVENT_TABLE(WinPortPanel, wxPanel)
	EVT_COMMAND(wxID_ANY, WX_CONSOLE_INITIALIZED, WinPortPanel::OnInitialized)
	EVT_TIMER(TIMER_ID, WinPortPanel::OnTimerPeriodic)
	EVT_COMMAND(wxID_ANY, WX_CONSOLE_REFRESH, WinPortPanel::OnRefreshSync)
	EVT_COMMAND(wxID_ANY, WX_CONSOLE_WINDOW_MOVED, WinPortPanel::OnWindowMovedSync)
	EVT_COMMAND(wxID_ANY, WX_CONSOLE_RESIZED, WinPortPanel::OnConsoleResizedSync)
	EVT_COMMAND(wxID_ANY, WX_CONSOLE_TITLE_CHANGED, WinPortPanel::OnTitleChangedSync)
	EVT_COMMAND(wxID_ANY, WX_CONSOLE_SET_MAXIMIZED, WinPortPanel::OnSetMaximizedSync)
	EVT_COMMAND(wxID_ANY, WX_CONSOLE_ADHOC_QEDIT, WinPortPanel::OnConsoleAdhocQuickEditSync)
	EVT_COMMAND(wxID_ANY, WX_CONSOLE_SET_TWEAKS, WinPortPanel::OnConsoleSetTweaksSync)
	EVT_COMMAND(wxID_ANY, WX_CONSOLE_CHANGE_FONT, WinPortPanel::OnConsoleChangeFontSync)
	EVT_COMMAND(wxID_ANY, WX_CONSOLE_SAVE_WIN_STATE, WinPortPanel::OnConsoleSaveWindowStateSync)
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


///////////////////////////


WinPortPanel::WinPortPanel(WinPortFrame *frame, const wxPoint& pos, const wxSize& size)
        :
		_paint_context(this), _has_focus(true), _prev_mouse_event_ts(0), _frame(frame), _periodic_timer(NULL),
		_last_keydown_enqueued(false), _initialized(false), _adhoc_quickedit(false),
		_resize_pending(RP_NONE),  _mouse_state(0), _mouse_qedit_start_ticks(0), _mouse_qedit_moved(false),
		_refresh_rects_throttle(0), _pending_refreshes(0), _timer_idling_counter(0), _stolen_key(0)
{
	// far2l doesn't need special erase background
	SetBackgroundStyle(wxBG_STYLE_PAINT);
	Create(frame, wxID_ANY, pos, size, wxWANTS_CHARS | wxNO_BORDER);
	SetBackgroundColour(*wxBLACK);

	g_winport_con_out->SetBackend(this);
	_periodic_timer = new wxTimer(this, TIMER_ID);
	_periodic_timer->Start(TIMER_PERIOD);
	OnConsoleOutputTitleChanged();
	_resize_pending = RP_INSTANT;
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
	_initialized = true;
	SetConsoleSizeFromWindow();

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
	}

#ifdef __APPLE__
	return Touchbar_SetTitles(titles);
#else
	return false;
#endif
}

BYTE WinPortPanel::OnConsoleGetColorPalette()
{
	return 24;
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

void WinPortPanel::SetConsoleSizeFromWindow()
{
	unsigned int prev_width = 0, prev_height = 0;

	g_winport_con_out->GetSize(prev_width, prev_height);

	int width = 0, height = 0;
	_frame->GetClientSize(&width, &height);
	const unsigned int font_width = _paint_context.FontWidth();
	const unsigned int font_height = _paint_context.FontHeight();
#ifndef __APPLE__
	fprintf(stderr, "Current client size: %u %u font %u %u\n",
		width, height, font_width, font_height);
#endif
	width/= font_width;
	height/= font_height;
	if (width != (int)prev_width || height != (int)prev_height) {
		fprintf(stderr, "Changing size: %u x %u\n", width, height);
#ifdef __APPLE__
		SetSize(width * font_width, height * font_height);
#endif
		g_winport_con_out->SetSize(width, height);
		INPUT_RECORD ir = {0};
		ir.EventType = WINDOW_BUFFER_SIZE_EVENT;
		ir.Event.WindowBufferSizeEvent.dwSize.X = width;
		ir.Event.WindowBufferSizeEvent.dwSize.Y = height;
		g_winport_con_in->Enqueue(&ir, 1);
	}
}

void WinPortPanel::CheckForResizePending()
{
#ifndef __APPLE__
	if (_initialized && _resize_pending!=RP_NONE)
#endif
	{
#ifndef __APPLE__
		fprintf(stderr, "%lu CheckForResizePending\n", GetProcessUptimeMSec());
#endif
		DWORD conmode = 0;
		if (WINPORT(GetConsoleMode)(NULL, &conmode) && (conmode&ENABLE_WINDOW_INPUT)!=0) {
			_resize_pending = RP_NONE;
			SetConsoleSizeFromWindow();
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
	if (_extra_refresh) {
		// see comment on WinPortPanel::OnTitleChangedSync
		if (WINPORT(GetTickCount)() - _last_title_ticks > TIMER_EXTRA_REFRESH) {
			_periodic_timer->Stop();
			_extra_refresh = false;
			Refresh();
			_periodic_timer->Start(TIMER_PERIOD);
			fprintf(stderr, "Extra refresh\n");
		}
		return;
	}

	CheckForResizePending();
	CheckPutText2CLip();	
	if (_mouse_qedit_start_ticks != 0 && WINPORT(GetTickCount)() - _mouse_qedit_start_ticks > QEDIT_COPY_MINIMAL_DELAY) {
		DamageAreaBetween(_mouse_qedit_start, _mouse_qedit_last);
	}
	_paint_context.BlinkCursor();
	++_timer_idling_counter;
	// stop timer if counter reached limit and cursor is visible and no other timer-dependent things remained
	if (_timer_idling_counter >= TIMER_IDLING_CYCLES && _paint_context.CursorBlinkState() && _text2clip.empty()) {
		_periodic_timer->Stop();
	}
}

void WinPortPanel::ResetTimerIdling()
{
	if (_timer_idling_counter >= TIMER_IDLING_CYCLES && !_periodic_timer->IsRunning()) {
		_periodic_timer->Start(_extra_refresh ? TIMER_EXTRA_REFRESH : TIMER_PERIOD);

	} else if (_extra_refresh) {
		_periodic_timer->Stop();
		_periodic_timer->Start(TIMER_EXTRA_REFRESH);
	}

	_timer_idling_counter = 0;
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

COORD WinPortPanel::OnConsoleGetLargestWindowSize()
{
	if (!wxIsMainThread()){
		auto fn = std::bind(&WinPortPanel::OnConsoleGetLargestWindowSize, this);
		return CallInMain<COORD>(fn);
	}
	
	wxSize sz = GetClientSize();
	if (_frame->IsMaximized()) {
		return COORD{(SHORT)(sz.GetWidth() / _paint_context.FontWidth()),
			(SHORT)(sz.GetHeight() / _paint_context.FontHeight())};
	}


	int disp_index = wxDisplay::GetFromWindow(this);
	if (disp_index < 0 || disp_index >= (int)wxDisplay::GetCount()) {
		fprintf(stderr, "OnConsoleGetLargestWindowSize: bad display %d\n", disp_index);
		disp_index = 0;
	}

	wxDisplay disp(disp_index);
	wxRect rc_disp = disp.GetClientArea();
	wxSize sz_frame = _frame->GetSize();

	return COORD {SHORT((rc_disp.GetWidth() - (sz_frame.GetWidth() - sz.GetWidth())) / _paint_context.FontWidth()),
		SHORT((rc_disp.GetHeight() - (sz_frame.GetHeight() - sz.GetHeight())) / _paint_context.FontHeight())};
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

#ifndef __APPLE__
	// see comment on WinPortPanel::OnTitleChangedSync
	if (WINPORT(GetTickCount)() - _last_title_ticks < TIMER_EXTRA_REFRESH && !_extra_refresh) {
		_extra_refresh = true;
		ResetTimerIdling();
	}
#endif

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
	// Workaround for #1303 #1454:
	// When running in X session there is floating bug somewhere in WX or GTK or even XOrg that
	// causes window repaint to be lost randomly in case repaint happened right after title change.
	// In case of far2l title change happens together with content changes that causes this issue.
	// So workaround is to detect if some refresh happened just after title changed and repeat
	// do extra refresh just in case. Note that 'just after' means 'during TIMER_EXTRA_REFRESH'.
	const std::wstring &title = g_winport_con_out->GetTitle();
	wxGetApp().SetAppDisplayName(title.c_str());
	_frame->SetTitle(title.c_str());
	_last_title_ticks = WINPORT(GetTickCount)();
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

void WinPortPanel::OnKeyDown( wxKeyEvent& event )
{
	ResetTimerIdling();
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

#ifdef __APPLE__
	if (!event.RawControlDown() && !event.ShiftDown() && !event.MetaDown() && !event.AltDown() && event.CmdDown()
		&& (uni == 'H' || uni == 'Q')) {
		fprintf(stderr, " Cmd+%lc\n", uni);
		ResetInputState();
		event.Skip();
		_stolen_key = uni;
		if (uni == 'Q') {
			_frame->Close();
		} else {
			MacHide();
			//_frame->Hide();
		}
		return;
	}
#endif
	_stolen_key = 0;

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

#ifdef WX_ALT_NONLATIN
	const bool alt_nonlatin_workaround = (
		(dwMods & (LEFT_ALT_PRESSED | LEFT_CTRL_PRESSED)) == LEFT_ALT_PRESSED
		&& event.GetUnicodeKey() != 0 && ir.Event.KeyEvent.wVirtualKeyCode == 0);
	// for non-latin unicode keycode pressed with Alt key together
	// simulate some dummy key code for far2l to "see" keypress
	if (alt_nonlatin_workaround) {
		ir.Event.KeyEvent.wVirtualKeyCode = VK_UNASSIGNED;
	}
#endif

	if ( (dwMods != 0 && event.GetUnicodeKey() < 32)
	  || (dwMods & (RIGHT_CTRL_PRESSED | LEFT_ALT_PRESSED)) != 0
	  || event.GetKeyCode() == WXK_DELETE || event.GetKeyCode() == WXK_RETURN
	  || (event.GetUnicodeKey()==WXK_NONE && !IsForcedCharTranslation(event.GetKeyCode()) )) {
		g_winport_con_in->Enqueue(&ir, 1);
		_last_keydown_enqueued = true;
	} 

#ifdef WX_ALT_NONLATIN
	if (alt_nonlatin_workaround) {
		OnChar(event);
	}
#endif

	event.Skip();
}

void WinPortPanel::OnKeyUp( wxKeyEvent& event )
{
	ResetTimerIdling();
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

	if (_stolen_key && _stolen_key == uni) {
		fprintf(stderr, " STOLEN\n");
		event.Skip();
		return;
	}

	fprintf(stderr, was_pressed ? "\n" : " UNPAIRED\n");

#ifndef __WXOSX__ //on OSX some keyups come without corresponding keydowns
	if (was_pressed)
#endif
	{
		wx2INPUT_RECORD ir(FALSE, event, _key_tracker);
#ifdef WX_ALT_NONLATIN
		const DWORD &dwMods = (ir.Event.KeyEvent.dwControlKeyState
			& (LEFT_ALT_PRESSED | SHIFT_PRESSED | LEFT_CTRL_PRESSED | RIGHT_CTRL_PRESSED));
		const bool alt_nonlatin_workaround = (
			(dwMods & (LEFT_ALT_PRESSED | LEFT_CTRL_PRESSED)) == LEFT_ALT_PRESSED
			&& event.GetUnicodeKey() != 0 && ir.Event.KeyEvent.wVirtualKeyCode == 0);
		// for non-latin unicode keycode pressed with Alt key together
		// simulate some dummy key code for far2l to "see" keypress
		if (alt_nonlatin_workaround) {
			ir.Event.KeyEvent.wVirtualKeyCode = VK_UNASSIGNED;
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
	ResetTimerIdling();
	const auto uni = event.GetUnicodeKey();
	fprintf(stderr, "OnChar: raw=%x code=%x uni=%x (%lc) ts=%lu lke=%u",
		event.GetRawKeyCode(), event.GetKeyCode(),
		uni, (uni > 0x1f) ? uni : L' ', event.GetTimestamp(), _last_keydown_enqueued);
	_exclusive_hotkeys.OnKeyUp(event);

	if (event.GetSkipped()) {
		fprintf(stderr, " SKIPPED\n");
		return;
	}
	if (_stolen_key && _stolen_key == uni) {
		fprintf(stderr, " STOLEN\n");
		event.Skip();
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
		ResetTimerIdling();
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
	ResetTimerIdling();

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
						if (g_winport_con_out->Read(ch, pos)) {
							if (CI_USING_COMPOSITE_CHAR(ch)) {
								_text2clip+= WINPORT(CompositeCharLookup)(ch.Char.UnicodeChar);
							} else if (ch.Char.UnicodeChar) {
								_text2clip+= ch.Char.UnicodeChar;
							}
						}
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
	EventWithDWORD64 *e = (EventWithDWORD64 *)&event;
	_exclusive_hotkeys.SetTriggerKeys( (e->cookie & EXCLUSIVE_CTRL_LEFT) != 0,
		(e->cookie & EXCLUSIVE_CTRL_RIGHT) != 0, (e->cookie & EXCLUSIVE_ALT_LEFT) != 0,
		(e->cookie & EXCLUSIVE_ALT_RIGHT) != 0,  (e->cookie & EXCLUSIVE_WIN_LEFT) != 0,
		(e->cookie & EXCLUSIVE_WIN_RIGHT) != 0);

	_paint_context.SetSharp( (e->cookie & CONSOLE_PAINT_SHARP) != 0);
}

DWORD64 WinPortPanel::OnConsoleSetTweaks(DWORD64 tweaks)
{
	DWORD64 out = TWEAK_STATUS_SUPPORT_CHANGE_FONT;

	if (_paint_context.IsSharpSupported())
		out|= TWEAK_STATUS_SUPPORT_PAINT_SHARP;
		
	if (_exclusive_hotkeys.Available())
		out|= TWEAK_STATUS_SUPPORT_EXCLUSIVE_KEYS;

	EventWithDWORD64 *event = new(std::nothrow) EventWithDWORD64(tweaks, WX_CONSOLE_SET_TWEAKS);
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

void WinPortPanel::OnConsoleSaveWindowStateSync(wxCommandEvent& event)
{
	if (_frame->IsShown()) {
		WinState ws;
		ws.maximized = _frame->IsMaximized();
		ws.fullscreen = _frame->IsFullScreen();

		if (!ws.maximized && !ws.fullscreen) {
			ws.pos = _frame->GetPosition();
			ws.size = _frame->GetSize();

			// align saved size by font dimensions to avoid blank edges on next start
			int width = 0, height = 0;
			_frame->GetClientSize(&width, &height);
			const unsigned int font_width = _paint_context.FontWidth();
			const unsigned int font_height = _paint_context.FontHeight();
			if (width % font_width) {
				ws.size.SetWidth(ws.size.GetWidth() - (width % font_width));
			}
			if (height % font_height) {
				ws.size.SetHeight(ws.size.GetHeight() - (height % font_height));
			}

		} else {
			// if window maximized on different display - have to save its position anyway
			// however dont save frame's position in such case but save workarea left.top instead
			// cuz maximized window's pos can be outside of related display
			const int prev_disp_index = wxDisplay::GetFromPoint(ws.pos);
			const int disp_index = wxDisplay::GetFromWindow(_frame);
//			fprintf(stderr, "prev_disp_index=%d disp_index=%d\n", prev_disp_index, disp_index);
			if (prev_disp_index != disp_index && disp_index >= 0 && disp_index < (int)wxDisplay::GetCount()) {
				wxDisplay disp(disp_index);
				wxRect rc = disp.GetClientArea();
				ws.pos.x = rc.GetLeft();
				ws.pos.y = rc.GetTop();
			}
		}
		ws.Save();
	}
}

void WinPortPanel::OnConsoleSaveWindowState()
{
	wxCommandEvent *event = new(std::nothrow) wxCommandEvent(WX_CONSOLE_SAVE_WIN_STATE);
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
	ResetTimerIdling();
}

void WinPortPanel::OnKillFocus( wxFocusEvent &event )
{
	fprintf(stderr, "OnKillFocus\n");
	_has_focus = false;
	ResetInputState();
}

void WinPortPanel::ResetInputState()
{
	_key_tracker.ForceAllUp();
	
	if (_mouse_qedit_start_ticks) {
		_mouse_qedit_start_ticks = 0;
		DamageAreaBetween(_mouse_qedit_start, _mouse_qedit_last);
	}
	
	_exclusive_hotkeys.Reset();
}
