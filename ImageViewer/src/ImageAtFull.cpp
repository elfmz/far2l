#include "Common.h"
#include "ImageView.h"
#include "Settings.h"

class ImageViewAtFull : public ImageView
{
	WinportGraphicsInfo _drag_wgi{};
	HANDLE _dlg{NULL};
	COORD _drag_prev_pos{}, _drag_pending{};
	bool _dragging{false};

protected:
	virtual void DenoteInfoAndPan(const std::string &info, const std::string &pan)
	{
		const int visible_box_dlgid = CurFileSelected() ? 1 : 0;
		const int invisible_box_dlgid = CurFileSelected() ? 0 : 1;
		const int hint_text_dlgid = 3;
		const int pan_text_dlgid = 4;
		const int info_text_dlgid = 5;

		ConsoleRepaintsDeferScope crds(NULL);
		std::wstring ws_title = CurFileSelected() ? L"* " : L"  ";
		StrMB2Wide(CurFile(), ws_title, true);
		FarDialogItemData dd_title = { ws_title.size(), (wchar_t*)ws_title.c_str() };

		// update pan and info lengthes before title, so title will paint over previous one
		// but texts  - after title, so text it will get drawn after title, and due to that - will remain visible
		const auto &ws_pan = StrMB2Wide(pan);
		FarDialogItem di{};
		if (g_far.SendDlgMessage(_dlg, DM_GETDLGITEMSHORT, pan_text_dlgid, (LONG_PTR)&di)) {
			di.X2 = di.X1 + (ws_pan.empty() ? 0 : ws_pan.size() - 1);
			g_far.SendDlgMessage(_dlg, DM_SETDLGITEMSHORT, pan_text_dlgid, (LONG_PTR)&di);
		}
		const auto &ws_info = StrMB2Wide(info);
		if (g_far.SendDlgMessage(_dlg, DM_GETDLGITEMSHORT, info_text_dlgid, (LONG_PTR)&di)) {
			di.X1 = di.X2 - (ws_info.empty() ? 0 : ws_info.size() - 1);
			g_far.SendDlgMessage(_dlg, DM_SETDLGITEMSHORT, info_text_dlgid, (LONG_PTR)&di);
		}

		g_far.SendDlgMessage(_dlg, DM_SHOWITEM, invisible_box_dlgid, 0);
		g_far.SendDlgMessage(_dlg, DM_SHOWITEM, visible_box_dlgid, 1);

		g_far.SendDlgMessage(_dlg, DM_SETTEXT, 0, (LONG_PTR)&dd_title);
		g_far.SendDlgMessage(_dlg, DM_SETTEXT, 1, (LONG_PTR)&dd_title);

		if (g_far.SendDlgMessage(_dlg, DM_GETDLGITEMSHORT, visible_box_dlgid, (LONG_PTR)&di)) {
			int X1 = di.X1, X2 = di.X2;
			const int hint_length = g_far.SendDlgMessage(_dlg, DM_GETTEXTPTR, hint_text_dlgid, 0);
			if (g_far.SendDlgMessage(_dlg, DM_GETDLGITEMSHORT, hint_text_dlgid, (LONG_PTR)&di)) {
				di.X1 = std::max(X1, int(X1 + X2 + 1 - hint_length) / 2);
				di.X2 = std::min(X2, int(di.X1 + hint_length - 1));
				g_far.SendDlgMessage(_dlg, DM_SETDLGITEMSHORT, hint_text_dlgid, (LONG_PTR)&di);
			}
		}

		FarDialogItemData dd_pan = { ws_pan.size(), (wchar_t*)ws_pan.c_str() };
		g_far.SendDlgMessage(_dlg, DM_SETTEXT, pan_text_dlgid, (LONG_PTR)&dd_pan);

		FarDialogItemData dd_info = { ws_info.size(), (wchar_t*)ws_info.c_str() };
		g_far.SendDlgMessage(_dlg, DM_SETTEXT, info_text_dlgid, (LONG_PTR)&dd_info);

		ImageView::DenoteInfoAndPan(info, pan);
	}

public:
	bool may_select{false};
	bool full_size{false};

	ImageViewAtFull(size_t initial_file, const std::vector<std::pair<std::string, bool> > &all_files)
		: ImageView(initial_file, all_files)
	{
	}

	bool Setup(SMALL_RECT &rc, HANDLE dlg)
	{
		_dlg = dlg;
		return ImageView::Setup(rc);
	}

	void DraggingMove(COORD pos)
	{
		if (!_dragging) {
			_dragging = true;
			_drag_pending = COORD{};
			if (!WINPORT(GetConsoleImageCaps)(NULL, sizeof(_drag_wgi), &_drag_wgi)) {
				fprintf(stderr, "%s: GetConsoleImageCaps failed\n", __FUNCTION__);
				_drag_wgi.PixPerCell = COORD{}; // essentially disable dragging
			}
		} else {
			_drag_pending.X+= SHORT((_drag_prev_pos.X - pos.X) * _drag_wgi.PixPerCell.X);
			_drag_pending.Y+= SHORT((_drag_prev_pos.Y - pos.Y) * _drag_wgi.PixPerCell.Y);
		}
		_drag_prev_pos = pos;
	}

	void DraggingApplyMoves()
	{
		if (_drag_pending.X != 0 || _drag_pending.Y != 0) {
			COORD actual = ShiftByPixels(_drag_pending);
			if (_dragging) {
				_drag_pending.X-= actual.X;
				_drag_pending.Y-= actual.Y;
			} else {
				_drag_pending = COORD{};
			}
		}
	}

	void DraggingFinish()
	{
		if (_dragging) {
			_dragging = false;
			DraggingApplyMoves();
		}
	}
};

static LONG_PTR WINAPI ImageDlgProc(HANDLE hDlg, int Msg, int Param1, LONG_PTR Param2)
{
	switch (Msg) {
		case DN_MOUSEEVENT:
		{
			ImageViewAtFull *iv = (ImageViewAtFull *)g_far.SendDlgMessage(hDlg, DM_GETDLGDATA, 0, 0);
			const MOUSE_EVENT_RECORD *me = (const MOUSE_EVENT_RECORD *)Param2;
			if ( (me->dwButtonState & RIGHTMOST_BUTTON_PRESSED) != 0 && (me->dwEventFlags & MOUSE_MOVED)  == 0) {
				if ((me->dwControlKeyState & (SHIFT_PRESSED | LEFT_CTRL_PRESSED | RIGHT_CTRL_PRESSED)) != 0) {
					iv->Rotate(-90);
				} else {
					iv->Rotate(90);
				}

			} else if ((me->dwButtonState & FROM_LEFT_1ST_BUTTON_PRESSED) != 0) {
				iv->DraggingMove(me->dwMousePosition);
				if (!WINPORT(WaitConsoleInput)(NULL, 0)) { // avoid movements 'accumulation'
					iv->DraggingApplyMoves();
				}
			} else {
				iv->DraggingFinish();
			}
		}
		return TRUE;

		case DN_INITDIALOG:
		{
			g_far.SendDlgMessage(hDlg, DM_SETDLGDATA, 0, Param2);
			SMALL_RECT rc;
			g_far.AdvControl(g_far.ModuleNumber, ACTL_GETFARRECT, &rc, 0);

			ImageViewAtFull *iv = (ImageViewAtFull *)Param2;
			if (!iv->full_size) {
				RectReduce(rc);
			}
			if (iv->Setup(rc, hDlg)) {
				g_far.SendDlgMessage(hDlg, DM_SETMOUSEEVENTNOTIFY, 1, 0);
			} else {
				g_far.SendDlgMessage(hDlg, DM_CLOSE, EXITED_DUE_ERROR, 0);
			}
		}
		return TRUE;

		case DN_KEY:
		{
			ImageViewAtFull *iv = (ImageViewAtFull *)g_far.SendDlgMessage(hDlg, DM_GETDLGDATA, 0, 0);
			const int delta = ((((int)Param2) & KEY_SHIFT) != 0) ? 1 : 10;
			const int key = (int)(Param2 & ~KEY_SHIFT);
			PurgeAccumulatedInputEvents(); // avoid navigation etc keypresses 'accumulation'
			switch (key) {
				case 'a': case 'A': case KEY_MULTIPLY: case '*':
					g_settings.SetDefaultScale(Settings::LESSOREQUAL_SCREEN);
					iv->Reset(true);
					break;
				case 'q': case 'Q': case KEY_DEL: case KEY_NUMDEL:
					g_settings.SetDefaultScale(Settings::EQUAL_SCREEN);
					iv->Reset(true);
					break;
				case 'z': case 'Z': case KEY_DIVIDE: case '/':
					g_settings.SetDefaultScale(Settings::EQUAL_IMAGE);
					iv->Reset(true);
					break;
				case KEY_CLEAR: case '=': iv->Reset(false); break;
				case KEY_ADD: case '+': case KEY_MSWHEEL_UP: iv->Scale(delta); break;
				case KEY_SUBTRACT: case '-': case KEY_MSWHEEL_DOWN: iv->Scale(-delta); break;
				case KEY_NUMPAD6: case KEY_RIGHT: iv->Shift(delta, 0); break;
				case KEY_NUMPAD4: case KEY_LEFT: iv->Shift(-delta, 0); break;
				case KEY_NUMPAD2: case KEY_DOWN: iv->Shift(0, delta); break;
				case KEY_NUMPAD8: case KEY_UP: iv->Shift(0, -delta); break;
				case KEY_NUMPAD9: iv->Shift(delta, -delta); break;
				case KEY_NUMPAD1: iv->Shift(-delta, delta); break;
				case KEY_NUMPAD3: iv->Shift(delta, delta); break;
				case KEY_NUMPAD7: iv->Shift(-delta, -delta); break;
				case KEY_TAB: iv->Rotate( (delta == 1) ? -90 : 90); break;
				case KEY_INS: case KEY_NUMPAD0:
					if (iv->may_select)
						iv->ToggleSelection();
					break;
				case KEY_SPACE:
					if (iv->may_select)
						iv->Select();
					break;
				case KEY_BS:
					if (iv->may_select)
						iv->Deselect();
					break;
				case KEY_HOME: iv->Home(); break;
				case KEY_PGDN: iv->Iterate(true); break;
				case KEY_PGUP: iv->Iterate(false); break;
				case KEY_ENTER: case KEY_NUMENTER:
					g_far.SendDlgMessage(hDlg, DM_CLOSE, EXITED_DUE_ENTER, 0);
					break;
				case KEY_ESC: case KEY_F10:
					g_far.SendDlgMessage(hDlg, DM_CLOSE, EXITED_DUE_ESCAPE, 0);
					break;
				case KEY_F7: case 'h': case 'H': iv->MirrorH(); break;
				case KEY_F8: case 'v': case 'V': iv->MirrorV(); break;
				case KEY_F5: case 'f': case 'F':
					iv->full_size = !iv->full_size;
					g_far.SendDlgMessage(hDlg, DM_CLOSE, EXITED_DUE_RESIZE, 0);
					break;
				case KEY_F9:
					WINPORT(DeleteConsoleImage)(NULL, WINPORT_IMAGE_ID);
					g_settings.ConfigurationDialog();
					iv->ForceShow();
					break;
				case KEY_F1:
					WINPORT(DeleteConsoleImage)(NULL, WINPORT_IMAGE_ID);
					g_far.ShowHelp(g_far.ModuleName, L"Contents", FHELP_USECONTENTS);
					iv->ForceShow();
					break;
				case KEY_F4:
					iv->RunProcessingCommand();
					break;
			}
		}
		return TRUE;

		case DN_CLOSE:
			WINPORT(DeleteConsoleImage)(NULL, WINPORT_IMAGE_ID);
			break;

		case DN_RESIZECONSOLE:
			g_far.SendDlgMessage(hDlg, DM_CLOSE, EXITED_DUE_RESIZE, 0);
			break;

		case DN_DRAGGED:
			return FALSE;
	}

	return g_far.DefDlgProc(hDlg, Msg, Param1, Param2);
}

static EXITED_DUE ShowImageAtFullInternal(size_t initial_file, std::vector<std::pair<std::string, bool> > &all_files, std::unordered_set<std::string> *selection, bool silent_exit_on_error)
{
	ImageViewAtFull iv(initial_file, all_files);
	if (selection) {
		iv.may_select = true;
	}

	for (;;) {
		SMALL_RECT rc;
		g_far.AdvControl(g_far.ModuleNumber, ACTL_GETFARRECT, &rc, 0);

		std::wstring hint;
		hint+= L' ';
		if (all_files.size() > 1) {
			hint+= g_settings.Msg(M_HINT_NAVIGATE);
			hint+= L" | ";
		}
		hint+= g_settings.Msg(M_HINT_PAN);
		hint+= L" | ";
		if (selection) {
			hint+= g_settings.Msg(M_HINT_SELECTION);
			hint+= L" | ";
		}
		hint+= g_settings.Msg(M_HINT_OTHER);
		hint+= L' ';

		FarDialogItem DlgItems[] = {
			{ DI_SINGLEBOX, 0, 0, rc.Right, rc.Bottom, FALSE, {}, DIF_SHOWAMPERSAND, 0, L"???", 0 },
			{ DI_DOUBLEBOX, 0, 0, rc.Right, rc.Bottom, FALSE, {}, DIF_HIDDEN | DIF_SHOWAMPERSAND, 0, L"???", 0 },
			{ DI_USERCONTROL, 1, 1, rc.Right - 1, rc.Bottom - 1, 0, {COL_DIALOGBOX}, 0, 0, L"", 0},
			{ DI_TEXT, 0, rc.Bottom, rc.Right, rc.Bottom, 0, {}, DIF_CENTERTEXT | DIF_SHOWAMPERSAND, 0, hint.c_str(), 0},
			{ DI_TEXT, rc.Left + 1, rc.Top, rc.Left + 1, rc.Top, 0, {}, DIF_SHOWAMPERSAND, 0, L"", 0},
			{ DI_TEXT, rc.Right - 1, rc.Top, rc.Right - 1, rc.Top, 0, {}, DIF_SHOWAMPERSAND, 0, L"", 0},
		};

		HANDLE dlg = g_far.DialogInit(g_far.ModuleNumber, 0, 0, rc.Right, rc.Bottom,
							 L"ImageViewer", DlgItems, sizeof(DlgItems)/sizeof(DlgItems[0]),
							 0, FDLG_NODRAWSHADOW|FDLG_NODRAWPANEL, ImageDlgProc, (LONG_PTR)&iv);

		auto exit_code = EXITED_DUE_ERROR;
		if (dlg != INVALID_HANDLE_VALUE) {
			exit_code = (EXITED_DUE)g_far.DialogRun(dlg);
			g_far.DialogFree(dlg);
		}

		if (exit_code != EXITED_DUE_RESIZE) {
			if (exit_code == EXITED_DUE_ENTER) {
				if (selection) {
					*selection = std::move(iv.GetSelection());
				}
			} else if (exit_code == EXITED_DUE_ERROR && !silent_exit_on_error) {
				std::wstring ws_cur_file = L"\"" + StrMB2Wide(all_files[initial_file].first) + L"\"";
				std::wstring werr_str = StrMB2Wide(iv.ErrorString());
				const wchar_t *MsgItems[] = {g_settings.Msg(M_TITLE),
					L"Failed to load image file:",
					ws_cur_file.c_str(),
					werr_str.c_str(),
					L"Ok"
				};
				g_far.Message(g_far.ModuleNumber, FMSG_WARNING, nullptr, MsgItems, ARRAYSIZE(MsgItems), 1);
			}
			return exit_code;
		}
	}
}

EXITED_DUE ShowImageAtFull(size_t initial_file, std::vector<std::pair<std::string, bool> > &all_files, std::unordered_set<std::string> &selection, bool silent_exit_on_error)
{
	return ShowImageAtFullInternal(initial_file, all_files, &selection, silent_exit_on_error);
}

EXITED_DUE ShowImageAtFull(const std::string &file, bool silent_exit_on_error)
{
	std::vector<std::pair<std::string, bool> > all_files{{file, false}};
	return ShowImageAtFullInternal(0, all_files, nullptr, silent_exit_on_error);
}
