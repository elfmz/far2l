#include <vector>
#include <mutex>
#include <wchar.h>
#include <limits.h>
#include "Globals.h"
#include <utils.h>
#include "SitesConfig.h"
#include "PluginImpl.h"
#include "PluginPanelItems.h"
#include "PooledStrings.h"
#include "ConnectionsPool.h"
#include "GetItems.h"
#include "Host/HostLocal.h"
#include "UI/Settings/SiteConnectionEditor.h"
#include "UI/Activities/Confirm.h"
#include "Op/OpConnect.h"
#include "Op/OpXfer.h"
#include "Op/OpRemove.h"
#include "Op/OpCheckDirectory.h"
#include "Op/OpMakeDirectory.h"
#include "Op/OpEnumDirectory.h"
#include "Op/OpExecute.h"
#include "Op/OpChangeMode.h"
#include "Op/OpGetLinkTarget.h"

static std::unique_ptr<ConnectionsPool> g_conn_pool;


class AllNetRocks
{
	std::mutex _mutex;
	std::map<void *, PluginImpl *> _all;

public:
	void Add(PluginImpl *it)
	{
		std::lock_guard<std::mutex> locker(_mutex);
		if (!_all.emplace((void *)it, it).second) {
			ABORT_MSG("dup entry %p", it);
		}
	}

	void Remove(PluginImpl *it)
	{
		std::lock_guard<std::mutex> locker(_mutex);
		_all.erase((void *)it);
		if (_all.empty()) {
			PurgePooledStrings();
		}
	}

	bool GetRemoteOf(void *handle, std::shared_ptr<IHost> &remote, std::string &site_dir, SitesConfigLocation &sites_cfg_location)
	{
		std::lock_guard<std::mutex> locker(_mutex);
		auto i = _all.find(handle);
		if (i == _all.end())
			return false;

		remote = i->second->_remote;
		site_dir = i->second->CurrentSiteDir(true);
		sites_cfg_location = i->second->_sites_cfg_location;
		return true;
	}

	void CloneRemoteOf(void *handle)
	{
		std::lock_guard<std::mutex> locker(_mutex);
		auto i = _all.find(handle);
		if (i != _all.end()) {
			i->second->_remote = i->second->_remote->Clone();
		}
	}

} g_all_netrocks;

PluginImpl::PluginImpl(const wchar_t *path, bool path_is_standalone_config, int OpMode)
{
	_cur_dir[0] = _panel_title[0] = _format[0] = 0;

	_local = std::make_shared<HostLocal>();
	if (path_is_standalone_config) {
		_standalone_config = path;
		_sites_cfg_location = SitesConfigLocation(StrWide2MB(_standalone_config));

	} else {
		if (path && wcsncmp(path, L"net:", 4) == 0) {
			path+= 4;
			while (*path == L'/') {
				++path;
			}
		}

		if (path && *path) {
			NR_DBG("path: %ls", path);
			const std::string &standalone_config = StrWide2MB(_standalone_config);
			if (_location.FromString(standalone_config, Wide2MB(path))) {
				_remote = OpConnect(0, standalone_config, _location).Do();
				if (!_remote) {
					throw std::runtime_error(G.GetMsgMB(MCouldNotConnect));
				}
				ValidateLocationDirectory(OpMode);

			} else if (!_sites_cfg_location.Change(Wide2MB(path))) {
				throw std::runtime_error(G.GetMsgMB(MWrongPath));
			}

			_wea_state = std::make_shared<WhatOnErrorState>();
		}
	}

	g_all_netrocks.Add(this);
	UpdatePathInfo();
}

PluginImpl::~PluginImpl()
{
	DismissRemoteHost();
	g_all_netrocks.Remove(this);
}

void PluginImpl::UpdatePathInfo()
{
	std::wstring tmp;
	if (_remote) {
		wcsncpy(_format, StrMB2Wide(_location.server).c_str(), ARRAYSIZE(_format) - 1);
		wcsncpy(_cur_dir, StrMB2Wide(_location.ToString(true)).c_str(), ARRAYSIZE(_cur_dir) - 1 );
		// make up URL string start
		IHost::Identity identity;
		_remote->GetIdentity(identity);
		tmp = StrMB2Wide(StrPrintf("%s://", identity.protocol.c_str()));
		if (!identity.username.empty()) {
			tmp += StrMB2Wide(StrPrintf("%s@", identity.username.c_str()));
		}
		tmp += StrMB2Wide(identity.host);
		const auto *pi = ProtocolInfoLookup(identity.protocol.c_str());
		if (identity.port && pi && pi->default_port != -1 && pi->default_port != (int)identity.port) {
			tmp += StrMB2Wide(StrPrintf(":%u", identity.port));
		}
		tmp += L"/" + StrMB2Wide(_location.ToString(false));
		wcsncpy(_cur_URL, tmp.c_str(), ARRAYSIZE(_cur_URL) - 1 );
		// make up URL string end
		tmp = _cur_dir;

	} else {
		tmp = StrMB2Wide(_sites_cfg_location.TranslateToPath(false));
		wcsncpy(_cur_dir, tmp.c_str(), ARRAYSIZE(_cur_dir) - 1);
		if (!_standalone_config.empty()) {
			tmp = ExtractFileName(_standalone_config);

		} else {
			wcsncpy(_format, L"NetRocks sites", ARRAYSIZE(_format) - 1);
			if (!tmp.empty()) {
				if (tmp[tmp.size() - 1] == '/') {
					tmp.resize(tmp.size() - 1);
				}
				tmp.insert(0, L": ");
			}
			tmp.insert(0, L"NetRocks sites");
		}
	}

	if (tmp.size() >= ARRAYSIZE(_panel_title)) {
		size_t rmlen = 4 + (tmp.size() - ARRAYSIZE(_panel_title));
		tmp.replace((tmp.size() - rmlen) / 2 , rmlen, L"...");
	}

	wcscpy(_panel_title, tmp.c_str());
}

std::string PluginImpl::CurrentSiteDir(bool with_ending_slash) const
{
	std::string out = _location.ToString(false);
	if (out.empty()) {
		out = ".";
	}
	if (with_ending_slash) {
		if (!out.empty() && out[out.size() - 1] != '/') {
			out+= '/';
		}
	} else if (out.size() > 1 && out[out.size() - 1] == '/') {
		out.resize(out.size() - 1);
	}

	NR_DBG("out='%s'", out.c_str());
	return out;
}

int PluginImpl::GetFindData(PluginPanelItem **pPanelItem, int *pItemsNumber, int OpMode)
{
	NR_DBG("cur_dir: '%ls'", _cur_dir);
	PluginPanelItems ppis;
	try { //_location.path
		auto *ppi = ppis.Add(L"..");
		ppi->FindData.dwFileAttributes = FILE_ATTRIBUTE_DIRECTORY;

		if (_remote == nullptr) {
			std::vector<std::string> children;
			_sites_cfg_location.Enum(children);
			for (const auto &child : children) {
				ppi = ppis.Add(child.c_str());
				ppi->FindData.dwFileAttributes = FILE_ATTRIBUTE_DIRECTORY;
			}

			ppi = ppis.Add(G.GetMsgWide(MCreateSiteConnection));
			ppi->FindData.dwFileAttributes = FILE_ATTRIBUTE_NORMAL;

			SitesConfig sc(_sites_cfg_location);
			const std::vector<std::string> &sites = sc.EnumSites();
			for (const std::string &site : sites) {
				ppi = ppis.Add(site.c_str());
				ppi->FindData.dwFileAttributes = FILE_ATTRIBUTE_NORMAL | FILE_ATTRIBUTE_EXECUTABLE;
			}

		} else {
			OpEnumDirectory(OpMode, _remote, CurrentSiteDir(false), ppis, _wea_state).Do();
			//_remote->DirectoryEnum(CurrentSiteDir(false), il, OpMode);
		}

	} catch (std::exception &e) {
		NR_ERR("cur_dir: %ls, OpMode:%d, ERROR: %s", _cur_dir, OpMode, e.what());
		return FALSE;
	}

	*pPanelItem = ppis.items;
	*pItemsNumber = ppis.count;
	ppis.Detach();
	return TRUE;
}

void PluginImpl::FreeFindData(PluginPanelItem *PanelItem, int ItemsNumber)
{
	PluginPanelItems_Free(PanelItem, ItemsNumber);
}

static bool PreprocessPathTokens(std::vector<std::string> &components)
{
	for (auto i = components.begin(); i != components.end();) {
		if (*i == ".") {
			i = components.erase(i);
		} else if (*i == "..") {
			i = components.erase(i);
			if (i == components.begin()) {
				return false;
			}
			--i;
			i = components.erase(i);
		} else {
			++i;
		}
	}

	return true;
}

int PluginImpl::SetDirectory(const wchar_t *Dir, int OpMode)
{
	NR_DBG("Dir: '%ls', OpMode: 0x%x", Dir, OpMode);
	StackedDir sd;
	StackedDirCapture(sd);

	if (!_remote) {
		if (_sites_cfg_location.Change(Wide2MB(Dir))) {
			UpdatePathInfo();
			return TRUE;
		}
	}

	_allow_remember_location_dir = (OpMode == 0);

	SiteSpecification site_specification;
	if (_location.server_kind == Location::SK_SITE) {
		site_specification = SiteSpecification(StrWide2MB(_standalone_config), _location.server);
	}

	if (*Dir == L'/') {
		DismissRemoteHost();

		do { ++Dir; } while (*Dir == L'/');
		if (*Dir == 0) {
			UpdatePathInfo();
			return TRUE;
		}
	}

	if (*Dir && !SetDirectoryInternal(Dir, OpMode)) {
		StackedDirApply(sd);
		return FALSE;
	}

	UpdatePathInfo();
	if (!IS_SILENT(OpMode) && !_remote && site_specification.IsValid()) {
		G.info.Control(PANEL_ACTIVE, FCTL_UPDATEPANEL, 0, 0);
		G.info.Control(PANEL_ACTIVE, FCTL_REDRAWPANEL, 0, 0);
		G.info.Control(PANEL_ACTIVE, FCTL_CLEARSELECTION, 0, 0);

		PanelRedrawInfo ri = {};

		const std::wstring &site_w = StrMB2Wide(site_specification.site);
		GetItems gi(false);
		for (const auto &item : gi) {
			if ( (item.FindData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) == 0
				&& site_w == item.FindData.lpwszFileName)
			{
				ri.CurrentItem = ri.TopPanelItem = &item - &gi[0];
				break;
			}
		}

		G.info.Control(PANEL_ACTIVE, FCTL_REDRAWPANEL, 0, (LONG_PTR)&ri);

	}
	return TRUE;
}

int PluginImpl::SetDirectoryInternal(const wchar_t *Dir, int OpMode)
{
	if (!_remote) {
		std::string dir_mb = Wide2MB(Dir);
		if (dir_mb.empty() || dir_mb[0] == '/'
		 || (dir_mb[0] == '~' && (dir_mb.size() == 1 || dir_mb[1] == '/'))) {
			NR_ERR("wrong path for non-connected state; Dir='%ls', OpMode=0x%x", Dir, OpMode);
			return FALSE;
		}

		if (dir_mb[0] != '<') {
			size_t p = dir_mb.find('/');
			if (p != std::string::npos) {
				dir_mb.insert(p, 1, '>');
			} else {
				dir_mb+= '>';
			}
			dir_mb.insert(0, _sites_cfg_location.TranslateToPath(true));
			dir_mb.insert(0, 1, '<');
		}

		Location new_location;
		if (!new_location.FromString(StrWide2MB(_standalone_config), dir_mb)) {
			NR_ERR("Dir='%ls', OpMode=0x%x; parse path failed", Dir, OpMode);
			return FALSE;
		}

		if (new_location.server_kind == Location::SK_URL && new_location.url.password.empty()
		 && new_location.server_kind == _location.server_kind && new_location.server == _location.server) {
			new_location.url.password = _location.url.password;
		}
		_location = new_location;

		if (!PreprocessPathTokens(_location.path.components)) {
			NR_ERR("Dir='%ls', OpMode=0x%x; inconstistent path", Dir, OpMode);
			return FALSE;
		}

		if (g_conn_pool) {
			_remote = g_conn_pool->Get(CurrentConnectionPoolId());
		}

		if (!_remote) {
			_remote = OpConnect(0, StrWide2MB(_standalone_config), _location).Do();
			if (!_remote) {
				NR_ERR("Dir='%ls', OpMode=0x%x; connect failed", Dir, OpMode);
				return FALSE;
			}
		}

		_wea_state = std::make_shared<WhatOnErrorState>();
		return ValidateLocationDirectory(OpMode) ? TRUE : FALSE;
	}


	std::vector<std::string> components;
	StrExplode(components, Wide2MB(Dir), "/");

	auto saved_path = _location.path;

	if (!components.empty() && components[0] == "~") {
		_location.path.ResetToHome();
		_location.path.components.insert(_location.path.components.end(), components.begin() + 1, components.end());

	} else if (*Dir != L'/') {
		if (components.empty()) {
			return TRUE;
		}
		_location.path.components.insert(_location.path.components.end(), components.begin(), components.end());

	} else {
		_location.path.components = components;
		_location.path.absolute = true;
	}

	if (!PreprocessPathTokens(_location.path.components)) {
		NR_ERR("close connection due to: '%ls'", Dir);
		DismissRemoteHost();
		return TRUE;
	}

	if (ValidateLocationDirectory(OpMode)) {
		return TRUE;
	}

	_location.path = saved_path;
	return FALSE;
}

bool PluginImpl::ValidateLocationDirectory(int OpMode)
{
	const std::string &dir = _location.ToString(false);
	NR_DBG("dir: '%s'", dir.c_str());
	if (dir.empty()) {
		return true;
	}

	std::string final_dir;
	if (!OpCheckDirectory(OpMode, _remote, dir, _wea_state).Do(final_dir)) {
		NR_DBG("failed");
		return false;
	}

	if (dir != final_dir) {
		NR_DBG("final_dir: '%s'", final_dir.c_str());
		_location.PathFromString(final_dir);
	}

	return true;
}

void PluginImpl::GetOpenPluginInfo(struct OpenPluginInfo *Info)
{
//	fprintf(stderr, "NetRocks::GetOpenPluginInfo: '%ls' \n", &_cur_dir[0]);
//	snprintf(_panel_title, ARRAYSIZE(_panel_title),
//	          " Inside: %ls@%s ", _dir.c_str(), _name.c_str());
	Info->Flags = OPIF_SHOWPRESERVECASE | OPIF_USEHIGHLIGHTING;
	if (_remote) {
		IHost::Identity identity;
		_remote->GetIdentity(identity);
		const auto *pi = ProtocolInfoLookup(identity.protocol.c_str());
		if (pi && pi->inaccurate_timestamps) {
			Info->Flags|= OPIF_COMPAREFATTIME;
		}
	}
	else {
		Info->Flags|= OPIF_HL_MARKERS_NOSHOW; // for site connections list always don't show markers
		// for site connections list don't use current far2l panel modes
		//  - only name column(s) without dependence on panel Align file extensions
		static struct PanelMode PanelModesArray[10] = {
			{ .ColumnTypes = L"N", .ColumnWidths = L"0" },
			{ .ColumnTypes = L"N,N,N", .ColumnWidths = L"0,0,0" },
			{ .ColumnTypes = L"N,N", .ColumnWidths = L"0,0" },
			{ .ColumnTypes = L"N", .ColumnWidths = L"0" }, { .ColumnTypes = L"N", .ColumnWidths = L"0" },
			{ .ColumnTypes = L"N", .ColumnWidths = L"0" }, { .ColumnTypes = L"N", .ColumnWidths = L"0" },
			{ .ColumnTypes = L"N", .ColumnWidths = L"0" }, { .ColumnTypes = L"N", .ColumnWidths = L"0" },
			{ .ColumnTypes = L"N", .ColumnWidths = L"0" },
		};
		Info->PanelModesArray = PanelModesArray;
		Info->PanelModesNumber = ARRAYSIZE(PanelModesArray);
	}
	Info->HostFile = _standalone_config.empty() ? NULL : _standalone_config.c_str();
	Info->CurDir = _cur_dir;
	Info->Format = _format;
	Info->PanelTitle = _panel_title;
	Info->CurURL = _cur_URL;
}


BackgroundTaskStatus PluginImpl::StartXfer(int op_mode, std::shared_ptr<IHost> &base_host, const std::string &base_dir,
		std::shared_ptr<IHost> &dst_host, const std::string &dst_dir, struct PluginPanelItem *items,
		int items_count, XferKind kind, XferDirection direction)
{
	BackgroundTaskStatus out = BTS_ABORTED;
	try {
		std::shared_ptr<IBackgroundTask> task = std::make_shared<OpXfer>(op_mode, base_host,
				base_dir, dst_host, dst_dir, items, items_count, kind, direction);

		// task->Show();
		out = task->GetStatus();
		if (out == BTS_ACTIVE || out == BTS_PAUSED) {
			AddBackgroundTask(task);
		}
	} catch (std::exception &ex) {
		NR_ERR("%s", ex.what());
	}

	return out;
}

BOOL PluginImpl::SitesXfer(const char *dir, struct PluginPanelItem *items, int items_count, bool mv, bool imp)
{
	if (!ConfirmSitesDisposition(imp ? ConfirmSitesDisposition::W_IMPORT
				: ConfirmSitesDisposition::W_EXPORT, mv).Ask()) {
		return TRUE;
	}

	for (int i = 0; i < items_count; ++i) {
		if (!FILENAME_ENUMERABLE(items[i].FindData.lpwszFileName)) {
			continue;
		}

		bool its_dir = ((items[i].FindData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) != 0);

		std::string item_name = Wide2MB(items[i].FindData.lpwszFileName);
		if (imp) {
			if (!_sites_cfg_location.Import(dir, item_name, its_dir, mv)) {
				return FALSE;
			}

		} else if ( its_dir || (items[i].FindData.dwFileAttributes & FILE_ATTRIBUTE_EXECUTABLE) != 0) {
			if (!_sites_cfg_location.Export(dir, item_name, its_dir, mv)) {
				return FALSE;
			}
		}
	}

	return TRUE;
}

int PluginImpl::GetFiles(struct PluginPanelItem *PanelItem, int ItemsNumber, int Move, const wchar_t *DestPath, int OpMode)
{
	NR_DBG("_dir='%ls' DestPath='%ls' ItemsNumber=%d OpMode=%d", _cur_dir, DestPath, ItemsNumber, OpMode);
	if (ItemsNumber <= 0) {
		return FALSE;
	}

	std::string dst_dir;
	if (DestPath)
		Wide2MB(DestPath, dst_dir);

	if (!_remote) {
		return (OpMode == 0) ? SitesXfer(dst_dir.c_str(), PanelItem, ItemsNumber, Move != 0, false) : FALSE;
	}

	switch (StartXfer(OpMode, _remote, CurrentSiteDir(true), _local,
		dst_dir, PanelItem, ItemsNumber, Move ? XK_MOVE : XK_COPY, XD_DOWNLOAD)) {

		case BTS_ACTIVE: case BTS_PAUSED:
			_remote = _remote->Clone();
			_local = _local->Clone();

		case BTS_COMPLETE:
			return TRUE;

		case BTS_ABORTED:
		default:
			return FALSE;
	}

}

int PluginImpl::PutFiles(struct PluginPanelItem *PanelItem, int ItemsNumber, int Move, const wchar_t *SrcPath, int OpMode)
{
	const std::string &site_dir = CurrentSiteDir(true);
	NR_DBG("_dir='%ls' SrcPath='%ls' site_dir='%s' ItemsNumber=%d OpMode=%d", _cur_dir, SrcPath, site_dir.c_str(), ItemsNumber, OpMode);
	if (ItemsNumber <= 0)
		return FALSE;

//	std::string src_dir;
//	if (SrcPath) {
//		Wide2MB(SrcPath, src_dir);
//	} else {
	char dst_dir[PATH_MAX] = {};
	sdc_getcwd(dst_dir, PATH_MAX-2);
	size_t l = strlen(dst_dir);
	if (l == 0) {
		strcpy(dst_dir, "./");

	} else if (dst_dir[l - 1] != '/') {
		dst_dir[l] = '/';
		dst_dir[l + 1] = 0;
	}

	if (!_remote) {
		return (OpMode == 0) ? SitesXfer(dst_dir, PanelItem, ItemsNumber, Move != 0, true) : FALSE;
	}

	switch (StartXfer(OpMode, _local, dst_dir, _remote, CurrentSiteDir(true),
		PanelItem, ItemsNumber, Move ? XK_MOVE : XK_COPY, XD_UPLOAD)) {

		case BTS_ACTIVE: case BTS_PAUSED:
			_remote = _remote->Clone();
			_local = _local->Clone();

		case BTS_COMPLETE:
			return TRUE;

		case BTS_ABORTED:
		default:
			return FALSE;
	}
}

int PluginImpl::GetLinkTarget(struct PluginPanelItem *PanelItem, wchar_t *Target, size_t TargetSize, int OpMode)
{
	if (!_remote || !PanelItem) {
		return 0;
    }
   	std::wstring result;
	if (!OpGetLinkTarget(OpMode, _remote, CurrentSiteDir(true), PanelItem).Do(result)) {
		return 0;
	}
	wcsncpy(Target, result.c_str(), TargetSize);
	return result.size() + 1;
}

int PluginImpl::DeleteFiles(struct PluginPanelItem *PanelItem, int ItemsNumber, int OpMode)
{
	NR_DBG("_dir='%ls' ItemsNumber=%d", _cur_dir, ItemsNumber);
	if (ItemsNumber <= 0)
		return FALSE;

	if (!_remote) {
		if (!ConfirmSitesDisposition(ConfirmSitesDisposition::W_REMOVE, false).Ask())
			return FALSE;

		SitesConfig sc(_sites_cfg_location);
		for (int i = 0; i < ItemsNumber; ++i) {
			const std::string &display_name = Wide2MB(PanelItem[i].FindData.lpwszFileName);

			if (PanelItem[i].FindData.dwFileAttributes&FILE_ATTRIBUTE_DIRECTORY) {
				NR_DBG("Removing sites directory: %s", display_name.c_str());
				_sites_cfg_location.Remove(display_name);
			} else {
				NR_DBG("Removing site: %s", display_name.c_str());
				sc.RemoveSite(display_name);
			}
		}
		return TRUE;
	}

	return OpRemove(OpMode, _remote, CurrentSiteDir(true), PanelItem, ItemsNumber).Do() ? TRUE : FALSE;
}

int PluginImpl::MakeDirectory(const wchar_t **Name, int OpMode)
{
	NR_DBG("Name='%ls', OpMode=0x%x", Name ? *Name : L"NULL", OpMode);

	std::string dir_name;
	if (Name && *Name) {
		Wide2MB(*Name, dir_name);
	}

	if (!IS_SILENT(OpMode)) {
		dir_name = ConfirmMakeDir(dir_name).Ask();
	}

	if (dir_name.empty()) {
		NR_DBG("cancel");
		return -1;
	}

	if (!_remote) {
		if (!_sites_cfg_location.Make(dir_name)) {
			return 0;
		}

	} else {
		OpMakeDirectory op(OpMode, _remote, CurrentSiteDir(true), dir_name);
		if (!op.Do()) {
			return 0;
		}

		dir_name = op.DirName();
	}

	if (Name && !IS_SILENT(OpMode)) {
		wcsncpy(_mk_dir, StrMB2Wide(dir_name).c_str(), ARRAYSIZE(_mk_dir) - 1);
		_mk_dir[ARRAYSIZE(_mk_dir) - 1] = 0;
		*Name = _mk_dir;
	}

	return 1;
}

static bool NoControls(unsigned int ControlState)
{
	return (ControlState & (PKF_CONTROL | PKF_ALT | PKF_SHIFT)) == 0;
}


int PluginImpl::ProcessKey(int Key, unsigned int ControlState)
{
//	fprintf(stderr, "NetRocks::ProcessKey(0x%x, 0x%x)\n", Key, ControlState);
	if (Key == VK_RETURN && ControlState == 0 && _remote
			&& G.GetGlobalConfigBool("EnterExecRemotely", true)) {
		return ByKey_TryExecuteSelected() ? TRUE : FALSE;
	}

	if ((Key == VK_RETURN || (Key == VK_NEXT && ControlState == PKF_CONTROL)) && !_remote) {
		return ByKey_TryEnterSelectedSite() ? TRUE : FALSE;
	}

	if ((Key == VK_F5 || Key == VK_F6) && NoControls(ControlState)) {
		return ByKey_TryCrossload(Key == VK_F6) ? TRUE : FALSE;
	}

	if (Key == VK_F4 && !_remote
	&& (ControlState == 0 || ControlState == PKF_SHIFT)) {
		ByKey_EditSiteConnection(ControlState == PKF_SHIFT);
		return TRUE;
	}

	if (Key == VK_F3 && !_remote) {
		return TRUE;
	}

	if (Key == 'A' && ControlState == PKF_CONTROL) {
		if (_remote) {
			ByKey_EditAttributesSelected();
		}
		return TRUE;
	}

	if (Key == 'R' && ControlState == PKF_CONTROL && !_remote && g_conn_pool) {
		g_conn_pool->PurgeAll();
	}

/*
	if (Key == VK_F7) {
		MakeDirectory();
		return TRUE;
	}
*/
	return FALSE;
}


void PluginImpl::ByKey_EditSiteConnection(bool create_new)
{
	if (g_conn_pool) {
		g_conn_pool->PurgeAll();
	}

	std::string site;

	if (!create_new) {
		GetFocusedItem gfi(this);
		if (gfi.IsValid()) {
			if ( (gfi->FindData.dwFileAttributes & FILE_ATTRIBUTE_EXECUTABLE) == 0) {
				// dont allow editing of directories and <Create site connection>
				return;
			}
			Wide2MB(gfi->FindData.lpwszFileName, site);
		}
	}

	SiteConnectionEditor sce(_sites_cfg_location, site);
	const bool connect_now = sce.Edit();
	G.info.Control(PANEL_ACTIVE, FCTL_UPDATEPANEL, 0, 0);
	if (connect_now) {
		SetDirectoryInternal(StrMB2Wide(sce.DisplayName()).c_str(), 0);
	}

	UpdatePathInfo();
	G.info.Control(PANEL_ACTIVE, FCTL_UPDATEPANEL, 0, 0);
}

bool PluginImpl::ByKey_TryCrossload(bool mv)
{
	HANDLE plugin = INVALID_HANDLE_VALUE;
	G.info.Control(PANEL_ACTIVE, FCTL_GETPANELPLUGINHANDLE, 0, (LONG_PTR)(void *)&plugin);
	if (plugin != (void *)this)
		return false;

	G.info.Control(PANEL_PASSIVE, FCTL_GETPANELPLUGINHANDLE, 0, (LONG_PTR)(void *)&plugin);
	if (plugin == INVALID_HANDLE_VALUE)
		return false;

	std::shared_ptr<IHost> dst_remote;
	std::string dst_site_dir;
	SitesConfigLocation sites_cfg_location;
	if (!g_all_netrocks.GetRemoteOf(plugin, dst_remote, dst_site_dir, sites_cfg_location))
		return false;

	GetItems gi(true);

	if (gi.empty()) {
		return true;
	}

	if ( _remote) {
		if (!dst_remote) { // TODO: Show error that need connection
			NR_ERR("mv=%d; tried to upload to unconnected NetRocks instance", mv);
			return true;
		}
		auto state = StartXfer(0, _remote, CurrentSiteDir(true), dst_remote, dst_site_dir,
			&gi[0], (int)gi.size(), mv ? XK_MOVE : XK_COPY, XD_CROSSLOAD);
		if (state == BTS_ACTIVE || state == BTS_PAUSED) {
			_remote = _remote->Clone();
			g_all_netrocks.CloneRemoteOf(plugin);
		}

	} else {
		if (dst_remote) { // TODO: Show error that need opened sites list
			NR_ERR("mv=%d; tried to transfer site to connected NetRocks instance", mv);
			return true;
		}

		if (_sites_cfg_location != sites_cfg_location) {
			if (!ConfirmSitesDisposition(ConfirmSitesDisposition::W_RELOCATE, mv).Ask()) {
				return true;
			}

			for (const auto &item : gi) if ( (item.FindData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) != 0
				&& wcscmp(item.FindData.lpwszFileName, L"..") != 0)
			{
				_sites_cfg_location.Transfer(sites_cfg_location, Wide2MB(item.FindData.lpwszFileName), mv);

			} else if ( (item.FindData.dwFileAttributes & FILE_ATTRIBUTE_EXECUTABLE) != 0) {
				SitesConfig src(_sites_cfg_location), dst(sites_cfg_location);
				src.Transfer(dst, Wide2MB(item.FindData.lpwszFileName), mv);
			}
		}
	}

	G.info.Control(PANEL_ACTIVE, FCTL_UPDATEPANEL, 0, 0);
	G.info.Control(PANEL_ACTIVE, FCTL_REDRAWPANEL, 0, 0);

	G.info.Control(PANEL_PASSIVE, FCTL_UPDATEPANEL, 0, 0);
	G.info.Control(PANEL_PASSIVE, FCTL_REDRAWPANEL, 0, 0);

	return true;
}

bool PluginImpl::ByKey_TryExecuteSelected()
{
	GetFocusedItem gfi(this);
	if ( (gfi->FindData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) != 0
	 || (gfi->FindData.dwFileAttributes & FILE_ATTRIBUTE_EXECUTABLE) == 0) {
		return false;
	}

	std::wstring cmd = L"./";
	cmd+= gfi->FindData.lpwszFileName;
	QuoteCmdArgIfNeed(cmd);

	G.info.Control(this, FCTL_SETCMDLINE, 0, (LONG_PTR)cmd.c_str());

	if (!ProcessEventCommand(cmd.c_str())) {
		G.info.Control(this, FCTL_SETCMDLINE, 0, (LONG_PTR)L"");
		return false;
	}

	return true;
}

bool PluginImpl::ByKey_TryEnterSelectedSite()
{
	GetFocusedItem gfi(this);
	if ( (gfi->FindData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) != 0) {
		return false;
	}

	if ( (gfi->FindData.dwFileAttributes & FILE_ATTRIBUTE_EXECUTABLE) == 0) {
		//&& wcscmp(gfi->FindData.lpwszFileName, G.GetMsgWide(MCreateSiteConnection)) == 0) {
		ByKey_EditSiteConnection(true);
		return true;
	}

	StackedDir sd;
	StackedDirCapture(sd);
	bool out = SetDirectoryInternal(gfi->FindData.lpwszFileName, 0);
	if (!out) {
		StackedDirApply(sd);
	}
	UpdatePathInfo();
	if (out) {
		G.info.Control(PANEL_ACTIVE, FCTL_UPDATEPANEL, 0, 0);
		G.info.Control(PANEL_ACTIVE, FCTL_CLEARSELECTION, 0, 0);
		PanelRedrawInfo ri = {};
		G.info.Control(PANEL_ACTIVE, FCTL_REDRAWPANEL, 0, (LONG_PTR)&ri);
	}

	return true;
}

void PluginImpl::ByKey_EditAttributesSelected()
{
	GetItems gi(true);

	if (!gi.empty()) try {
		OpChangeMode(_remote, CurrentSiteDir(true), &gi[0], (int)gi.size()).Do();
		G.info.Control(PANEL_ACTIVE, FCTL_UPDATEPANEL, 0, 0);

	} catch (std::exception &ex) {
		NR_ERR("%s", ex.what());
	}
}


static std::wstring GetCommandArgument(const wchar_t *cmd)
{
	std::wstring out;
	const wchar_t *arg = wcschr(cmd, ' ');
	if (arg) {
		while (*arg == ' ') {
			++arg;
		}
		out = arg;
		while (!out.empty() && out[out.size() - 1] == ' ') {
			out.resize(out.size() - 1);
		}
	}

	return out;
}

static void FakeExec()
{
	G.info.FSF->Execute(L"true", EF_NOCMDPRINT);
}

void PluginImpl::StackedDirCapture(StackedDir &sd)
{
	sd.remote = _remote;
	sd.location = _location;
	sd.sites_cfg_location = _sites_cfg_location;
	sd.standalone_config = _standalone_config;
}

void PluginImpl::StackedDirApply(StackedDir &sd)
{
	if (sd.remote.get() != _remote.get()) {
		if (sd.remote && sd.remote.use_count() > 1) {
			// original connection could be used for background download etc
			_remote = sd.remote->Clone();
		} else {
			_remote = sd.remote;
		}
	}
	_location = sd.location;
	_sites_cfg_location = sd.sites_cfg_location;
	_standalone_config = sd.standalone_config;
	UpdatePathInfo();
}

int PluginImpl::ProcessEventCommand(const wchar_t *cmd)
{
	if (wcsstr(cmd, L"exit ") == cmd || wcscmp(cmd, L"exit") == 0) {
		if (GetCommandArgument(cmd) == L"far") {
			return FALSE;
		}
		DismissRemoteHost();
		UpdatePathInfo();

	} else if (wcsstr(cmd, L"pushd ") == cmd || wcscmp(cmd, L"pushd") == 0) {
		StackedDir sd;
		StackedDirCapture(sd);

		const std::wstring &dir = GetCommandArgument(cmd);
		if ((!_remote && (dir.empty() || dir == L".")) || SetDirectoryInternal(dir.empty() ? L"~" : dir.c_str(), 0)) {
			_dir_stack.emplace_back(sd);
			FakeExec();
		}

	} else if (wcscmp(cmd, L"popd") == 0) {
		if (!_dir_stack.empty()) {
			StackedDirApply(_dir_stack.back());
			_dir_stack.pop_back();
			UpdatePathInfo();
			FakeExec();
		}

	} else if (wcsstr(cmd, L"cd ") == cmd || wcscmp(cmd, L"cd") == 0) {
		const std::wstring &dir = GetCommandArgument(cmd);
		if (SetDirectoryInternal(dir.empty() ? L"~" : dir.c_str(), 0)) {
			FakeExec();
		}

	} else if (_remote) try {
		OpExecute(0, _remote, CurrentSiteDir(false), Wide2MB(cmd)).Do();

	} catch (ProtocolUnsupportedError &) {
		const wchar_t *msg[] = { G.GetMsgWide(MCommandsNotSupportedTitle), G.GetMsgWide(MCommandsNotSupportedText), G.GetMsgWide(MOK)};
		G.info.Message(G.info.ModuleNumber, FMSG_WARNING, nullptr, msg, ARRAYSIZE(msg), 1);

	} catch (std::exception &ex) {
		NR_ERR("OpExecute::Do: %s", ex.what());

		const std::wstring &tmp_what = MB2Wide(ex.what());
		const wchar_t *msg[] = { G.GetMsgWide(MError), tmp_what.c_str(), G.GetMsgWide(MOK)};
		G.info.Message(G.info.ModuleNumber, FMSG_WARNING, nullptr, msg, ARRAYSIZE(msg), 1);

	} else {
		return FALSE;
	}

	UpdatePathInfo();

	G.info.Control(this, FCTL_SETCMDLINE, 0, (LONG_PTR)L"");
	G.info.Control(PANEL_ACTIVE, FCTL_UPDATEPANEL, 0, 0);
	G.info.Control(PANEL_ACTIVE, FCTL_REDRAWPANEL, 0, 0);
	G.info.Control(PANEL_PASSIVE, FCTL_UPDATEPANEL, 0, 0);
	G.info.Control(PANEL_PASSIVE, FCTL_REDRAWPANEL, 0, 0);

	return TRUE;
}

void PluginImpl::DismissRemoteHost()
{
	if (!g_conn_pool)
		g_conn_pool.reset(new ConnectionsPool);

	g_conn_pool->Put(CurrentConnectionPoolId(), _remote);

	if (_allow_remember_location_dir &&
		_location.server_kind == Location::SK_SITE
		&& G.GetGlobalConfigBool("RememberDirectory", false))
	{
		SiteSpecification site_specification(StrWide2MB(_standalone_config), _location.server);
		SitesConfig sc(site_specification.sites_cfg_location);
		sc.SetDirectory(site_specification.site, _location.ToString(false));
	}
	_remote.reset();
}

std::string PluginImpl::CurrentConnectionPoolId()
{
	std::string out = _location.server;
	if (!_standalone_config.empty()) {
		out+= '@';
		out+= StrWide2MB(_standalone_config);
	}
	return out;
}

void PluginImpl::sOnExiting()
{
	g_conn_pool.reset();
}

void PluginImpl::sOnGlobalSettingsChanged()
{
	if (g_conn_pool)
		g_conn_pool->OnGlobalSettingsChanged();
}
