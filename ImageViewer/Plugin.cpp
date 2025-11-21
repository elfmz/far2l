#include "Common.h"
#include "ImageViewer.h"

PluginStartupInfo g_far;
FarStandardFunctions g_fsf;

void PurgeAccumulatedKeyPresses()
{ // just purge all currently queued keypresses
	WINPORT(CheckForKeyPress)(NULL, NULL, 0, CFKP_KEEP_OTHER_EVENTS | CFKP_KEEP_MOUSE_EVENTS);
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

static bool GetInitialPanelItems(const std::string &name, std::vector<std::string> &all_files, std::set<std::string> &selection)
{
	PanelInfo pi{};
	g_far.Control(PANEL_ACTIVE, FCTL_GETPANELINFO, 0, (LONG_PTR)&pi);
	if (pi.PanelType != PTYPE_FILEPANEL || pi.ItemsNumber <= 0)
		return false;
	if (pi.Plugin && !(pi.Flags & PFLAGS_REALNAMES)) {
		const wchar_t *MsgItems[] = { PLUGIN_TITLE,
			L"The active panel must be with files accessible locally via real path",
			L"Close"
		};
		g_far.Message(g_far.ModuleNumber, FMSG_WARNING, nullptr, MsgItems, ARRAYSIZE(MsgItems), 1);
		return false;
	}
	all_files.clear();
	selection.clear();
	if (pi.SelectedItemsNumber > 0) {
		for (int i = 0; i < pi.SelectedItemsNumber; ++i) {
			const auto &fn_sel = GetPanelItem(FCTL_GETSELECTEDPANELITEM, i);
			if (!fn_sel.first.empty() && fn_sel.second) {
				all_files.push_back(fn_sel.first);
				selection.insert(fn_sel.first);
			}
	}
	if (selection.empty()) { // if no selected files add all files from panel
		for (int i = 0; i < pi.ItemsNumber; ++i) {
			const auto &fn = GetPanelItem(FCTL_GETPANELITEM, i);
			if (!fn.first.empty())
				all_files.push_back(fn.first);
			}
		}
	}
	return true;
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
	std::vector<std::string> _all_files;
	std::set<std::string> _selection;
	if (!GetInitialPanelItems(name, _all_files, _selection))
		return;
	if (ShowImageAtFull(name, _all_files, _selection)) {
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
	std::vector<std::string> all_files;
	std::set<std::string> selection;
	if (ShowImageAtFull(name, all_files, selection)) {
		if (!selection.empty()) {
			const wchar_t *MsgItems[] = { PLUGIN_TITLE,
				L"Selection will not be applied cuz was invoked for non-panel file",
				L"Ok"
			};
			g_far.Message(g_far.ModuleNumber, FMSG_WARNING, nullptr, MsgItems, ARRAYSIZE(MsgItems), 1);
		}
	}
}

std::string GetCurrentPanelItem()
{
	const auto &fn_sel = GetPanelItem(FCTL_GETCURRENTPANELITEM, 0);
	return fn_sel.first;
}

SHAREDSYMBOL HANDLE WINAPI OpenPluginW(int OpenFrom, INT_PTR Item)
{
	if (OpenFrom == OPEN_PLUGINSMENU) {
		const auto &fn_sel = GetCurrentPanelItem();
		if (!fn_sel.empty()) {
			struct PanelInfo pi{};
			g_far.Control(PANEL_PASSIVE, FCTL_GETPANELINFO, 0, (LONG_PTR)&pi);
			if (pi.Visible && pi.PanelType == PTYPE_QVIEWPANEL) {
				ShowImageAtQV();
			} else {
				OpenPluginAtCurrentPanel(fn_sel);
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
		if (path.find('/') == std::string::npos) {
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
