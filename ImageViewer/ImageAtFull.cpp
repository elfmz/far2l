#include "Common.h"
#include "ImageViewer.h"

static LONG_PTR WINAPI DlgProcAtMax(HANDLE hDlg, int Msg, int Param1, LONG_PTR Param2)
{
	switch(Msg) {
		case DN_INITDIALOG:
		{
			g_far.SendDlgMessage(hDlg, DM_SETDLGDATA, 0, Param2);

			SMALL_RECT Rect;
			g_far.AdvControl(g_far.ModuleNumber, ACTL_GETFARRECT, &Rect, 0);

			ImageViewer *iv = (ImageViewer *)Param2;

			if (!iv->Setup(Rect, hDlg)) {
				g_far.SendDlgMessage(hDlg, DM_CLOSE, EXITED_DUE_ERROR, 0);
			}
			return TRUE;
		}

		case DN_KEY:
		{
			ImageViewer *iv = (ImageViewer *)g_far.SendDlgMessage(hDlg, DM_GETDLGDATA, 0, 0);
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

bool ShowImageAtFull(const std::string &initial_file, std::vector<std::string> &all_files, std::set<std::string> &selection)
{
	DismissImageAtQV();

	ImageViewer iv(initial_file, all_files, selection);

	for (;;) {
		SMALL_RECT Rect;
		g_far.AdvControl(g_far.ModuleNumber, ACTL_GETFARRECT, &Rect, 0);

		FarDialogItem DlgItems[] = {
			{ DI_SINGLEBOX, 0, 0, Rect.Right, Rect.Bottom, FALSE, {}, DIF_SHOWAMPERSAND, 0, L"???", 0 },
			{ DI_DOUBLEBOX, 0, 0, Rect.Right, Rect.Bottom, FALSE, {}, DIF_HIDDEN | DIF_SHOWAMPERSAND, 0, L"???", 0 },
			{ DI_USERCONTROL, 1, 1, Rect.Right - 1, Rect.Bottom - 1, 0, {COL_DIALOGBOX}, 0, 0, L"", 0},
			{ DI_TEXT, 0, Rect.Bottom, Rect.Right, Rect.Bottom, 0, {}, DIF_CENTERTEXT | DIF_SHOWAMPERSAND, 0, L"", 0},
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
