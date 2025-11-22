#include "Common.h"
#include "ImageView.h"

PluginStartupInfo g_far;
FarStandardFunctions g_fsf;

void PurgeAccumulatedInputEvents()
{ // just purge all currently queued keypresses
	WINPORT(CheckForKeyPress)(NULL, NULL, 0, CFKP_KEEP_OTHER_EVENTS);
}


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

static std::string GetCurrentPanelItem()
{
	const auto &fn_sel = GetPanelItem(FCTL_GETCURRENTPANELITEM, 0);
	return fn_sel.first;
}

static ssize_t GetInitialPanelItems(const std::string &name, std::vector<std::pair<std::string, bool>> &all_files)
{
	PanelInfo pi{};
	g_far.Control(PANEL_ACTIVE, FCTL_GETPANELINFO, 0, (LONG_PTR)&pi);
	if (pi.ItemsNumber <= 0) {
		return -1;
	}
	if (pi.PanelType != PTYPE_FILEPANEL || (pi.Plugin && !(pi.Flags & PFLAGS_REALNAMES))) {
		const wchar_t *MsgItems[] = { PLUGIN_TITLE,
			L"The active panel must be with files accessible locally via real path",
			L"Close"
		};
		g_far.Message(g_far.ModuleNumber, FMSG_WARNING, nullptr, MsgItems, ARRAYSIZE(MsgItems), 1);
		return -1;
	}

	if (pi.SelectedItemsNumber <= 0) abort();

	bool has_real_selection = false;
	ssize_t cur = 0;
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

	if (has_real_selection) { // leave only selected files, and current one
		std::vector<std::pair<std::string, bool>> selected_files;
		const auto saved_cur = cur;
		for (ssize_t i = 0; i != (ssize_t)all_files.size(); ++i) {
			if (i == saved_cur) {
				cur = selected_files.size();
				selected_files.emplace_back(all_files[i]);
			} else if (all_files[i].second) {
				selected_files.emplace_back(all_files[i]);
			}
		}
		all_files = std::move(selected_files);
	}


	return cur;
}

static void OpenPluginAtCurrentPanel(const std::string &name)
{
	std::vector<std::pair<std::string, bool>> all_items;
	ssize_t initial_file = GetInitialPanelItems(name, all_items);
	if (initial_file < 0) {
		return;
	}

	std::unordered_set<std::string> selection;
	if (ShowImageAtFull(initial_file, all_items, selection)) {
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
}

static void OpenPluginAtSomePath(const std::string &name)
{
	std::vector<std::pair<std::string, bool> > all_files{{name, false}};
	std::unordered_set<std::string> selection;
	if (ShowImageAtFull(0, all_files, selection)) {
		if (!selection.empty()) {
			const wchar_t *MsgItems[] = { PLUGIN_TITLE,
				L"Selection will not be applied cuz was invoked for non-panel file",
				L"Ok"
			};
			g_far.Message(g_far.ModuleNumber, FMSG_WARNING, nullptr, MsgItems, ARRAYSIZE(MsgItems), 1);
		}
	}
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
			OpenPluginAtSomePath(path);
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
	if ((Event == VE_READ || Event == VE_CLOSE) && IsShowingImageAtQV()) {
		bool dismiss = true;
		const auto &fn_sel = GetCurrentPanelItem();
		if (!fn_sel.empty()) {
			struct PanelInfo pi{};
			g_far.Control(PANEL_PASSIVE, FCTL_GETPANELINFO, 0, (LONG_PTR)&pi);
			if (pi.Visible && pi.PanelType == PTYPE_QVIEWPANEL) {
				ShowImageAtQV(fn_sel,
					SMALL_RECT{
						SHORT(pi.PanelRect.left), SHORT(pi.PanelRect.top),
						SHORT(pi.PanelRect.right), SHORT(pi.PanelRect.bottom)
					}
				);
				dismiss = false;
			}
		}
		if (dismiss) {
			DismissImageAtQV();
		}
	}
	return 0;
}

