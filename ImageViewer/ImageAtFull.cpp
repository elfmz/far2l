#include "Common.h"
#include "ImageViewer.h"

class ImageViewerAtFull : public ImageViewer
{
	WinportGraphicsInfo _drag_wgi{};
	COORD _drag_prev_pos{}, _drag_pending{};
	bool _dragging{false};

public:
	ImageViewerAtFull(const std::string &initial_file, const std::set<std::string> &selection)
		: ImageViewer(initial_file, selection)
	{
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

	void DraggingFinish()
	{
		_dragging = false;
	}

	void DraggingCommit()
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
};

static LONG_PTR WINAPI DlgProcAtMax(HANDLE hDlg, int Msg, int Param1, LONG_PTR Param2)
{
	switch(Msg) {
		case DN_ENTERIDLE:
		{
			ImageViewerAtFull *iv = (ImageViewerAtFull *)g_far.SendDlgMessage(hDlg, DM_GETDLGDATA, 0, 0);
			iv->DraggingCommit();
			return TRUE;
		}
		case DN_MOUSEEVENT:
		{
			ImageViewerAtFull *iv = (ImageViewerAtFull *)g_far.SendDlgMessage(hDlg, DM_GETDLGDATA, 0, 0);
			const MOUSE_EVENT_RECORD *me = (const MOUSE_EVENT_RECORD *)Param2;
			if ((me->dwButtonState & FROM_LEFT_1ST_BUTTON_PRESSED) != 0) {
				iv->DraggingMove(me->dwMousePosition);
			} else {
				iv->DraggingFinish();
			}
			if ((me->dwControlKeyState & (SHIFT_PRESSED | LEFT_CTRL_PRESSED | RIGHT_CTRL_PRESSED)) != 0) {
				iv->DraggingCommit();
			}
			return TRUE;
		}

		case DN_INITDIALOG:
		{
			g_far.SendDlgMessage(hDlg, DM_SETDLGDATA, 0, Param2);

			SMALL_RECT Rect;
			g_far.AdvControl(g_far.ModuleNumber, ACTL_GETFARRECT, &Rect, 0);

			ImageViewerAtFull *iv = (ImageViewerAtFull *)Param2;

			if (Rect.Right - Rect.Left > 2) {
				Rect.Left++;
				Rect.Right--;
			}
			if (Rect.Bottom - Rect.Top > 2) {
				Rect.Top++;
				Rect.Bottom--;
			}
			if (iv->SetupFull(Rect, hDlg)) {
				g_far.SendDlgMessage(hDlg, DM_SETMOUSEEVENTNOTIFY, 1, 0);
			} else {
				g_far.SendDlgMessage(hDlg, DM_CLOSE, EXITED_DUE_ERROR, 0);
			}

			return TRUE;
		}

		case DN_KEY:
		{
			ImageViewerAtFull *iv = (ImageViewerAtFull *)g_far.SendDlgMessage(hDlg, DM_GETDLGDATA, 0, 0);
			const int delta = ((((int)Param2) & KEY_SHIFT) != 0) ? 1 : 10;
			const int key = (int)(Param2 & ~KEY_SHIFT);
			PurgeAccumulatedKeyPresses(); // avoid navigation etc keypresses 'accumulation'
			switch (key) {
				case 'a': case 'A': case KEY_MULTIPLY: case '*':
					g_def_scale = DS_LESSOREQUAL_SCREEN;
					iv->Reset();
					break;
				case 'q': case 'Q': case KEY_DEL: case KEY_NUMDEL:
					g_def_scale = DS_EQUAL_SCREEN;
					iv->Reset();
					break;
				case 'z': case 'Z': case KEY_DIVIDE: case '/':
					g_def_scale = DS_EQUAL_IMAGE;
					iv->Reset();
					break;
				case KEY_CLEAR: case '=': iv->Reset(); break;
				case KEY_ADD: case '+': iv->Scale(delta); break;
				case KEY_SUBTRACT: case '-': iv->Scale(-delta); break;
				case KEY_NUMPAD6: case KEY_RIGHT: iv->Shift(delta, 0); break;
				case KEY_NUMPAD4: case KEY_LEFT: iv->Shift(-delta, 0); break;
				case KEY_NUMPAD2: case KEY_DOWN: iv->Shift(0, delta); break;
				case KEY_NUMPAD8: case KEY_UP: iv->Shift(0, -delta); break;
				case KEY_NUMPAD9: iv->Shift(delta, -delta); break;
				case KEY_NUMPAD1: iv->Shift(-delta, delta); break;
				case KEY_NUMPAD3: iv->Shift(delta, delta); break;
				case KEY_NUMPAD7: iv->Shift(-delta, -delta); break;
				case KEY_TAB: iv->Rotate( (delta == 1) ? -90 : 90); break;
				case KEY_INS: case KEY_NUMPAD0: iv->ToggleSelection(); break;
				case KEY_SPACE: iv->Select(); break;
				case KEY_BS: iv->Deselect(); break;
				case KEY_HOME: iv->Home(); break;
				case KEY_PGDN: iv->Iterate(true); break;
				case KEY_PGUP: iv->Iterate(false); break;
				case KEY_ENTER: case KEY_NUMENTER:
					g_far.SendDlgMessage(hDlg, DM_CLOSE, EXITED_DUE_ENTER, 0);
					break;
				case KEY_ESC: case KEY_F10:
					g_far.SendDlgMessage(hDlg, DM_CLOSE, EXITED_DUE_ESCAPE, 0);
					break;
			}
			return TRUE;
		}

		case DN_CLOSE:
			WINPORT(DeleteConsoleImage)(NULL, WINPORT_IMAGE_ID);
			break;

		case DN_RESIZECONSOLE:
			g_far.SendDlgMessage(hDlg, DM_CLOSE, EXITED_DUE_RESIZE, 0);
			break;
	}

	return g_far.DefDlgProc(hDlg, Msg, Param1, Param2);
}

bool ShowImageAtFull(const std::string &initial_file, std::set<std::string> &selection)
{
	ImageViewerAtFull iv(initial_file, selection);

	for (;;) {
		SMALL_RECT Rect;
		g_far.AdvControl(g_far.ModuleNumber, ACTL_GETFARRECT, &Rect, 0);

		FarDialogItem DlgItems[] = {
			{ DI_SINGLEBOX, 0, 0, Rect.Right, Rect.Bottom, FALSE, {}, 0, 0, L"???", 0 },
			{ DI_DOUBLEBOX, 0, 0, Rect.Right, Rect.Bottom, FALSE, {}, DIF_HIDDEN, 0, L"???", 0 },
			{ DI_USERCONTROL, 1, 1, Rect.Right - 1, Rect.Bottom - 1, 0, {COL_DIALOGBOX}, 0, 0, L"", 0},
			{ DI_TEXT, 0, Rect.Bottom, Rect.Right, Rect.Bottom, 0, {}, DIF_CENTERTEXT, 0, L"", 0},
		};

		HANDLE hDlg = g_far.DialogInit(g_far.ModuleNumber, 0, 0, Rect.Right, Rect.Bottom,
							 L"ImageViewer", DlgItems, sizeof(DlgItems)/sizeof(DlgItems[0]),
							 0, FDLG_NODRAWSHADOW|FDLG_NODRAWPANEL, DlgProcAtMax, (LONG_PTR)&iv);

		if (hDlg == INVALID_HANDLE_VALUE) {
			return false;
		}

		int exit_code = g_far.DialogRun(hDlg);
		g_far.DialogFree(hDlg);

		if (exit_code != EXITED_DUE_RESIZE) {
			if (exit_code != EXITED_DUE_ENTER) {
				return false;
			}
			selection = iv.GetSelection();
			return true;
		}
	}
}
