#include "stdafx.h"
#include "ConsoleOutput.h"
#include "ConsoleInput.h"
#include "wxWinTranslations.h"
#include <wx/clipbrd.h>
#include "CallInMain.h"
#include "PathHelpers.h"
#include "Paint.h"
#include "utils.h"
#include <set>
#include <fstream>

ConsoleOutput g_wx_con_out;
ConsoleInput g_wx_con_in;
enum
{
    TIMER_ID_PERIODIC = 10
};

extern "C" void WinPortInitRegistry();
extern "C" bool WinPortIsClipboardBusy();


static void NormalizeArea(SMALL_RECT &area)
{
	if (area.Left > area.Right) std::swap(area.Left, area.Right);
	if (area.Top > area.Bottom) std::swap(area.Top, area.Bottom);	
}

class WinPortAppThread : public wxThread
{
public:
	WinPortAppThread(int argc, char **argv, int(*appmain)(int argc, char **argv))
		: wxThread(wxTHREAD_DETACHED), _argv(argv), _argc(argc), _appmain(appmain)  {  }

protected:
	virtual ExitCode Entry()
	{
		_r = _appmain(_argc, _argv);
		exit(_r);
		return 0;
	}

private:
	WinPortAppThread() = delete;
	WinPortAppThread(const WinPortAppThread&) = delete;
	
	char **_argv;
	int _argc;
	int(*_appmain)(int argc, char **argv);
	int _r;
} *g_winport_app_thread = NULL;


extern "C" int WinPortMain(int argc, char **argv, int(*AppMain)(int argc, char **argv))
{
	wxInitialize();
	WinPortInitRegistry();
	WinPortInitWellKnownEnv();
	g_wx_con_out.WriteString(L"Hello", 5);
	if (AppMain && !g_winport_app_thread) {
		g_winport_app_thread = new WinPortAppThread(argc, argv, AppMain);
		if (!g_winport_app_thread)
			return -1;		
	}

	wxEntry(argc, argv);
	wxUninitialize();
	return 0;
}

///////////////


static void SaveSize(unsigned int width, unsigned int height)
{
	std::ofstream os;
	os.open(InMyProfile("consolesize").c_str());
	if (os.is_open()) {
		os << width << std::endl;
		os << height << std::endl;
	}
}

static void LoadSize(unsigned int &width, unsigned int &height)
{
	std::ifstream is;
	is.open(InMyProfile("consolesize").c_str());
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

struct EventWithRect : wxCommandEvent
{
	EventWithRect(const SMALL_RECT &rect_, wxEventType commandType = wxEVT_NULL, int winid = 0) 
		:wxCommandEvent(commandType, winid) , rect(rect_)
	{
		NormalizeArea(rect);
	}

	virtual wxEvent *Clone() const { return new EventWithRect(*this); }

	SMALL_RECT rect;
};

///////////////////////////////////////////

wxDEFINE_EVENT(WX_CONSOLE_INITIALIZED, wxCommandEvent);
wxDEFINE_EVENT(WX_CONSOLE_REFRESH, EventWithRect);
wxDEFINE_EVENT(WX_CONSOLE_WINDOW_MOVED, EventWithRect);
wxDEFINE_EVENT(WX_CONSOLE_RESIZED, wxCommandEvent);
wxDEFINE_EVENT(WX_CONSOLE_TITLE_CHANGED, wxCommandEvent);
wxDEFINE_EVENT(WX_CONSOLE_ADHOC_QEDIT, wxCommandEvent);

//////////////////////////////////////////

class WinPortApp: public wxApp
{
public:
    virtual bool OnInit();
};

class WinPortFrame;

class WinPortPanel: public wxPanel, protected ConsoleOutputListener
{
public:
    WinPortPanel(WinPortFrame *frame, const wxPoint& pos, const wxSize& size);
	virtual ~WinPortPanel();
	void CompleteInitialization();
	void OnChar( wxKeyEvent& event );

protected: 
	virtual void OnConsoleOutputUpdated(const SMALL_RECT &area);
	virtual void OnConsoleOutputResized();
	virtual void OnConsoleOutputTitleChanged();
	virtual void OnConsoleOutputWindowMoved(bool absolute, COORD pos);
	virtual COORD OnConsoleGetLargestWindowSize();
	virtual void OnConsoleAdhocQuickEdit();
	
private:
	void CheckForResizePending();
	void CheckPutText2CLip();
	void OnInitialized( wxCommandEvent& event );
	void OnTimerPeriodic(wxTimerEvent& event);	
	void OnWindowMovedSync( wxCommandEvent& event );
	void OnRefreshSync( wxCommandEvent& event );
	void OnConsoleResizedSync( wxCommandEvent& event );
	void OnTitleChangedSync( wxCommandEvent& event );
	void OnConsoleAdhocQuickEditSync( wxCommandEvent& event );
	void OnKeyDown( wxKeyEvent& event );
	void OnKeyUp( wxKeyEvent& event );
	void OnPaint( wxPaintEvent& event );
    void OnEraseBackground( wxEraseEvent& event );
	void OnSize(wxSizeEvent &event);
	void OnMouse( wxMouseEvent &event );
	void OnMouseNormal( wxMouseEvent &event, COORD pos_char);
	void OnMouseQEdit( wxMouseEvent &event, COORD pos_char);
	void OnKillFocus( wxFocusEvent &event );
	COORD TranslateMousePosition( wxMouseEvent &event );
	void DamageAreaBetween(COORD c1, COORD c2);
	int GetDisplayIndex();

	wxDECLARE_EVENT_TABLE();
	struct PressedKeys : std::set<int> 
	{
		bool simulate_alt() const
		{
			return find(WXK_ALT)!=end();
		}
	} _pressed_keys;
	
	ConsolePaintContext _paint_context;
	wxKeyEvent _last_keydown;
	wxMouseEvent _last_mouse_event;
	std::wstring _text2clip;
	
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
	DWORD _mouse_state, _mouse_qedit_pending;
	COORD _mouse_qedit_start, _mouse_qedit_last;
	
	int _last_valid_display;
};

///////////////////////////////////////////

class WinPortFrame: public wxFrame
{
public:
    WinPortFrame(const wxString& title, const wxPoint& pos, const wxSize& size);
	virtual ~WinPortFrame();
	
	void OnShow(wxShowEvent &show);
protected: 
private:
	WinPortPanel		*_panel;
	bool _shown;

	void OnChar( wxKeyEvent& event )
	{
		_panel->OnChar(event);
	}
	void OnPaint( wxPaintEvent& event ) {}
	void OnEraseBackground( wxEraseEvent& event ) {}
    wxDECLARE_EVENT_TABLE();
};


wxBEGIN_EVENT_TABLE(WinPortFrame, wxFrame)
	EVT_PAINT(WinPortFrame::OnPaint)
	EVT_SHOW(WinPortFrame::OnShow)
	EVT_ERASE_BACKGROUND(WinPortFrame::OnEraseBackground)
	EVT_CHAR(WinPortFrame::OnChar)
wxEND_EVENT_TABLE()

void WinPortFrame::OnShow(wxShowEvent &show)
{
	if (!_shown) {
		_shown = true;
		wxCommandEvent *event = new wxCommandEvent(WX_CONSOLE_INITIALIZED);
		if (event)
			wxQueueEvent(_panel, event);		
	}
}	

WinPortFrame::WinPortFrame(const wxString& title, const wxPoint& pos, const wxSize& size)
        : wxFrame(NULL, wxID_ANY, title, pos, size), _shown(false)
{
	_panel = new WinPortPanel(this, wxPoint(0, 0), GetClientSize());
	_panel->SetFocus();
	SetBackgroundColour(*wxBLACK);
}

WinPortFrame::~WinPortFrame()
{
	delete _panel;
	_panel = NULL;
}


////////////////////////////



wxBEGIN_EVENT_TABLE(WinPortPanel, wxPanel)
	EVT_COMMAND(wxID_ANY, WX_CONSOLE_INITIALIZED, WinPortPanel::OnInitialized)
	EVT_TIMER(TIMER_ID_PERIODIC, WinPortPanel::OnTimerPeriodic)
	EVT_COMMAND(wxID_ANY, WX_CONSOLE_REFRESH, WinPortPanel::OnRefreshSync)
	EVT_COMMAND(wxID_ANY, WX_CONSOLE_WINDOW_MOVED, WinPortPanel::OnWindowMovedSync)
	EVT_COMMAND(wxID_ANY, WX_CONSOLE_RESIZED, WinPortPanel::OnConsoleResizedSync)
	EVT_COMMAND(wxID_ANY, WX_CONSOLE_TITLE_CHANGED, WinPortPanel::OnTitleChangedSync)
	EVT_COMMAND(wxID_ANY, WX_CONSOLE_ADHOC_QEDIT, WinPortPanel::OnConsoleAdhocQuickEditSync)
	
	EVT_KEY_DOWN(WinPortPanel::OnKeyDown)
	EVT_KEY_UP(WinPortPanel::OnKeyUp)
	EVT_CHAR(WinPortPanel::OnChar)
	EVT_PAINT(WinPortPanel::OnPaint)
	EVT_ERASE_BACKGROUND(WinPortPanel::OnEraseBackground)
	EVT_SIZE(WinPortPanel::OnSize)
	EVT_MOUSE_EVENTS(WinPortPanel::OnMouse )
	EVT_KILL_FOCUS(WinPortPanel::OnKillFocus )

wxEND_EVENT_TABLE()

wxIMPLEMENT_APP_NO_MAIN(WinPortApp);




bool WinPortApp::OnInit()
{
	WinPortFrame *frame = new WinPortFrame("WinPortApp", wxDefaultPosition, wxDefaultSize );
    //WinPortFrame *frame = new WinPortFrame( "WinPortApp", wxPoint(50, 50), wxSize(800, 600) );
	frame->Show( true );
    return true;
}



///////////////////////////


WinPortPanel::WinPortPanel(WinPortFrame *frame, const wxPoint& pos, const wxSize& size)
        : wxPanel(frame, wxID_ANY, pos, size, wxWANTS_CHARS | wxNO_BORDER), 
		_paint_context(this), _frame(frame), _periodic_timer(NULL),  
		_last_keydown_enqueued(false), _initialized(false), _adhoc_quickedit(false),
		_resize_pending(RP_NONE),  _mouse_state(0), _mouse_qedit_pending(false), _last_valid_display(0)
{
	g_wx_con_out.SetListener(this);
	_periodic_timer = new wxTimer(this, TIMER_ID_PERIODIC);
	_periodic_timer->Start(500);
	OnConsoleOutputTitleChanged();
}

WinPortPanel::~WinPortPanel()
{
	delete _periodic_timer;
	g_wx_con_out.SetListener(NULL);
}

void WinPortPanel::OnInitialized( wxCommandEvent& event )
{
	int w, h;
	GetClientSize(&w, &h);
	fprintf(stderr, "OnInitialized: client size = %u x %u\n", w, h);
	unsigned int cw, ch;
	g_wx_con_out.GetSize(cw, ch);
	LoadSize(cw, ch);
	wxDisplay disp(GetDisplayIndex());
	wxRect rc = disp.GetClientArea();
	if ((unsigned)rc.GetWidth() >= cw * _paint_context.FontWidth() 
		&& (unsigned)rc.GetHeight() >= ch * _paint_context.FontHeight()) {
		g_wx_con_out.SetSize(cw, ch);		
	}
	g_wx_con_out.GetSize(cw, ch);
	
	cw*= _paint_context.FontWidth();
	ch*= _paint_context.FontHeight();
	if ( w != (int)cw || h != (int)ch)
		_frame->SetClientSize(cw, ch);
		
	_initialized = true;

	if (g_winport_app_thread) {
		WinPortAppThread *tmp = g_winport_app_thread;
		g_winport_app_thread = NULL;
		if (tmp->Run() != wxTHREAD_NO_ERROR)
			delete tmp;
	}
}


void WinPortPanel::CheckForResizePending()
{
#ifndef __APPLE__
	if (_initialized && _resize_pending!=RP_NONE)
#endif
	{
		fprintf(stderr, "CheckForResizePending\n");
		DWORD conmode = 0;
		if (WINPORT(GetConsoleMode)(NULL, &conmode) && (conmode&ENABLE_WINDOW_INPUT)!=0) {
			unsigned int prev_width = 0, prev_height = 0;
			_resize_pending = RP_NONE;

			g_wx_con_out.GetSize(prev_width, prev_height);
	
			int width = 0, height = 0;
			_frame->GetClientSize(&width, &height);
			fprintf(stderr, "Current client size: %u %u font %u %u\n", 
				width, height, _paint_context.FontWidth(), _paint_context.FontHeight());
			width/= _paint_context.FontWidth(); 
			height/= _paint_context.FontHeight();
			if (width!=(int)prev_width || height!=(int)prev_height) {
				fprintf(stderr, "Changing size: %u x %u\n", width, height);
#ifdef __APPLE__
				this->SetSize(width * _paint_context.FontWidth(), height * _paint_context.FontHeight());
#endif
				g_wx_con_out.SetSize(width, height);
				if (!_frame->IsFullScreen() && !_frame->IsMaximized() && _frame->IsShown()) {
					SaveSize(width, height);
				}
				INPUT_RECORD ir = {0};
				ir.EventType = WINDOW_BUFFER_SIZE_EVENT;
				ir.Event.WindowBufferSizeEvent.dwSize.X = width;
				ir.Event.WindowBufferSizeEvent.dwSize.Y = height;
				g_wx_con_in.Enqueue(&ir, 1);
			}
	
			Refresh(false);
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
	_paint_context.ToggleCursor();
}

void WinPortPanel::OnConsoleOutputUpdated(const SMALL_RECT &area)
{
	wxCommandEvent *event = new EventWithRect(area, WX_CONSOLE_REFRESH);
	if (event)
		wxQueueEvent	(this, event);
}

void WinPortPanel::OnConsoleOutputResized()
{
	wxCommandEvent *event = new wxCommandEvent(WX_CONSOLE_RESIZED);
	if (event)
		wxQueueEvent	(this, event);
}

void WinPortPanel::OnConsoleOutputTitleChanged()
{
	wxCommandEvent *event = new wxCommandEvent(WX_CONSOLE_TITLE_CHANGED);
	if (event)
		wxQueueEvent	(this, event);
}

void WinPortPanel::OnConsoleOutputWindowMoved(bool absolute, COORD pos)
{
	SMALL_RECT rect = {pos.X, pos.Y, absolute ? (SHORT)1 : (SHORT)0, 0};
	wxCommandEvent *event = new EventWithRect(rect, WX_CONSOLE_WINDOW_MOVED);
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

void WinPortPanel::OnWindowMovedSync( wxCommandEvent& event )
{
	EventWithRect *e = (EventWithRect *)&event;
	int x = e->rect.Left, y = e->rect.Top;
	x*= _paint_context.FontWidth(); y*= _paint_context.FontHeight();
	if (!e->rect.Right) {
		int dx = 0, dy = 0;
		GetPosition(&dx, &dy);
		x+= dx; y+= dy;
	}
	Move(x, y);
}

void WinPortPanel::OnRefreshSync( wxCommandEvent& event )
{
	EventWithRect *e = (EventWithRect *)&event;
	_paint_context.RefreshArea( e->rect );
}

void WinPortPanel::OnConsoleResizedSync( wxCommandEvent& event )
{
	unsigned int width = 0, height = 0;
	g_wx_con_out.GetSize(width, height);
	int prev_width = 0, prev_height = 0;
	_frame->GetClientSize(&prev_width, &prev_height);
	fprintf(stderr, "OnConsoleResized client size: %u %u\n", prev_width, prev_height);

	prev_width/= _paint_context.FontWidth();
	prev_height/= _paint_context.FontHeight();
	
	if ((int)width != prev_width || (int)height != prev_height) {
		if ((int)width > prev_width || (int)height > prev_height) {
			_frame->SetPosition(wxPoint(0,0 ));
		}
		
		width*= _paint_context.FontWidth();
		height*= _paint_context.FontHeight();
		fprintf(stderr, "OnConsoleResized SET client size: %u %u\n", width, height);
		_frame->SetClientSize(width, height);		
	}
	Refresh(false);
}

void WinPortPanel::OnTitleChangedSync( wxCommandEvent& event )
{
	const std::wstring &title = g_wx_con_out.GetTitle();
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

void WinPortPanel::OnKeyDown( wxKeyEvent& event )
{
	fprintf(stderr, "OnKeyDown: %x %x %u %lu\n", 
		event.GetUnicodeKey(), event.GetKeyCode(), event.GetSkipped(), event.GetTimestamp());
		
	if (event.GetSkipped() || (event.GetTimestamp() && 
		_last_keydown.GetKeyCode()==event.GetKeyCode() &&
		_last_keydown.GetTimestamp()==event.GetTimestamp())) {
		event.Skip();
		return;
	}
	_last_keydown = event;
	_last_keydown_enqueued = false;
	
	if (event.GetKeyCode()==WXK_RETURN 
		&& (event.AltDown() || _pressed_keys.simulate_alt()) 
		&& !event.ShiftDown() && !event.ControlDown() ) {
		_resize_pending = RP_INSTANT;
		//fprintf(stderr, "RP_INSTANT\n");
		_frame->ShowFullScreen(!_frame->IsFullScreen());
		if (_resize_pending != RP_INSTANT)
			_resize_pending = RP_DEFER;
		_last_keydown_enqueued = true;
		return;
	}
	
	if (event.HasModifiers() || _pressed_keys.simulate_alt() || event.GetKeyCode()==WXK_DELETE ||
		(event.GetUnicodeKey()==WXK_NONE && !IsForcedCharTranslation(event.GetKeyCode()) )) {
		wx2INPUT_RECORD ir(event, TRUE);
		_pressed_keys.insert(event.GetKeyCode());
		if (_pressed_keys.simulate_alt())
			ir.Event.KeyEvent.dwControlKeyState|= LEFT_ALT_PRESSED;
		g_wx_con_in.Enqueue(&ir, 1);
		_last_keydown_enqueued = true;
	} 
	event.Skip();
}

void WinPortPanel::OnKeyUp( wxKeyEvent& event )
{
	fprintf(stderr, "OnKeyUp: %x %x %d %lu\n", 
	event.GetUnicodeKey(), event.GetKeyCode(), event.GetSkipped(), event.GetTimestamp());
	if (event.GetSkipped())
		return;
		
	if (_pressed_keys.erase(event.GetKeyCode())) {
		wx2INPUT_RECORD ir(event, FALSE);
		if (_pressed_keys.simulate_alt())
			ir.Event.KeyEvent.dwControlKeyState|= LEFT_ALT_PRESSED;	
	
		g_wx_con_in.Enqueue(&ir, 1);
	}
	//event.Skip();
}

void WinPortPanel::OnChar( wxKeyEvent& event )
{
	fprintf(stderr, "OnChar: %x %x %d %lu _lk_ts=%lu _lk_enqueued=%u\n", 
		event.GetUnicodeKey(), event.GetKeyCode(), event.GetSkipped(), event.GetTimestamp(),
		_last_keydown.GetTimestamp(), _last_keydown_enqueued);
	if (event.GetSkipped())
		return;
		
	if (event.GetUnicodeKey()!=WXK_NONE && 
		(!_last_keydown_enqueued || _last_keydown.GetTimestamp()!=event.GetTimestamp())) {
		INPUT_RECORD ir = {0};
		ir.EventType = KEY_EVENT;
		ir.Event.KeyEvent.wRepeatCount = 1;
		ir.Event.KeyEvent.wVirtualKeyCode = VK_OEM_PERIOD;
		if (event.GetUnicodeKey() <= 0x7f) { 
			if (_last_keydown.GetTimestamp()==event.GetTimestamp()) {
				wx2INPUT_RECORD irx(_last_keydown, TRUE);
				ir.Event.KeyEvent.wVirtualKeyCode = irx.Event.KeyEvent.wVirtualKeyCode;
				ir.Event.KeyEvent.dwControlKeyState = irx.Event.KeyEvent.dwControlKeyState;
			}
		}
		ir.Event.KeyEvent.uChar.UnicodeChar = event.GetUnicodeKey();

		ir.Event.KeyEvent.bKeyDown = TRUE;
		g_wx_con_in.Enqueue(&ir, 1);
		
		ir.Event.KeyEvent.bKeyDown = FALSE;
		g_wx_con_in.Enqueue(&ir, 1);
		
	}
	//event.Skip();
}


void WinPortPanel::OnPaint( wxPaintEvent& event )
{
	if (_mouse_qedit_pending) {
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
	COORD out;
	out.X =(SHORT)(USHORT)(pos.x / _paint_context.FontWidth());
	out.Y =(SHORT)(USHORT)(pos.y / _paint_context.FontHeight());

	unsigned int width = 80, height = 25;
	g_wx_con_out.GetSize(width, height);
	
	if ( out.X < 0 ) out.X = 0;
	if ( (USHORT)out.X >= width) out.X = width - 1;
	if ( out.Y < 0 ) out.Y = 0;
	if ( (USHORT)out.Y >= height) out.Y = height - 1;
	return out;
}

void WinPortPanel::OnMouse( wxMouseEvent &event )
{
	COORD pos_char = TranslateMousePosition( event );
	
	DWORD mode = 0;
	if (!WINPORT(GetConsoleMode)(NULL, &mode))
		mode = 0;

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
	if (wxGetKeyState(WXK_SHIFT)) ir.Event.MouseEvent.dwControlKeyState|= SHIFT_PRESSED;
	if (wxGetKeyState(WXK_CONTROL)) ir.Event.MouseEvent.dwControlKeyState|= LEFT_CTRL_PRESSED;
	if (wxGetKeyState(WXK_ALT)) ir.Event.MouseEvent.dwControlKeyState|= LEFT_ALT_PRESSED;
	if (wxGetKeyState(WXK_SHIFT)) ir.Event.MouseEvent.dwControlKeyState|= SHIFT_PRESSED;
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
	g_wx_con_in.Enqueue(&ir, 1);	
}

void WinPortPanel::DamageAreaBetween(COORD c1, COORD c2)
{
	SMALL_RECT area = {c1.X, c1.Y, c2.X, c2.Y};
	OnConsoleOutputUpdated(area);
}

void WinPortPanel::OnMouseQEdit( wxMouseEvent &event, COORD pos_char )
{
	if (event.LeftDown()) {
		if (_mouse_qedit_pending)
			DamageAreaBetween(_mouse_qedit_start, _mouse_qedit_last);
		_mouse_qedit_start = _mouse_qedit_last = pos_char;
		_mouse_qedit_pending = true;
		DamageAreaBetween(_mouse_qedit_start, _mouse_qedit_last);
		
	} else if (_mouse_qedit_pending) {
		if (event.Moving() || event.Dragging()) {
			DamageAreaBetween(_mouse_qedit_start, _mouse_qedit_last);
			DamageAreaBetween(_mouse_qedit_start, pos_char);			
			_mouse_qedit_last = pos_char;
		} else if (event.LeftUp()) {
			_mouse_qedit_pending = _adhoc_quickedit = false;
			DamageAreaBetween(_mouse_qedit_start, _mouse_qedit_last);
			DamageAreaBetween(_mouse_qedit_start, pos_char);			
			_text2clip.clear();
			USHORT y1 = _mouse_qedit_start.Y, y2 = pos_char.Y;
			USHORT x1 = _mouse_qedit_start.X, x2 = pos_char.X;
			if (y1 > y2) std::swap(y1, y2);
			if (x1 > x2) std::swap(x1, x2);

			COORD pos;
			for (pos.Y = y1; pos.Y<=y2; ++pos.Y) {
				if (!_text2clip.empty())
					_text2clip+= NATIVE_EOLW;
				for (pos.X = x1; pos.X<=x2; ++pos.X) {
					CHAR_INFO ch;
					if (g_wx_con_out.Read(ch, pos))
						_text2clip+= ch.Char.UnicodeChar ? ch.Char.UnicodeChar : L' ';
				}
			}
			CheckPutText2CLip();
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
		g_wx_con_in.Enqueue(&ir, 1);
		_last_mouse_event.SetEventType(wxEVT_LEFT_DOWN);
		_last_mouse_event.SetLeftDown(true);
		fprintf(stderr, "OnConsoleAdhocQuickEditSync: lbutton pressed, %u\n", _last_mouse_event.LeftIsDown());

		OnMouseQEdit( _last_mouse_event, pos_char );
	} else
		fprintf(stderr, "OnConsoleAdhocQuickEditSync: lbutton not pressed\n");
}

void WinPortPanel::OnConsoleAdhocQuickEdit()
{
	wxCommandEvent *event = new wxCommandEvent(WX_CONSOLE_ADHOC_QEDIT);
	if (event)
		wxQueueEvent(this, event);
}

void WinPortPanel::CheckPutText2CLip()
{
	if (!_text2clip.empty())  {
		if (!WinPortIsClipboardBusy()) {
			if (wxTheClipboard->Open()) {
				std::wstring text2clip; text2clip.swap(_text2clip);
				wxTheClipboard->SetData( new wxTextDataObject(text2clip) );
				wxTheClipboard->Close();
			}
		} else 
			fprintf(stderr, "CheckPutText2CLip: clipboard busy\n");
	}	
}


void WinPortPanel::OnKillFocus( wxFocusEvent &event )
{
	fprintf(stderr, "OnKillFocus\n");

	for (auto k : _pressed_keys) {
		INPUT_RECORD ir = {0};
		ir.EventType = KEY_EVENT;
		ir.Event.KeyEvent.wRepeatCount = 1;
		ir.Event.KeyEvent.wVirtualKeyCode = k;
		g_wx_con_in.Enqueue(&ir, 1);		
	}
	_pressed_keys.clear();
	
	if (_mouse_qedit_pending) {
		_mouse_qedit_pending = false;
		DamageAreaBetween(_mouse_qedit_start, _mouse_qedit_last);
	}
}

