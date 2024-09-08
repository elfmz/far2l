#include "wxMain.h"

#define AREAS_REDUCTION

#define TIMER_ID     10

// interval of timer that used to blink cursor and do some other things
#define DEF_TIMER_PERIOD 500  // 0.1 second
#define MIN_TIMER_PERIOD 100  // 0.1 second
#define MAX_TIMER_PERIOD 500  // 0.5 second

// time interval that used for deferred extra refresh after last title update
// see comment on WinPortPanel::OnTitleChangedSync
#define TIMER_EXTRA_REFRESH 100        // 0.1 second

// how many timer ticks may pass since last input activity
// before timer will be stopped until restarted by some activity
//#define TIMER_IDLING_CYCLES 600       // 0.5 second * 60 = 30 seconds
#define TIMER_IDLING_TIME (60000 * 3)  // 3 minutes


// If time between adhoc text copy and mouse button release less then this value then text will not be copied. Used to protect against unwanted copy-paste-s
#define QEDIT_COPY_MINIMAL_DELAY 150

#if (wxCHECK_VERSION(3, 0, 5) || (wxCHECK_VERSION(3, 0, 4) && WX304PATCH)) && !(wxCHECK_VERSION(3, 1, 0) && !wxCHECK_VERSION(3, 1, 3))
	// wx version is greater than 3.0.5 (3.0.4 on Ubuntu 20) and not in 3.1.0-3.1.2
	#define WX_ALT_NONLATIN
#endif

//30000

IConsoleOutput *g_winport_con_out = nullptr;
IConsoleInput *g_winport_con_in = nullptr;
bool g_broadway = false, g_wayland = false, g_remote = false;

static int g_exit_code = 0;
static int g_maximize = 0;
static WinPortAppThread *g_winport_app_thread = NULL;
static WinPortFrame *g_winport_frame = nullptr;

static DWORD g_TIMER_PERIOD = DEF_TIMER_PERIOD;
static DWORD g_TIMER_IDLING_CYCLES = TIMER_IDLING_TIME / DEF_TIMER_PERIOD;

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

bool WinPortAppThread::Prepare()
{
	_start.lock();
	return Run() == wxTHREAD_NO_ERROR;
}

void WinPortAppThread::Start(IConsoleOutputBackend *backend)
{
	_backend = backend;
	_start.unlock();
}

wxThread::ExitCode WinPortAppThread::Entry()
{
	_start.lock();
	g_exit_code = _appmain(_argc, _argv);
	_start.unlock();
	_backend->OnConsoleExit();
	//exit(_r);
	return 0;
}

///////////

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

	const char *wayland_display = getenv("WAYLAND_DISPLAY");
	if (wayland_display) {
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

#ifdef __APPLE__
void MacInit();
#endif

extern "C" __attribute__ ((visibility("default"))) bool WinPortMainBackend(WinPortMainBackendArg *a)
{
	if (a->abi_version != FAR2L_BACKEND_ABI_VERSION) {
		fprintf(stderr, "This far2l_gui is not compatible and cannot be used\n");
		return false;
	}
#ifdef __APPLE__
	MacInit();
#endif


	g_wx_norgb = a->norgb;
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

	g_wx_palette = g_winport_palette;

	ClipboardBackendSetter clipboard_backend_setter;
	if (!a->ext_clipboard) {
		clipboard_backend_setter.Set<wxClipboardBackend>();
	}

	if (a->app_main && !g_winport_app_thread) {
		g_winport_app_thread = new(std::nothrow) WinPortAppThread(a->argc, a->argv, a->app_main);
		if (UNLIKELY(!g_winport_app_thread) || UNLIKELY(!g_winport_app_thread->Prepare())) {
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

WinState::WinState()
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
		fprintf(stderr, "WinState: bad flags [%d]\n", i);
		return;
	}
	maximized = (i & 2) != 0;
	fullscreen = (i & 4) != 0;

	getline(is, str);
	int width = atoi(str.c_str());
	if (width >= 100)
		size.SetWidth(width);

	getline(is, str);
	int height = atoi(str.c_str());
	if (height >= 100)
		size.SetHeight(height);

	// interpret negative widht/height values as size in chars
	if (width < 0 && height < 0) {
		charSize.SetWidth(-width);
		charSize.SetHeight(-height);
	}

	getline(is, str);
	pos.x = atoi(str.c_str());
	getline(is, str);
	pos.y = atoi(str.c_str());
}

void WinState::Save()
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
	if (charSize.GetWidth() > 0 && charSize.GetHeight() > 0) {
		// preserve size in chars if both original widht/height values are negative
		os << -charSize.GetWidth() << std::endl;
		os << -charSize.GetHeight() << std::endl;
	}
	else {
		os << size.GetWidth() << std::endl;
		os << size.GetHeight() << std::endl;
	}
	os << pos.x << std::endl;
	os << pos.y << std::endl;
	fprintf(stderr, "WinState: saved flags=%d size={%d, %d} pos={%d, %d}\n",
		flags, size.GetWidth(), size.GetHeight(), pos.x, pos.y);
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
wxDEFINE_EVENT(WX_CONSOLE_SET_CURSOR_BLINK_TIME, wxCommandEvent);
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
	g_winport_frame = new WinPortFrame(APP_BASENAME);
//	WinPortFrame *frame = new WinPortFrame( "WinPortApp", wxPoint(50, 50), wxSize(800, 600) );
	g_winport_frame->Show( true );
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

	EVT_COMMAND(wxID_ANY, WX_CONSOLE_SAVE_WIN_STATE, WinPortFrame::OnConsoleSaveWindowStateSync)
wxEND_EVENT_TABLE()

WinPortFrame::WinPortFrame(const wxString& title)
	: _shown(false),  _menu_bar(nullptr)
{
	long style = wxDEFAULT_FRAME_STYLE;
	if (g_maximize >= 0 && (_win_state.maximized || g_maximize > 0 || g_broadway)) {
		style|= wxMAXIMIZE;
	}

	int disp_index = wxDisplay::GetFromPoint(_win_state.pos);
	if (disp_index < 0 || disp_index >= (int)wxDisplay::GetCount()) {
		disp_index = 0;
	}

	wxDisplay disp(disp_index);
	wxRect rc = disp.GetClientArea();
	fprintf(stderr, "WinPortFrame: display %d from %d.%d area %d.%d - %d.%d\n",
		disp_index, _win_state.pos.x, _win_state.pos.y, rc.GetLeft(), rc.GetTop(), rc.GetRight(), rc.GetBottom());

	if (_win_state.size.GetWidth() > rc.GetWidth()) {
		_win_state.size.SetWidth(rc.GetWidth());
	}
	if (_win_state.size.GetHeight() > rc.GetHeight()) {
		_win_state.size.SetHeight(rc.GetHeight());
	}
	if (_win_state.pos.x + _win_state.size.GetWidth() > rc.GetRight()) {
		_win_state.pos.x = rc.GetRight() - _win_state.size.GetWidth();
	}
	if (_win_state.pos.y + _win_state.size.GetHeight() > rc.GetBottom()) {
		_win_state.pos.y = rc.GetBottom() - _win_state.size.GetHeight();
	}
	if (_win_state.pos.x >= 0 && _win_state.pos.x < rc.GetLeft()) {
		_win_state.pos.x = rc.GetLeft();
	}
	if (_win_state.pos.y >= 0 && _win_state.pos.y < rc.GetTop()) {
		_win_state.pos.y = rc.GetTop();
	}

	// far2l doesn't need special erase background
	SetBackgroundStyle(wxBG_STYLE_PAINT);
	const auto bkclr = g_wx_palette.background[0];
	SetBackgroundColour(wxColour(bkclr.r, bkclr.g, bkclr.b));
	Create(NULL, wxID_ANY, title, _win_state.pos, _win_state.size, style);
	_panel = new WinPortPanel(this, wxPoint(0, 0), GetClientSize());
	_panel->SetFocus();

#ifdef __APPLE__
	if (style & wxMAXIMIZE) { // under MacOS wxMAXIMIZE doesn't do what should do...
		Maximize();
	}
#endif
	if (_win_state.fullscreen && g_maximize >= 0) {
		ShowFullScreen(true);
	}
}

WinPortFrame::~WinPortFrame()
{
	SetMenuBar(nullptr);
	delete _menu_bar;
	delete _panel;
	_panel = NULL;
	g_winport_frame = nullptr;
}

void WinPortFrame::SetInitialSize()
{
	if (!_win_state.fullscreen && !_win_state.maximized && !g_broadway && g_maximize <= 0) {
		// workaround for #1483 (wrong initial size on Lubuntu's LXQt DE)
		SetSize(_win_state.pos.x, _win_state.pos.y,
			_win_state.size.GetWidth(), _win_state.size.GetHeight());
		if(_win_state.charSize.GetWidth() > 0 && _win_state.charSize.GetHeight() > 0) {
			_panel->SetClientCharSize(_win_state.charSize.GetWidth(), _win_state.charSize.GetHeight());
		}
	}
}

void WinPortFrame::OnConsoleSaveWindowStateSync(wxCommandEvent& event)
{
	if (IsShown()) {
		_win_state.maximized = IsMaximized();
		_win_state.fullscreen = IsFullScreen();

		if (!_win_state.maximized && !_win_state.fullscreen) {
			_win_state.pos = GetPosition();
			_win_state.size = GetSize();

			// align saved size by font dimensions to avoid blank edges on next start
			int gap_horz = 0, gap_vert = 0;
			_panel->GetAlignmentGaps(gap_horz, gap_vert);
			_win_state.size.SetWidth(_win_state.size.GetWidth() - gap_horz);
			_win_state.size.SetHeight(_win_state.size.GetHeight() - gap_vert);

		} else {
			// if window maximized on different display - have to save its position anyway
			// however dont save frame's position in such case but save workarea left.top instead
			// cuz maximized window's pos can be outside of related display
			const int prev_disp_index = wxDisplay::GetFromPoint(_win_state.pos);
			const int disp_index = wxDisplay::GetFromWindow(this);
//			fprintf(stderr, "prev_disp_index=%d disp_index=%d\n", prev_disp_index, disp_index);
			if (prev_disp_index != disp_index && disp_index >= 0 && disp_index < (int)wxDisplay::GetCount()) {
				wxDisplay disp(disp_index);
				wxRect rc = disp.GetClientArea();
				_win_state.pos.x = rc.GetLeft();
				_win_state.pos.y = rc.GetTop();
			}
		}
		_win_state.Save();
	}
}


void WinPortFrame::OnEraseBackground(wxEraseEvent &event)
{
}

void WinPortFrame::OnPaint(wxPaintEvent &event)
{
	wxPaintDC dc(this);
	dc.Clear();
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
		for (char c = 'A'; c <= 'Z'; ++c) {
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
		//	SetAcceleratorTable(table);
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

	bool dup = wxConsoleInputShim::IsKeyDowned(ir.Event.KeyEvent.wVirtualKeyCode);
	fprintf(stderr, "OnAccelerator: ID=%u ControlKeyState=0x%x Key=0x%x '%c'%s\n",
		event.GetId(), ir.Event.KeyEvent.dwControlKeyState, ir.Event.KeyEvent.wVirtualKeyCode,
		ir.Event.KeyEvent.wVirtualKeyCode, dup ? " DUP" : "");

	if (!dup) {
		wxConsoleInputShim::Enqueue(&ir, 1);
		ir.Event.KeyEvent.bKeyDown = FALSE;
		wxConsoleInputShim::Enqueue(&ir, 1);
	}
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
	EVT_COMMAND(wxID_ANY, WX_CONSOLE_SET_CURSOR_BLINK_TIME, WinPortPanel::OnConsoleSetCursorBlinkTimeSync)
	EVT_COMMAND(wxID_ANY, WX_CONSOLE_EXIT, WinPortPanel::OnConsoleExitSync)

	EVT_IDLE(WinPortPanel::OnIdle)
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
	: _paint_context(this), _frame(frame), _refresh_rects_throttle(WINPORT(GetTickCount)())
{
	// far2l doesn't need special erase background
	SetBackgroundStyle(wxBG_STYLE_PAINT);
	Create(frame, wxID_ANY, pos, size, wxWANTS_CHARS | wxNO_BORDER);
	g_winport_con_out->SetBackend(this);
	_periodic_timer = new wxTimer(this, TIMER_ID);
	_periodic_timer->Start(g_TIMER_PERIOD);
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

void WinPortPanel::GetAlignmentGaps(int &horz, int &vert)
{
	int width = 0, height = 0;
	GetClientSize(&width, &height);
	const unsigned int font_width = _paint_context.FontWidth();
	const unsigned int font_height = _paint_context.FontHeight();
	horz = (width % font_width);
	vert = (height % font_height);
}

void WinPortPanel::SetClientCharSize(int cw, int ch)
{
	_frame->SetClientSize(cw*_paint_context.FontWidth(), ch*_paint_context.FontHeight());
}

void WinPortPanel::SetInitialSize()
{
	_frame->SetInitialSize();

	int w, h;
	GetClientSize(&w, &h);
	fprintf(stderr, "SetInitialSize: client size = %u x %u\n", w, h);
	SetConsoleSizeFromWindow();
}

void WinPortPanel::OnInitialized( wxCommandEvent& event )
{
	SetInitialSize();
	_initial_size = _frame->GetSize();
	if (g_winport_app_thread) {
#ifdef __APPLE__
		Touchbar_Register(this);
#endif
		_app_entry_started = true;
		WinPortAppThread *tmp = g_winport_app_thread;
		g_winport_app_thread = NULL;
		tmp->Start(this);
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
	return g_wx_norgb ? 4 : 24;
}

void WinPortPanel::OnTouchbarKey(bool alternate, int index)
{
	INPUT_RECORD ir = {};
	ir.EventType = KEY_EVENT;
	ir.Event.KeyEvent.wRepeatCount = 1;

	if (!alternate) {
		ir.Event.KeyEvent.wVirtualKeyCode = VK_F1 + index;

	} else switch (index) { // "", "Ins", "Del", "", "+", "-", "*", "/", "Home", "End", "PageUp", "PageDown"
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

	if (wxGetKeyState(WXK_SHIFT)) ir.Event.KeyEvent.dwControlKeyState|= SHIFT_PRESSED;
	if (wxGetKeyState(WXK_CONTROL)) ir.Event.KeyEvent.dwControlKeyState|= LEFT_CTRL_PRESSED;
	if (wxGetKeyState(WXK_ALT)) ir.Event.KeyEvent.dwControlKeyState|= LEFT_ALT_PRESSED;
	ir.Event.KeyEvent.dwControlKeyState|= WxKeyboardLedsState();

	fprintf(stderr, "%s: F%d dwControlKeyState=0x%x\n", __FUNCTION__,
		index + 1, ir.Event.KeyEvent.dwControlKeyState);

	ir.Event.KeyEvent.bKeyDown = TRUE;
	wxConsoleInputShim::Enqueue(&ir, 1);
	ir.Event.KeyEvent.bKeyDown = FALSE;
	wxConsoleInputShim::Enqueue(&ir, 1);

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
		fprintf(stderr, "Changing size: %u x %u -> %u x %u %s\n",
			prev_width, prev_height, width, height, _app_entry_started ? "with notify" : "");
#ifdef __APPLE__
		SetSize(width * font_width, height * font_height);
#endif
		g_winport_con_out->SetSize(width, height);
		if (_app_entry_started) {
			INPUT_RECORD ir = {0};
			ir.EventType = WINDOW_BUFFER_SIZE_EVENT;
			ir.Event.WindowBufferSizeEvent.dwSize.X = width;
			ir.Event.WindowBufferSizeEvent.dwSize.Y = height;
			wxConsoleInputShim::Enqueue(&ir, 1);
		}
	}
}

void WinPortPanel::CheckForResizePending()
{
#ifndef __APPLE__
	if (_app_entry_started && _resize_pending!=RP_NONE)
#endif
	{
#ifndef __APPLE__
//		fprintf(stderr, "%lu CheckForResizePending\n", (unsigned long)GetProcessUptimeMSec());
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
			_periodic_timer->Start(g_TIMER_PERIOD);
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
	if (_timer_idling_counter >= g_TIMER_IDLING_CYCLES && _paint_context.CursorBlinkState() && _text2clip.empty()) {
		_periodic_timer->Stop();
	}
}

void WinPortPanel::ResetTimerIdling()
{
	if (_timer_idling_counter >= g_TIMER_IDLING_CYCLES && !_periodic_timer->IsRunning()) {
		_periodic_timer->Start(_extra_refresh ? TIMER_EXTRA_REFRESH : g_TIMER_PERIOD);

	} else if (_extra_refresh) {
		_periodic_timer->Stop();
		_periodic_timer->Start(TIMER_EXTRA_REFRESH);
	}

	_timer_idling_counter = 0;
}

static int ProcessAllEvents()
{
	wxApp *app =wxTheApp;
	if (app) {
		while (app->Pending())
			app->Dispatch();
	}
	return 0;
}

void WinPortPanel::OnIdle( wxIdleEvent& event )
{
	if (_force_size_on_paint_state == 1) {
		_force_size_on_paint_state = 2;
		const auto &cur_size = _frame->GetSize();
		if (_initial_size != cur_size) {
			fprintf(stderr, "Re-enforce _initial_size={%d:%d} cuz cur_size={%d:%d}\n",
				_initial_size.GetWidth(), _initial_size.GetHeight(), cur_size.GetWidth(), cur_size.GetHeight());
			SetInitialSize();
		}
	}

	// first finalize any still pending repaints
	wxCommandEvent cmd_evnt;
	OnRefreshSync(cmd_evnt);
}

void WinPortPanel::OnConsoleOutputUpdated(const SMALL_RECT *areas, size_t count)
{
	enum {
		A_NOTHING,
		A_QUEUE,
		A_THROTTLE // dont emit WX_CONSOLE_REFRESH but instead do ProcessAllEvents to dispatch already queued events
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
/*
				if (!(area.Left <= pending.Right && area.Right >= pending.Left &&
						area.Top <= pending.Bottom && area.Bottom >= pending.Top ))
				{
					continue;
				}
*/

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

////////////////////////////////////////////////////////////////////////////////////////////////
// Workaround for #1303 #1454:
// When running in X session there is floating bug somewhere in WX or GTK or even XOrg that
// causes window repaint to be lost randomly in case repaint happened right after title change.
// In case of far2l title change happens together with content changes that causes this issue.
// So workaround is to detect if some refresh happened just after title changed and repeat
// do extra refresh just in case. Note that 'just after' means 'during TIMER_EXTRA_REFRESH'.
void WinPortPanel::OnTitleChangedSync( wxCommandEvent& event )
{
	if (!g_winport_frame) {
		fprintf(stderr, "%s: frame is gone\n", __FUNCTION__);
		return;
	}

	// first finalize any still pending repaints
	OnRefreshSync( event );
	Update();

	const std::wstring &title = g_winport_con_out->GetTitle();
	wxGetApp().SetAppDisplayName(title.c_str());
	_frame->SetTitle(title.c_str());
	_last_title_ticks = WINPORT(GetTickCount)();
}

static void TitleChangeCallback(PVOID ctx)
{
	WinPortPanel *it = (WinPortPanel *)ctx;
	wxCommandEvent *event = new(std::nothrow) wxCommandEvent(WX_CONSOLE_TITLE_CHANGED);
	if (!g_winport_frame) {
		fprintf(stderr, "%s: frame is gone\n", __FUNCTION__);

	} else if (event)
		wxQueueEvent(it, event);
}

// Another level of workaround for #1303 #1454:
// Problem happens if window title change happened just before repaint but
// it doesn't happen if title changed after repaint even if just after repaint.
// So instead of applying new title just when application wanted it to apply
// - wait until application will invoke some console readout function, meaning
// it entered idle state and risk of upcoming repaints is much lowered then.
// Do this by using CALLBACK_EVENT functionality that was added exactly for this.
void WinPortPanel::OnConsoleOutputTitleChanged()
{
	INPUT_RECORD ir{CALLBACK_EVENT};
	ir.Event.CallbackEvent.Function = TitleChangeCallback;
	ir.Event.CallbackEvent.Context = this;
	wxConsoleInputShim::Enqueue(&ir, 1);
}
////////////////////////////////////////////////////////////////////////////////////////////////

static bool IsForcedCharTranslation(int code)
{
	return (code==WXK_NUMPAD0 || code==WXK_NUMPAD1|| code==WXK_NUMPAD2 
		|| code==WXK_NUMPAD3 || code==WXK_NUMPAD4 || code==WXK_NUMPAD5
		|| code==WXK_NUMPAD6 || code==WXK_NUMPAD7 || code==WXK_NUMPAD8 
		|| code==WXK_NUMPAD9 || code==WXK_NUMPAD_DECIMAL || code==WXK_NUMPAD_SPACE
		|| code==WXK_NUMPAD_SEPARATOR || code==WXK_NUMPAD_EQUAL || code==WXK_NUMPAD_ADD
		|| code==WXK_NUMPAD_MULTIPLY || code==WXK_NUMPAD_SUBTRACT || code==WXK_NUMPAD_DIVIDE);
}

// helper function that returns textual description of wx virtual keycode
const char* GetWxVirtualKeyCodeName(int keycode)
{
	switch ( keycode )
	{
#define WXK_(x) \
		case WXK_##x: return #x;

		WXK_(BACK)
		WXK_(TAB)
		WXK_(RETURN)
		WXK_(ESCAPE)
		WXK_(SPACE)
		WXK_(DELETE)
		WXK_(START)
		WXK_(LBUTTON)
		WXK_(RBUTTON)
		WXK_(CANCEL)
		WXK_(MBUTTON)
//		WXK_(NUMPAD_CENTER)
		WXK_(SHIFT)
		WXK_(ALT)
		WXK_(CONTROL)
		WXK_(MENU)
		WXK_(PAUSE)
		WXK_(CAPITAL)
		WXK_(END)
		WXK_(HOME)
		WXK_(LEFT)
		WXK_(UP)
		WXK_(RIGHT)
		WXK_(DOWN)
		WXK_(SELECT)
		WXK_(PRINT)
		WXK_(EXECUTE)
		WXK_(SNAPSHOT)
		WXK_(INSERT)
		WXK_(HELP)
		WXK_(NUMPAD0)
		WXK_(NUMPAD1)
		WXK_(NUMPAD2)
		WXK_(NUMPAD3)
		WXK_(NUMPAD4)
		WXK_(NUMPAD5)
		WXK_(NUMPAD6)
		WXK_(NUMPAD7)
		WXK_(NUMPAD8)
		WXK_(NUMPAD9)
		WXK_(MULTIPLY)
		WXK_(ADD)
		WXK_(SEPARATOR)
		WXK_(SUBTRACT)
		WXK_(DECIMAL)
		WXK_(DIVIDE)
		WXK_(F1)
		WXK_(F2)
		WXK_(F3)
		WXK_(F4)
		WXK_(F5)
		WXK_(F6)
		WXK_(F7)
		WXK_(F8)
		WXK_(F9)
		WXK_(F10)
		WXK_(F11)
		WXK_(F12)
		WXK_(F13)
		WXK_(F14)
		WXK_(F15)
		WXK_(F16)
		WXK_(F17)
		WXK_(F18)
		WXK_(F19)
		WXK_(F20)
		WXK_(F21)
		WXK_(F22)
		WXK_(F23)
		WXK_(F24)
		WXK_(NUMLOCK)
		WXK_(SCROLL)
		WXK_(PAGEUP)
		WXK_(PAGEDOWN)
		WXK_(NUMPAD_SPACE)
		WXK_(NUMPAD_TAB)
		WXK_(NUMPAD_ENTER)
		WXK_(NUMPAD_F1)
		WXK_(NUMPAD_F2)
		WXK_(NUMPAD_F3)
		WXK_(NUMPAD_F4)
		WXK_(NUMPAD_HOME)
		WXK_(NUMPAD_LEFT)
		WXK_(NUMPAD_UP)
		WXK_(NUMPAD_RIGHT)
		WXK_(NUMPAD_DOWN)
		WXK_(NUMPAD_PAGEUP)
		WXK_(NUMPAD_PAGEDOWN)
		WXK_(NUMPAD_END)
		WXK_(NUMPAD_INSERT)
		WXK_(NUMPAD_DELETE)
		WXK_(NUMPAD_EQUAL)
		WXK_(NUMPAD_MULTIPLY)
		WXK_(NUMPAD_ADD)
		WXK_(NUMPAD_SEPARATOR)
		WXK_(NUMPAD_SUBTRACT)
		WXK_(NUMPAD_DECIMAL)
		WXK_(NUMPAD_DIVIDE)

		WXK_(WINDOWS_LEFT)
		WXK_(WINDOWS_RIGHT)
#ifdef __WXOSX__
		WXK_(RAW_CONTROL)
#endif
#undef WXK_

	default:
		return "ERR";
	}
}

char* FormatWxKeyState(uint16_t state) {

	static char buffer[5];

	buffer[0]  = state & wxMOD_ALT          ? 'A' : 'a';
	buffer[1]  = state & wxMOD_CONTROL      ? 'C' : 'c';
	buffer[2]  = state & wxMOD_SHIFT        ? 'S' : 's';
	buffer[3]  = state & wxMOD_META         ? 'M' : 'm';

	buffer[4] = '\0';

	return buffer;
}

void WinPortPanel::OnKeyDown( wxKeyEvent& event )
{
	ResetTimerIdling();
	DWORD now = WINPORT(GetTickCount)();
	const auto uni = event.GetUnicodeKey();
	fprintf(stderr, "\nOnKeyDown: %s %s raw=%x code=%x uni=%x (%lc) ts=%lu [now=%u]",
		FormatWxKeyState(event.GetModifiers()),
		GetWxVirtualKeyCodeName(event.GetKeyCode()),
		event.GetRawKeyCode(), event.GetKeyCode(),
		uni, (uni > 0x1f) ? uni : L' ', event.GetTimestamp(), now);

	_exclusive_hotkeys.OnKeyDown(event, _frame);

	bool keystroke_doubled = !g_wayland
		&& event.GetTimestamp()
		&& _key_tracker.LastKeydown().GetKeyCode() == event.GetKeyCode()
		&& _key_tracker.LastKeydown().GetTimestamp() == event.GetTimestamp()
		// in macos and wslg under certain stars superposition all events get same timestamps (#325)
		// however vise-verse problem also can be observed, where some keystrokes get duplicated
		// last time: catalina in hackintosh, Ctrl+O works buggy
		&& now - _key_tracker.LastKeydownTicks() < 50 // so enforce extra check actual real time interval
		;

	if (event.GetSkipped() || keystroke_doubled) {
		fprintf(stderr, " SKIP\n");
		event.Skip();
		return;
	}

#ifdef __APPLE__
	if (!event.RawControlDown() && !event.ShiftDown() && !event.MetaDown() && !event.AltDown() && event.CmdDown()
		&& (uni == 'H' || uni == 'Q' || uni == 'M') ) {
		fprintf(stderr, " Cmd+%lc\n", uni);
		ResetInputState();
		event.Skip();
		_stolen_key = uni;
		if (uni == 'M') {
			_frame->Iconize();
		} else if (uni == 'Q') {
			_frame->Close();
		} else { // 'H'
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
		|| (dwMods & (LEFT_CTRL_PRESSED | RIGHT_CTRL_PRESSED | LEFT_ALT_PRESSED)) != 0
		|| event.GetKeyCode() == WXK_DELETE || event.GetKeyCode() == WXK_RETURN
		|| (event.GetUnicodeKey()==WXK_NONE && !IsForcedCharTranslation(event.GetKeyCode()) ))
	{
		wxConsoleInputShim::Enqueue(&ir, 1);
		_last_keydown_enqueued = true;

	} else if (
		event.ControlDown() &&
		ir.Event.KeyEvent.wVirtualKeyCode &&
		((ir.Event.KeyEvent.wVirtualKeyCode < 'A') || (ir.Event.KeyEvent.wVirtualKeyCode > 'Z')) &&
		(event.GetUnicodeKey() > 127)
	) {
		// ctrl+non_latin_letter what do not have menu shortcut, like ctrl+">"
		wxConsoleInputShim::Enqueue(&ir, 1);
		_last_keydown_enqueued = true;

	}
#ifdef WX_ALT_NONLATIN
	else if (alt_nonlatin_workaround) {
		OnChar(event);
	}
#endif

	event.Skip();
}

void WinPortPanel::OnKeyUp( wxKeyEvent& event )
{
	ResetTimerIdling();
	const auto uni = event.GetUnicodeKey();
	fprintf(stderr, "\nOnKeyUp: %s %s raw=%x code=%x uni=%x (%lc) ts=%lu",
		FormatWxKeyState(event.GetModifiers()),
		GetWxVirtualKeyCodeName(event.GetKeyCode()),
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

#ifdef __WXOSX__
	// Workaround for #1580:
	// if focus switch happened due to hotkey pressed then MacOS
	// sends us keyup events for keys that were used for that hotkey
	// that discourages users and also me
	if (!was_pressed) {
		const DWORD ts = WINPORT(GetTickCount)();
		if (ts >= _focused_ts && ts - _focused_ts < 200) {
			fprintf(stderr, " SKIP_UNPAIRED (%u msec)\n", ts - _focused_ts);
			event.Skip();
			return;
		}
	}
#endif
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
			wxConsoleInputShim::Enqueue(&ir, 1);
			ir.Event.KeyEvent.bKeyDown = TRUE;
		}
#endif
		wxConsoleInputShim::Enqueue(&ir, 1);
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
	if (_key_tracker.LastKeydown().GetTimestamp() != event.GetTimestamp()) {
		fprintf(stderr, "\n");
	}
	fprintf(stderr, "OnChar: %s %s raw=%x code=%x uni=%x (%lc) ts=%lu lke=%u",
		FormatWxKeyState(event.GetModifiers()),
		GetWxVirtualKeyCodeName(event.GetKeyCode()),
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
				ir.Event.KeyEvent.wVirtualScanCode = irx.Event.KeyEvent.wVirtualScanCode;
				ir.Event.KeyEvent.dwControlKeyState = irx.Event.KeyEvent.dwControlKeyState;
			}
		}
		ir.Event.KeyEvent.uChar.UnicodeChar = event.GetUnicodeKey();

		ir.Event.KeyEvent.bKeyDown = TRUE;
		wxConsoleInputShim::Enqueue(&ir, 1);
		
		ir.Event.KeyEvent.bKeyDown = FALSE;
		wxConsoleInputShim::Enqueue(&ir, 1);
		
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
	if (_force_size_on_paint_state == 0) {
		_force_size_on_paint_state = 1;
	}
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
		|| (event.RightDown() && !_last_mouse_event.RightDown()) )
	{
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
	else if (event.RightDown()) _mouse_state|= RIGHTMOST_BUTTON_PRESSED;
	else if (event.LeftUp()) _mouse_state&= ~FROM_LEFT_1ST_BUTTON_PRESSED;
	else if (event.MiddleUp()) _mouse_state&= ~FROM_LEFT_2ND_BUTTON_PRESSED;
	else if (event.RightUp()) _mouse_state&= ~RIGHTMOST_BUTTON_PRESSED;
	else if (event.Moving() || event.Dragging()) ir.Event.MouseEvent.dwEventFlags|= MOUSE_MOVED;
	else if (event.GetWheelRotation()!=0) {
		if (event.GetWheelAxis()==wxMOUSE_WHEEL_HORIZONTAL)
			ir.Event.MouseEvent.dwEventFlags|= MOUSE_HWHEELED;
		else
			ir.Event.MouseEvent.dwEventFlags|= MOUSE_WHEELED;
		ir.Event.MouseEvent.dwButtonState|= (event.GetWheelRotation() > 0) ? 0x00010000 : 0xffff0000;

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
		wxConsoleInputShim::Enqueue(&ir, 1);
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
		wxConsoleInputShim::Enqueue(&ir, 1);
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
		(e->cookie & EXCLUSIVE_ALT_RIGHT) != 0, (e->cookie & EXCLUSIVE_WIN_LEFT) != 0,
		(e->cookie & EXCLUSIVE_WIN_RIGHT) != 0);

	_paint_context.SetSharp( (e->cookie & CONSOLE_PAINT_SHARP) != 0);
}

DWORD64 WinPortPanel::OnConsoleSetTweaks(DWORD64 tweaks)
{
	DWORD64 out = TWEAK_STATUS_SUPPORT_CHANGE_FONT | TWEAK_STATUS_SUPPORT_BLINK_RATE;

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
	return _focused_ts != 0;
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
	const std::string &str_title = Wide2MB(title);
	const std::string &str_text = Wide2MB(text);

#ifdef __APPLE__
	auto fn = std::bind(MacDisplayNotify, str_title.c_str(), str_text.c_str());
	CallInMain<bool>(fn);

#else

	static std::string s_notify_sh = GetNotifySH();
	if (s_notify_sh.empty()) {
		fprintf(stderr, "OnConsoleDisplayNotification: notify.sh not found\n");
		return;
	}

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
#endif
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

void WinPortPanel::OnConsoleSaveWindowState()
{
	wxCommandEvent *event = new(std::nothrow) wxCommandEvent(WX_CONSOLE_SAVE_WIN_STATE);
	if (event)
		wxQueueEvent(_frame, event);
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

void WinPortPanel::OnConsoleSetCursorBlinkTimeSync( wxCommandEvent& event )
{
	EventWithDWORD64 *e = (EventWithDWORD64 *)&event;
	DWORD interval = e->cookie;
	if (interval < 100 )
		g_TIMER_PERIOD = 100;
	else if (interval > 500 )
		g_TIMER_PERIOD = 500;
	else
		g_TIMER_PERIOD = interval;

	g_TIMER_IDLING_CYCLES = TIMER_IDLING_TIME / g_TIMER_PERIOD;

	_periodic_timer->Stop();
	_periodic_timer->Start(g_TIMER_PERIOD);
}

void WinPortPanel::OnConsoleSetCursorBlinkTime(DWORD interval)
{
	EventWithDWORD64 *event = new(std::nothrow) EventWithDWORD64(interval, WX_CONSOLE_SET_CURSOR_BLINK_TIME);
	if (event)
		wxQueueEvent(this, event);
}

void WinPortPanel::CheckPutText2CLip()
{
	if (!_text2clip.empty()) {
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
	//fprintf(stderr, "OnSetFocus\n");
	const DWORD ts = WINPORT(GetTickCount)();
	_focused_ts = ts ? ts : 1;
	ResetTimerIdling();
}

void WinPortPanel::OnKillFocus( wxFocusEvent &event )
{
	fprintf(stderr, "OnKillFocus\n");
	_focused_ts = 0;
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

static void ConsoleOverrideColorInMain(DWORD Index, DWORD *ColorFG, DWORD *ColorBK)
{
	if (Index == (DWORD)-1) {
		const DWORD64 orig_attrs = g_winport_con_out->GetAttributes();
		DWORD64 new_attrs = orig_attrs;
		if ((*ColorFG & 0xff000000) == 0) {
			SET_RGB_FORE(new_attrs, *ColorFG);
		}
		if ((*ColorBK & 0xff000000) == 0) {
			SET_RGB_BACK(new_attrs, *ColorBK);
		}
		if (new_attrs != orig_attrs) {
			g_winport_con_out->SetAttributes(new_attrs);
		}

		*ColorFG = WxConsoleForeground2RGB(orig_attrs & ~(DWORD64)COMMON_LVB_REVERSE_VIDEO).AsRGB();
		*ColorBK = WxConsoleBackground2RGB(orig_attrs & ~(DWORD64)COMMON_LVB_REVERSE_VIDEO).AsRGB();
		return;
	}

	WinPortRGB fg(*ColorFG), bk(*ColorBK);
	if (*ColorFG == (DWORD)-1) {
		fg = g_winport_palette.foreground[Index];
	}
	if (*ColorBK == (DWORD)-1) {
		bk = g_winport_palette.background[Index];
	}
	const auto prev_fg = g_wx_palette.foreground[Index].AsRGB();
	const auto prev_bk = g_wx_palette.background[Index].AsRGB();
	if (*ColorFG != (DWORD)-2) {
		g_wx_palette.foreground[Index] = fg;
	}
	if (*ColorBK != (DWORD)-2) {
		g_wx_palette.background[Index] = bk;
	}
	*ColorFG = prev_fg;
	*ColorBK = prev_bk;
}

static void ConsoleOverrideBasePaletteInMain(void *pbuff)
{
	memcpy(&g_wx_palette, pbuff, BASE_PALETTE_SIZE * sizeof(WinPortRGB) * 2);
}

void WinPortPanel::OnConsoleGetBasePalette(void *pbuff)
{
	memcpy(pbuff, &g_wx_palette, BASE_PALETTE_SIZE * sizeof(WinPortRGB) * 2);
}

bool WinPortPanel::OnConsoleSetBasePalette(void *pbuff)
{
	if (!pbuff)
		return false;

	auto fn = std::bind(&ConsoleOverrideBasePaletteInMain, pbuff);
	CallInMainNoRet(fn);

	return true;
}

void WinPortPanel::OnConsoleOverrideColor(DWORD Index, DWORD *ColorFG, DWORD *ColorBK)
{
	if (Index >= BASE_PALETTE_SIZE) {
		fprintf(stderr, "%s: too big index=%u\n", __FUNCTION__, Index);
		return;
	}

	auto fn = std::bind(&ConsoleOverrideColorInMain, Index, ColorFG, ColorBK);
	CallInMainNoRet(fn);
}
