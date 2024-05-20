#pragma once
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>

#include "Backend.h"
#include "wxWinTranslations.h"
#include "wxConsoleInputShim.h"
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
#include <mutex>

#ifdef __APPLE__
# include "Mac/dockicon.h"
# include "Mac/touchbar.h"
# include "Mac/hide.h"
# include "Mac/displaynotify.h"
#endif

class WinPortAppThread : public wxThread
{
	std::mutex _start;
	IConsoleOutputBackend *_backend;
	char **_argv;
	int _argc;
	int(*_appmain)(int argc, char **argv);

	WinPortAppThread() = delete;
	WinPortAppThread(const WinPortAppThread&) = delete;

	virtual ExitCode Entry();

public:
	WinPortAppThread(int argc, char **argv, int(*appmain)(int argc, char **argv));
	bool Prepare();

	void Start(IConsoleOutputBackend *backend);
};

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

	wxDECLARE_EVENT_TABLE();
	KeyTracker _key_tracker;
	
	ConsolePaintContext _paint_context;
	WinPortFrame *_frame;
	DWORD _refresh_rects_throttle;

	COORD _last_mouse_click{};
	wxMouseEvent _last_mouse_event;
	std::wstring _text2clip;
	ExclusiveHotkeys _exclusive_hotkeys;
	std::atomic<DWORD> _focused_ts{1};
	MOUSE_EVENT_RECORD _prev_mouse_event{};
	DWORD _prev_mouse_event_ts{0};

	wxTimer* _periodic_timer{nullptr};
	unsigned int _timer_idling_counter{0};
	std::atomic<unsigned int> _last_title_ticks{0};
	wxSize _initial_size{};
	unsigned char _force_size_on_paint_state{0};
	bool _extra_refresh{false};
	bool _last_keydown_enqueued{false};
	bool _app_entry_started{false};
	bool _adhoc_quickedit{false};
	enum
	{
		RP_NONE,
		RP_DEFER,
		RP_INSTANT
	} _resize_pending{RP_NONE};
	DWORD _mouse_state{0}, _mouse_qedit_start_ticks{0}, _mouse_qedit_moved{0};
	COORD _mouse_qedit_start{}, _mouse_qedit_last{};
	wchar_t _stolen_key{0};
	
	unsigned int _pending_refreshes{0};
	struct RefreshRects : std::vector<SMALL_RECT>, std::mutex {} _refresh_rects;

	void SetConsoleSizeFromWindow();
	void CheckForResizePending();
	void CheckPutText2CLip();
	void SetInitialSize();
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
	void OnConsoleSaveWindowStateSync(wxCommandEvent& event);
	void OnConsoleExitSync( wxCommandEvent& event );
	void OnConsoleSetCursorBlinkTimeSync(wxCommandEvent& event);
	void OnIdle( wxIdleEvent& event );
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
	void ResetInputState();
	COORD TranslateMousePosition( wxMouseEvent &event );
	void DamageAreaBetween(COORD c1, COORD c2);
	void ResetTimerIdling();

	virtual void OnConsoleOutputUpdated(const SMALL_RECT *areas, size_t count);
	virtual void OnConsoleOutputResized();
	virtual void OnConsoleOutputTitleChanged();
	virtual void OnConsoleOutputWindowMoved(bool absolute, COORD pos);
	virtual COORD OnConsoleGetLargestWindowSize();
	virtual void OnConsoleSetMaximized(bool maximized);
	virtual void OnConsoleAdhocQuickEdit();
	virtual DWORD64 OnConsoleSetTweaks(DWORD64 tweaks);
	virtual void OnConsoleChangeFont();
	virtual void OnConsoleSaveWindowState();
	virtual void OnConsoleExit();
	virtual bool OnConsoleIsActive();
	virtual void OnConsoleDisplayNotification(const wchar_t *title, const wchar_t *text);
	virtual bool OnConsoleBackgroundMode(bool TryEnterBackgroundMode);
	virtual bool OnConsoleSetFKeyTitles(const char **titles);
	virtual BYTE OnConsoleGetColorPalette();
	virtual void OnConsoleGetBasePalette(void *pbuff);
	virtual bool OnConsoleSetBasePalette(void *pbuff);
	virtual void OnConsoleOverrideColor(DWORD Index, DWORD *ColorFG, DWORD *ColorBK);
	virtual void OnConsoleSetCursorBlinkTime(DWORD interval);

public:
	WinPortPanel(WinPortFrame *frame, const wxPoint& pos, const wxSize& size);
	virtual ~WinPortPanel();
	void GetAlignmentGaps(int &horz, int &vert);
	void OnChar( wxKeyEvent& event );
	virtual void OnTouchbarKey(bool alternate, int index);
	void SetClientCharSize(int cw, int ch);
};

///////////////////////////////////////////

struct WinState
{
	wxPoint pos = wxDefaultPosition;
	wxSize size {800, 440};
	wxSize charSize {-1,-1};
	bool maximized{false};
	bool fullscreen{false};

	WinState();
	void Save();
};

class WinPortFrame: public wxFrame
{
	wxDECLARE_EVENT_TABLE();

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
	WinState _win_state;

	void OnEraseBackground(wxEraseEvent &event);
	void OnPaint(wxPaintEvent &event);
	void OnChar(wxKeyEvent &event);
	void OnAccelerator(wxCommandEvent &event);
	void OnShow(wxShowEvent &show);
	void OnClose(wxCloseEvent &show);
	void OnConsoleSaveWindowStateSync(wxCommandEvent& event);

public:
	WinPortFrame(const wxString& title);
	virtual ~WinPortFrame();

	void SetInitialSize();
	void SaveWindowState();
};
