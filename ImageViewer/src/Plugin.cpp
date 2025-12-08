#include "Common.h"
#include "ImageView.h"
#include "Settings.h"

PluginStartupInfo g_far;
FarStandardFunctions g_fsf;

void PurgeAccumulatedInputEvents()
{ // just purge all currently queued keypresses
	WINPORT(CheckForKeyPress)(NULL, NULL, 0, CFKP_KEEP_OTHER_EVENTS);
}

bool CheckForEscAndPurgeAccumulatedInputEvents()
{ // if ESC pressed: purge all currently queued keypresses and return true, else: just return false
	WORD keys[] = {VK_ESCAPE};
	if (WINPORT(CheckForKeyPress)(NULL, keys, ARRAYSIZE(keys),
		CFKP_KEEP_MATCHED_KEY_EVENTS | CFKP_KEEP_UNMATCHED_KEY_EVENTS | CFKP_KEEP_MOUSE_EVENTS | CFKP_KEEP_OTHER_EVENTS) == 0) {
		return false;
	}

	PurgeAccumulatedInputEvents();
	return true;
}


void RectReduce(SMALL_RECT &rc)
{
	if (rc.Right - rc.Left >= 2) {
		rc.Left++;
		rc.Right--;
	}
	if (rc.Bottom - rc.Top >= 2) {
		rc.Top++;
		rc.Bottom--;
	}
}


SHAREDSYMBOL void WINAPI SetStartupInfoW(const struct PluginStartupInfo *Info)
{
	g_far = *Info;
	g_fsf = *Info->FSF;
}

SHAREDSYMBOL void WINAPI GetPluginInfoW(struct PluginInfo *Info)
{
	Info->StructSize = sizeof(struct PluginInfo);
	Info->Flags = PF_VIEWER;

	static const wchar_t *menu_strings[1];
	menu_strings[0] = g_settings.Msg(M_TITLE);

    Info->PluginConfigStrings = menu_strings;
    Info->PluginConfigStringsNumber = 1;
	Info->PluginMenuStrings = menu_strings;
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

static std::string GetCurrentPanelItem()
{
	const auto &fn_sel = GetPanelItem(FCTL_GETCURRENTPANELITEM, 0);
	return fn_sel.first;
}

static ssize_t GetPanelItemsForView(const std::string &name, std::vector<std::pair<std::string, bool>> &all_files)
{
	PanelInfo pi{};
	g_far.Control(PANEL_ACTIVE, FCTL_GETPANELINFO, 0, (LONG_PTR)&pi);
	if (pi.ItemsNumber <= 0) {
		return -1;
	}
	if (pi.PanelType != PTYPE_FILEPANEL || (pi.Plugin && !(pi.Flags & PFLAGS_REALNAMES))) {
		const wchar_t *MsgItems[] = { g_settings.Msg(M_TITLE),
			L"The active panel must be with files accessible locally via real path",
			L"Close"
		};
		g_far.Message(g_far.ModuleNumber, FMSG_WARNING, nullptr, MsgItems, ARRAYSIZE(MsgItems), 1);
		return -1;
	}

	bool has_real_selection = false;
	ssize_t cur = -1;
	const auto &cur_fn = GetCurrentPanelItem();
	for (int i = 0; i < pi.ItemsNumber; ++i) {
		const auto &fn_sel = GetPanelItem(FCTL_GETPANELITEM, i);
		if (!fn_sel.first.empty() && fn_sel.first != "." && fn_sel.first != "..") {
			if (fn_sel.second) {
				has_real_selection = true;
			}
			if (fn_sel.first == cur_fn) {
				cur = all_files.size();
			}
			all_files.emplace_back(fn_sel);
		}
	}

	std::vector<std::pair<std::string, bool>> chosen_files;
	const auto saved_cur = cur;
	for (ssize_t i = 0; i != (ssize_t)all_files.size(); ++i) {
		if (i == saved_cur) {
			cur = chosen_files.size();
			chosen_files.emplace_back(all_files[i]);
		} else {
			if (has_real_selection) { // if some has marking selection - choose all such files
				if (all_files[i].second) {
					chosen_files.emplace_back(all_files[i]);
				}
			} else if (g_settings.MatchFile(all_files[i].first.c_str())) { // no marking selection - choose name-matching files
				chosen_files.emplace_back(all_files[i]);
			}
		}
	}
	all_files = std::move(chosen_files);

	return all_files.empty() ? -1 : cur;
}

static EXITED_DUE OpenPluginAtCurrentPanel(const std::string &name)
{
	std::vector<std::pair<std::string, bool>> all_items;
	ssize_t initial_file = GetPanelItemsForView(name, all_items);
	if (initial_file < 0) {
		return EXITED_DUE_ERROR;
	}

	std::unordered_set<std::string> selection;
	auto ed = ShowImageAtFull(initial_file, all_items, selection, false);
	if (ed == EXITED_DUE_ENTER) {
		PanelInfo pi{};
		g_far.Control(PANEL_ACTIVE, FCTL_GETPANELINFO, 0, (LONG_PTR)&pi);
		if (pi.ItemsNumber > 0) {
			std::vector<bool> selection_to_apply;
			selection_to_apply.reserve(pi.ItemsNumber);
			for (int i = 0; i < pi.ItemsNumber; ++i) {
				const auto &fn_sel = GetPanelItem(FCTL_GETPANELITEM, i);
				selection_to_apply.emplace_back(selection.find(fn_sel.first) != selection.end());
			}
			g_far.Control(PANEL_ACTIVE, FCTL_BEGINSELECTION, 0, 0);
			for (size_t i = 0; i < selection_to_apply.size(); ++i) {
				BOOL selected = selection_to_apply[i] ? TRUE : FALSE;
				g_far.Control(PANEL_ACTIVE, FCTL_SETSELECTION, i, (LONG_PTR)selected);
			}
			g_far.Control(PANEL_ACTIVE,FCTL_ENDSELECTION, 0, 0);
			g_far.Control(PANEL_ACTIVE,FCTL_REDRAWPANEL, 0, 0);
		}
	}
	return ed;
}

static EXITED_DUE OpenPluginAtSomePath(const std::string &name, bool silent)
{
	return ShowImageAtFull(name, silent);
}

static void OpenAtCurrentPanelItem()
{
	const auto &fn_sel = GetCurrentPanelItem();
	if (!fn_sel.empty()) {
		struct PanelInfo pi{};
		g_far.Control(PANEL_PASSIVE, FCTL_GETPANELINFO, 0, (LONG_PTR)&pi);
		if (pi.Visible && pi.PanelType == PTYPE_QVIEWPANEL) {
			ShowImageAtQV(fn_sel,
				SMALL_RECT {
					SHORT(pi.PanelRect.left), SHORT(pi.PanelRect.top),
					SHORT(pi.PanelRect.right), SHORT(pi.PanelRect.bottom)
				}
			);
		} else {
			OpenPluginAtCurrentPanel(fn_sel);
		}
	}
}

SHAREDSYMBOL HANDLE WINAPI OpenPluginW(int OpenFrom, INT_PTR Item)
{
	if (OpenFrom == OPEN_PLUGINSMENU) {
		OpenAtCurrentPanelItem();

	} else if (OpenFrom == OPEN_VIEWER) {
		ViewerInfo vi{sizeof(ViewerInfo), 0};
		if (g_far.ViewerControl(VCTL_GETINFO, &vi)) {
			if (vi.FileName && g_settings.MatchFile(Wide2MB(vi.FileName).c_str())) {
				if (OpenPluginAtSomePath(Wide2MB(vi.FileName), false) != EXITED_DUE_ERROR) {
					// to quit or not to quit?
					// g_far.ViewerControl(VCTL_QUIT, 0);
				}
			}
		}

	} else if (Item > 0xfff) {
		std::string path = Wide2MB((const wchar_t *)Item);
		while (!path.empty() && path.front() == ' ') {
			path.erase(0, 1);
		}
		for (size_t i = 0; i + 1 < path.size(); ++i) {
			if (path[i] == '\\') {
				path.erase(i, 1);
			}
		}
		while (strncmp(path.c_str(), "./", 2) == 0) {
			path.erase(0, 2);
		}
		if (path.empty()) {
			OpenAtCurrentPanelItem();
		} else if (path.find('/') == std::string::npos) {
			OpenPluginAtCurrentPanel(path);
		} else {
			OpenPluginAtSomePath(path, false);
		}
	}

	return INVALID_HANDLE_VALUE;
}

SHAREDSYMBOL void WINAPI ExitFARW(void)
{
	DismissImageAtQV();
}


SHAREDSYMBOL void WINAPI ClosePluginW(HANDLE hPlugin) {}
SHAREDSYMBOL int WINAPI ProcessEventW(HANDLE hPlugin, int Event, void *Param) { return FALSE; }

SHAREDSYMBOL int WINAPI ProcessViewerEventW(int Event,void *Param)
{
	if (Event == VE_READ || Event == VE_CLOSE) {
		struct PanelInfo pi{};
		g_far.Control(PANEL_PASSIVE, FCTL_GETPANELINFO, 0, (LONG_PTR)&pi);
		if (pi.Visible && pi.PanelType == PTYPE_QVIEWPANEL && (IsShowingImageAtQV() || g_settings.OpenInQV())) {
			const auto &fn_sel = GetCurrentPanelItem();
			if (!fn_sel.empty() && (IsShowingImageAtQV() || g_settings.MatchFile(fn_sel.c_str()))) {
				ShowImageAtQV(fn_sel,
					SMALL_RECT{
						SHORT(pi.PanelRect.left), SHORT(pi.PanelRect.top),
						SHORT(pi.PanelRect.right), SHORT(pi.PanelRect.bottom)
					}
				);
			}
		} else {
			if (IsShowingImageAtQV()) {
				DismissImageAtQV();
			}
						
			if (Event == VE_READ && (!pi.Visible || pi.PanelType != PTYPE_QVIEWPANEL) && g_settings.OpenInFV()) {
				ViewerInfo vi{sizeof(ViewerInfo), 0};
				if (g_far.ViewerControl(VCTL_GETINFO, &vi)) {
					if (vi.FileName && g_settings.MatchFile(Wide2MB(vi.FileName).c_str())) {
						OpenPluginAtSomePath(Wide2MB(vi.FileName), true);
					}
				}
			}
		}
	}
	return 0;
}

SHAREDSYMBOL int WINAPI ConfigureW(int ItemNumber)
{
	g_settings.ConfigurationDialog();
	return 1;
}

SHAREDSYMBOL HANDLE WINAPI _export OpenFilePluginW(const wchar_t *Name, const unsigned char *Data, int DataSize, int OpMode)
{
	if (Name) {
		if ((OpMode == OPM_PGDN && g_settings.OpenByCtrlPgDn())
		 || (OpMode  == 0 && g_settings.OpenByEnter())) {
			const wchar_t *slash = wcsrchr(Name, L'/');
			const auto &name_mb = Wide2MB(slash ? slash + 1 : Name);
			if (g_settings.MatchFile(name_mb.c_str())) {
				if (OpenPluginAtCurrentPanel(name_mb) != EXITED_DUE_ERROR) {
					return (HANDLE)-2;
				}
			}
		}
	}

	return INVALID_HANDLE_VALUE;
}
