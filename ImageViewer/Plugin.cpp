#include "ImageViewer.h"
#include "Common.h"

PluginStartupInfo g_far;
FarStandardFunctions g_fsf;

static void PurgeAccumulatedKeyPresses()
{
	// just purge all currently queued keypresses
	WINPORT(CheckForKeyPress)(NULL, NULL, 0, CFKP_KEEP_OTHER_EVENTS | CFKP_KEEP_MOUSE_EVENTS);
}


static LONG_PTR WINAPI ViewerDlgProc(HANDLE hDlg, int Msg, int Param1, LONG_PTR Param2)
{
	switch(Msg) {
		case DN_INITDIALOG:
		{
			g_far.SendDlgMessage(hDlg, DM_SETDLGDATA, 0, Param2);

			SMALL_RECT Rect;
			g_far.AdvControl(g_far.ModuleNumber, ACTL_GETFARRECT, &Rect, 0);

			ImageViewer *iv = (ImageViewer *)Param2;

			if (!iv->Setup(hDlg, Rect)) {
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

static bool ShowImage(const std::string &initial_file, std::set<std::string> &selection)
{
	ImageViewer iv(initial_file, selection);

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
							 0, FDLG_NODRAWSHADOW|FDLG_NODRAWPANEL, ViewerDlgProc, (LONG_PTR)&iv);

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

// --- Экспортируемые функции плагина ---

SHAREDSYMBOL void WINAPI SetStartupInfoW(const struct PluginStartupInfo *Info)
{
	g_far = *Info;
	g_fsf = *Info->FSF;
}

SHAREDSYMBOL void WINAPI GetPluginInfoW(struct PluginInfo *Info)
{
	Info->StructSize = sizeof(struct PluginInfo);
	Info->Flags = 0;

	static const wchar_t *PluginMenuStrings[1];
	PluginMenuStrings[0] = PLUGIN_TITLE;

	Info->PluginMenuStrings = PluginMenuStrings;
	Info->PluginMenuStringsNumber = 1;

	Info->CommandPrefix = L"img:";
}

static std::pair<std::string, bool> GetPanelItem(int cmd, int index)
{
	std::pair<std::string, bool> out;
	out.second = false;
	size_t sz = g_far.Control(PANEL_ACTIVE, cmd, index, 0);
	if (sz) {
		std::vector<char> buf(sz + 16);
		sz = g_far.Control(PANEL_ACTIVE, cmd, index, (LONG_PTR)buf.data());
		const PluginPanelItem *ppi = (const PluginPanelItem *)buf.data();
		if (ppi->FindData.lpwszFileName && *ppi->FindData.lpwszFileName) {
			out.first = Wide2MB(ppi->FindData.lpwszFileName);
			out.second = (ppi->Flags & PPIF_SELECTED) != 0;
		}
	}
	return out;
}

static std::set<std::string> GetSelectedItems()
{
	std::set<std::string> out;
	PanelInfo pi{};
	g_far.Control(PANEL_ACTIVE, FCTL_GETPANELINFO, 0, (LONG_PTR)&pi);
	if (pi.SelectedItemsNumber > 0 && pi.PanelType == PTYPE_FILEPANEL) {
		for (int i = 0; i < pi.SelectedItemsNumber; ++i) {
			const auto &fn_sel = GetPanelItem(FCTL_GETSELECTEDPANELITEM, i);
			if (!fn_sel.first.empty() && fn_sel.second) {
				out.insert(fn_sel.first);
			}
		}
	}
	return out;
}

static std::vector<std::string> GetAllItems()
{
	std::vector<std::string> out;
	PanelInfo pi{};
	g_far.Control(PANEL_ACTIVE, FCTL_GETPANELINFO, 0, (LONG_PTR)&pi);
	for (int i = 0; i < pi.ItemsNumber; ++i) {
		auto fn_sel = GetPanelItem(FCTL_GETPANELITEM, i);
		if (!fn_sel.first.empty()) {
			out.emplace_back(std::move(fn_sel.first));
		}
	}
	return out;
}

static void OpenPluginAtCurrentPanel(const std::string &name)
{
	std::set<std::string> _selection = GetSelectedItems();
	if (ShowImage(name, _selection)) {
		std::vector<std::string> all_items = GetAllItems();
		g_far.Control(PANEL_ACTIVE, FCTL_BEGINSELECTION, 0, 0);
		for (size_t i = 0; i < all_items.size(); ++i) {
			BOOL selected = _selection.find(all_items[i]) != _selection.end();
			g_far.Control(PANEL_ACTIVE, FCTL_SETSELECTION, i, (LONG_PTR)selected);
		}
		g_far.Control(PANEL_ACTIVE,FCTL_ENDSELECTION, 0, 0);
		g_far.Control(PANEL_ACTIVE,FCTL_REDRAWPANEL, 0, 0);
	}
}

static void OpenPluginAtSomePath(const std::string &name)
{
	std::set<std::string> selection;
	if (ShowImage(name, selection)) {
		if (!selection.empty()) {
			const wchar_t *MsgItems[] = { PLUGIN_TITLE,
				L"Selection will not be applied cuz was invoked for non-panel file",
				L"Ok"
			};
			g_far.Message(g_far.ModuleNumber, FMSG_WARNING, nullptr, MsgItems, ARRAYSIZE(MsgItems), 1);
		}
	}
}

SHAREDSYMBOL HANDLE WINAPI OpenPluginW(int OpenFrom, INT_PTR Item)
{
	if (OpenFrom == OPEN_PLUGINSMENU) {
		const auto &fn_sel = GetPanelItem(FCTL_GETCURRENTPANELITEM, 0);
		if (!fn_sel.first.empty()) {
			OpenPluginAtCurrentPanel(fn_sel.first);
		}
	} else if (Item > 0xfff) {
		const wchar_t *wide_path = (const wchar_t *)Item;
		while (wcsncmp(wide_path, L"./", 2) == 0) {
			wide_path+= 2;
		}
		const auto &path = Wide2MB(wide_path);
		if (path.find('/') == std::string::npos) {
			OpenPluginAtCurrentPanel(path);
		} else {
			OpenPluginAtSomePath(path);
		}
	}

	return INVALID_HANDLE_VALUE;
}

SHAREDSYMBOL void WINAPI ClosePluginW(HANDLE hPlugin) {}
SHAREDSYMBOL int WINAPI ProcessEventW(HANDLE hPlugin, int Event, void *Param) { return FALSE; }
