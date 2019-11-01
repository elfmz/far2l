#include <vector>
#include <mutex>
#include <wchar.h>
#include "Globals.h"
#include <utils.h>
#include <TailedStruct.hpp>
#include "SitesConfig.h"
#include "PluginImpl.h"
#include "PluginPanelItems.h"
#include "PooledStrings.h"
#include "ConnectionsPool.h"
#include "GetSelectedItems.h"
#include "Host/HostLocal.h"
#include "UI/Settings/SiteConnectionEditor.h"
#include "UI/Activities/Confirm.h"
#include "Op/OpConnect.h"
#include "Op/OpGetMode.h"
#include "Op/OpXfer.h"
#include "Op/OpRemove.h"
#include "Op/OpMakeDirectory.h"
#include "Op/OpEnumDirectory.h"
#include "Op/OpExecute.h"
#include "Op/OpChangeMode.h"

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
			fprintf(stderr, "AllNetRocks - dup entry: %p\n", it);
			abort();
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

	bool GetRemoteOf(void *handle, std::shared_ptr<IHost> &remote, std::string &site_dir)
	{
		std::lock_guard<std::mutex> locker(_mutex);
		auto i = _all.find(handle);
		if (i == _all.end())
			return false;

		remote = i->second->_remote;
		site_dir = i->second->CurrentSiteDir(true);
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

PluginImpl::PluginImpl(const wchar_t *path)
{
	_cur_dir[0] = _panel_title[0] = 0;
	_local = std::make_shared<HostLocal>();

	if (path && *path) {
		if (!_location.FromString(Wide2MB(path))) {
			throw std::runtime_error(G.GetMsgMB(MWrongPath));
		}

		_remote = OpConnect(0, _location).Do();
		if (!_remote) {
			throw std::runtime_error(G.GetMsgMB(MCouldNotConnect));
		}

		_wea_state = std::make_shared<WhatOnErrorState>();
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
		wcsncpy(_cur_dir, StrMB2Wide(_location.ToString(true)).c_str(), ARRAYSIZE(_cur_dir) - 1 );
//		tmp = _remote->SiteInfo();
//		tmp+= '/';
		tmp+= _cur_dir;
	} else {
		_cur_dir[0] = 0;
		tmp = L"NetRocks connections list";
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

	fprintf(stderr, "CurrentSiteDir: out='%s'\n", out.c_str());
	return out;
}

int PluginImpl::GetFindData(PluginPanelItem **pPanelItem, int *pItemsNumber, int OpMode)
{
	fprintf(stderr, "NetRocks::GetFindData '%ls'\n", _cur_dir);
	PluginPanelItems ppis;
	try { //_location.path
		auto *ppi = ppis.Add(L"..");
		ppi->FindData.dwFileAttributes = FILE_ATTRIBUTE_DIRECTORY;

		if (_remote == nullptr) {
			SitesConfig sc;
			auto *ppi = ppis.Add(G.GetMsgWide(MCreateSiteConnection));
			ppi->FindData.dwFileAttributes = FILE_ATTRIBUTE_DIRECTORY;

			const std::vector<std::string> &sites = sc.EnumSites();
			for (const std::string &site : sites) {
				ppi = ppis.Add(site.c_str());
				ppi->FindData.dwFileAttributes = FILE_ATTRIBUTE_DIRECTORY;
			}

		} else {
			OpEnumDirectory(OpMode, _remote, CurrentSiteDir(false), ppis, _wea_state).Do();
			//_remote->DirectoryEnum(CurrentSiteDir(false), il, OpMode);
		}

	} catch (std::exception &e) {
		fprintf(stderr, "NetRocks::GetFindData('%ls', %d) ERROR: %s\n", _cur_dir, OpMode, e.what());
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
	fprintf(stderr, "PluginImpl::SetDirectory('%ls', %d)\n", Dir, OpMode);
	if (!_remote && wcscmp(Dir, G.GetMsgWide(MCreateSiteConnection)) == 0) {
		ByKey_EditSiteConnection(true);
		UpdatePathInfo();
		return FALSE;
	}

	StackedDir sd;
	StackedDirCapture(sd);

	if (*Dir == L'/') {
		DismissRemoteHost();

		do { ++Dir; } while (*Dir == L'/');
		if (*Dir == 0) {
			return TRUE;
		}
	}

	if (*Dir && !SetDirectoryInternal(Dir, OpMode)) {
		StackedDirApply(sd);
		return FALSE;
	}

	UpdatePathInfo();
	return TRUE;
}

int PluginImpl::SetDirectoryInternal(const wchar_t *Dir, int OpMode)
{
	if (!_remote) {
		std::string dir_mb = Wide2MB(Dir);
		if (dir_mb.empty() || dir_mb[0] == '/'
		 || (dir_mb[0] == '~' && (dir_mb.size() == 1 || dir_mb[1] == '/'))) {
			fprintf(stderr, "SetDirectoryInternal('%ls', 0x%x): wrong path for non-connected state\n", Dir, OpMode);
			return FALSE;
		}

		if (dir_mb[0] != '<') {
			dir_mb.insert(0, 1, '<');
			size_t p = dir_mb.find('/');
			if (p != std::string::npos) {
				dir_mb.insert(p, 1, '>');
			} else {
				dir_mb+= '>';
			}
		}

		Location new_location;
		if (!new_location.FromString(dir_mb)) {
			fprintf(stderr, "SetDirectoryInternal('%ls', 0x%x): parse path failed\n", Dir, OpMode);
			return FALSE;
		}

		if (new_location.server_kind == Location::SK_URL && new_location.url.password.empty()
		 && new_location.server_kind == _location.server_kind && new_location.server == _location.server) {
			new_location.url.password = _location.url.password;
		}
		_location = new_location;

		if (!PreprocessPathTokens(_location.path.components)) {
			fprintf(stderr, "SetDirectoryInternal('%ls', 0x%x): inconstistent path\n", Dir, OpMode);
			return FALSE;
		}

		if (g_conn_pool) {
			_remote = g_conn_pool->Get(_location.server);
		}

		if (!_remote) {
			_remote = OpConnect(0, _location).Do();
			if (!_remote) {
				fprintf(stderr, "SetDirectoryInternal('%ls', 0x%x): connect failed\n", Dir, OpMode);
				return FALSE;
			}
		}

		_wea_state = std::make_shared<WhatOnErrorState>();
		return TRUE;
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
		fprintf(stderr, "NetRocks::SetDirectoryInternal - close connection due to: '%ls'\n", Dir);
		DismissRemoteHost();
		return TRUE;
	}

	const std::string &dir = _location.ToString(false);
	fprintf(stderr, "NetRocks::SetDirectoryInternal - dir: '%s'\n", dir.c_str());
	if (!dir.empty()) {
		mode_t mode = 0;
		if (!OpGetMode(OpMode, _remote, dir, _wea_state).Do(mode)) {
			fprintf(stderr, "NetRocks::SetDirectoryInternal - can't get mode: '%s'\n", dir.c_str());
			_location.path = saved_path;
			return FALSE; 
		}

		if (!S_ISDIR(mode)) {
			fprintf(stderr, "NetRocks::SetDirectoryInternal - not dir mode=0x%x: '%s'\n", mode, dir.c_str());
			_location.path = saved_path;
			return FALSE;
		}
	}

	fprintf(stderr, "NetRocks::SetDirectoryInternal('%ls', %d) OK: '%s'\n", Dir, OpMode, dir.c_str());
	return TRUE;
}

void PluginImpl::GetOpenPluginInfo(struct OpenPluginInfo *Info)
{
//	fprintf(stderr, "NetRocks::GetOpenPluginInfo: '%ls' \n", &_cur_dir[0]);
//	snprintf(_panel_title, ARRAYSIZE(_panel_title),
//	          " Inside: %ls@%s ", _dir.c_str(), _name.c_str());

	Info->Flags = OPIF_SHOWPRESERVECASE | OPIF_USEHIGHLIGHTING;
	Info->HostFile = NULL;
	Info->CurDir = _cur_dir;
	Info->PanelTitle = _panel_title;
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
		fprintf(stderr, "NetRocks::StartXfer: %s\n", ex.what());
	}

	return out;
}

int PluginImpl::GetFiles(struct PluginPanelItem *PanelItem, int ItemsNumber, int Move, const wchar_t *DestPath, int OpMode)
{
	fprintf(stderr, "NetRocks::GetFiles: _dir='%ls' DestPath='%ls' ItemsNumber=%d OpMode=%d\n", _cur_dir, DestPath, ItemsNumber, OpMode);
	if (ItemsNumber <= 0)
		return FALSE;

	if (!_remote) {
		return FALSE;
	}

	std::string dst_dir;
	if (DestPath)
		Wide2MB(DestPath, dst_dir);

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
	fprintf(stderr, "NetRocks::GetFiles: _dir='%ls' SrcPath='%ls' site_dir='%s' ItemsNumber=%d OpMode=%d\n", _cur_dir, SrcPath, site_dir.c_str(), ItemsNumber, OpMode);
	if (ItemsNumber <= 0)
		return FALSE;

	if (!_remote) {
		return FALSE;
	}

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

int PluginImpl::DeleteFiles(struct PluginPanelItem *PanelItem, int ItemsNumber, int OpMode)
{
	fprintf(stderr, "NetRocks::DeleteFiles: _dir='%ls' ItemsNumber=%d\n", _cur_dir, ItemsNumber);
	if (ItemsNumber <= 0)
		return FALSE;

	if (!_remote) {
		if (!ConfirmRemoveSites().Ask())
			return FALSE;

		SitesConfig sc;
		for (int i = 0; i < ItemsNumber; ++i) {
			const std::string &display_name = Wide2MB(PanelItem[i].FindData.lpwszFileName);
			fprintf(stderr, "removing %s\n", display_name.c_str());
			sc.RemoveSite(display_name);
		}
		return TRUE;
	}

	return OpRemove(OpMode, _remote, CurrentSiteDir(true), PanelItem, ItemsNumber).Do() ? TRUE : FALSE;
}

int PluginImpl::MakeDirectory(const wchar_t **Name, int OpMode)
{
	fprintf(stderr, "NetRocks::MakeDirectory('%ls', 0x%x)\n", Name ? *Name : L"NULL", OpMode);
	if (!_remote) {
		return FALSE;
	}

	std::string tmp;
	if (Name && *Name) {
		Wide2MB(*Name, tmp);
	}
	OpMakeDirectory op(OpMode, _remote, CurrentSiteDir(true), Name ? tmp.c_str() : nullptr);
	if (!op.Do()) {
		return FALSE;
	}
	if (Name && !IS_SILENT(OpMode)) {
		tmp = op.DirName();
		const std::wstring name_w = StrMB2Wide(tmp);
		wcsncpy(_mk_dir, name_w.c_str(), ARRAYSIZE(_mk_dir) - 1);
		_mk_dir[ARRAYSIZE(_mk_dir) - 1] = 0;
		*Name = _mk_dir;
	}

	return TRUE;;
}

int PluginImpl::ProcessKey(int Key, unsigned int ControlState)
{
//	fprintf(stderr, "NetRocks::ProcessKey(0x%x, 0x%x)\n", Key, ControlState);
	if (Key == VK_RETURN && ControlState == 0 && _remote
	 && G.GetGlobalConfigBool("EnterExecRemotely", true)) {
		return ByKey_TryExecuteSelected() ? TRUE : FALSE;
	}

	if ((Key == VK_F5 || Key == VK_F6) && _remote) {
		return ByKey_TryCrossload(Key==VK_F6) ? TRUE : FALSE;
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
		intptr_t size = G.info.Control(this, FCTL_GETSELECTEDPANELITEM, 0, 0);
		if (size >= (intptr_t)sizeof(PluginPanelItem)) {
			TailedStruct<PluginPanelItem> ppi(size + 0x20 - sizeof(PluginPanelItem));
			G.info.Control(this, FCTL_GETSELECTEDPANELITEM, 0, (LONG_PTR)(void *)ppi.ptr());
			if (wcscmp(ppi->FindData.lpwszFileName, G.GetMsgWide(MCreateSiteConnection)) != 0) {
				Wide2MB(ppi->FindData.lpwszFileName, site);
			}
		}
	}

	SiteConnectionEditor sce(site);
	const bool connect_now = sce.Edit();
	G.info.Control(PANEL_ACTIVE, FCTL_UPDATEPANEL, 0, 0);
	if (connect_now) {
		SetDirectoryInternal(StrMB2Wide(sce.DisplayName()).c_str(), 0);
		UpdatePathInfo();
		G.info.Control(PANEL_ACTIVE, FCTL_UPDATEPANEL, 0, 0);
	}
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
	if (!g_all_netrocks.GetRemoteOf(plugin, dst_remote, dst_site_dir))
		return false;

	if (!dst_remote) {
		;// TODO: Show error that need connection
		fprintf(stderr, "TryCrossSiteCrossload: tried to upload to unconnected NetRocks instance\n");
		return true;
	}


	GetSelectedItems gsi;

	if (!gsi.empty()) {
		auto state = StartXfer(0, _remote, CurrentSiteDir(true), dst_remote, dst_site_dir,
			&gsi[0], (int)gsi.size(), mv ? XK_MOVE : XK_COPY, XD_CROSSLOAD);
		if (state == BTS_ACTIVE || state == BTS_PAUSED) {
			_remote = _remote->Clone();
			g_all_netrocks.CloneRemoteOf(plugin);
		}

		G.info.Control(PANEL_ACTIVE, FCTL_UPDATEPANEL, 0, 0);
		G.info.Control(PANEL_PASSIVE, FCTL_UPDATEPANEL, 0, 0);
	}

	return true;
}

bool PluginImpl::ByKey_TryExecuteSelected()
{
	intptr_t size = G.info.Control(this, FCTL_GETSELECTEDPANELITEM, 0, 0);
	if (size < (intptr_t)sizeof(PluginPanelItem)) {
		return false;
	}
	TailedStruct<PluginPanelItem> ppi(size + 0x20 - sizeof(PluginPanelItem));
	G.info.Control(this, FCTL_GETSELECTEDPANELITEM, 0, (LONG_PTR)(void *)ppi.ptr());
	if ( (ppi->FindData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) != 0
	 || (ppi->FindData.dwFileAttributes & FILE_ATTRIBUTE_EXECUTABLE) == 0) {
//		fprintf(stderr, "!!!!dwUnixMode = 0%o\n", ppi->FindData.dwUnixMode );
		return false;
	}

	std::wstring cmd = L"./";
	cmd+= ppi->FindData.lpwszFileName;
	QuoteCmdArgIfNeed(cmd);

	G.info.Control(this, FCTL_SETCMDLINE, 0, (LONG_PTR)cmd.c_str());

	if (!ProcessEventCommand(cmd.c_str())) {
		G.info.Control(this, FCTL_SETCMDLINE, 0, (LONG_PTR)L"");
		return false;
	}

	return true;
}

void PluginImpl::ByKey_EditAttributesSelected()
{
	GetSelectedItems gsi;

	if (!gsi.empty())  try {
		OpChangeMode(_remote, CurrentSiteDir(true), &gsi[0], (int)gsi.size()).Do();
		G.info.Control(PANEL_ACTIVE, FCTL_UPDATEPANEL, 0, 0);

	} catch (std::exception &ex) {
		fprintf(stderr, "ByKey_EditAttributesSelected: %s\n", ex.what());
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
		if (SetDirectoryInternal(dir.empty() ? L"~" : dir.c_str(), 0)) {
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
		OpExecute(_remote, CurrentSiteDir(false), Wide2MB(cmd)).Do();

	} catch (ProtocolUnsupportedError &) {
		const wchar_t *msg[] = { G.GetMsgWide(MCommandsNotSupportedTitle), G.GetMsgWide(MCommandsNotSupportedText), G.GetMsgWide(MOK)};
		G.info.Message(G.info.ModuleNumber, FMSG_WARNING, nullptr, msg, ARRAYSIZE(msg), 1);

	} catch (std::exception &ex) {
		fprintf(stderr, "OpExecute::Do: %s\n", ex.what());

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

	g_conn_pool->Put(_location.server, _remote);
	_remote.reset();
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
