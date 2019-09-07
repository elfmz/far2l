#include <vector>
#include <wchar.h>
#include "Globals.h"
#include <utils.h>
#include <TailedStruct.hpp>
#include "SitesConfig.h"
#include "PluginImpl.h"
#include "PluginPanelItems.h"
#include "PooledStrings.h"
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
		std::wstring protocol, host, username, password, directory;
		unsigned int port = 0;

		if (!SplitPathSpecification(path, protocol, host, port, username, password, directory)) {
			throw std::runtime_error(G.GetMsgMB(MWrongPath));
		}

		_remote = OpConnect(0, StrWide2MB(protocol), StrWide2MB(host), port,
			StrWide2MB(username), StrWide2MB(password), StrWide2MB(directory)).Do();

		if (!_remote) {
			throw std::runtime_error(G.GetMsgMB(MCouldNotConnect));
		}

		_wea_state = std::make_shared<WhatOnErrorState>();
		
		wcsncpy(_cur_dir, StrMB2Wide(_remote->SiteName()).c_str(), ARRAYSIZE(_cur_dir) - 1 );
		if (!directory.empty()) {
			_cur_dir_absolute = (directory[0] == '/');
			while (!directory.empty() && directory[directory.size() - 1] == '/') {
				directory.resize(directory.size() - 1);
			}
			wcsncat(_cur_dir, directory.c_str(), ARRAYSIZE(_cur_dir) - 1 );
		}
	}

	g_all_netrocks.Add(this);
	UpdatePanelTitle();
}

PluginImpl::~PluginImpl()
{
	g_all_netrocks.Remove(this);
}

void PluginImpl::UpdatePanelTitle()
{
	std::wstring tmp;
	if (_remote) {
//		tmp = _remote->SiteInfo();
//		tmp+= '/';
		tmp+= _cur_dir;
	} else {
		tmp = L"NetRocks connections list";
	}

	if (tmp.size() >= ARRAYSIZE(_panel_title)) {
		size_t rmlen = 4 + (tmp.size() - ARRAYSIZE(_panel_title));
		tmp.replace((tmp.size() - rmlen) / 2 , rmlen, L"...");
	}
	
	wcscpy(_panel_title, tmp.c_str());
}

bool PluginImpl::ValidateConnection()
{
	return _remote != nullptr;
}

std::string PluginImpl::CurrentSiteDir(bool with_ending_slash) const
{
	std::string out;
	const wchar_t *slash = wcschr(_cur_dir, '/');
	if (slash) {
		out+= Wide2MB(slash + 1);
	}

	if (out.empty()) {
		out = with_ending_slash ? "./" : ".";

	} else if (out[out.size() - 1] != '/') {
		if (with_ending_slash) {
			out+= "/";
		}

	} else if (!with_ending_slash) {
		out.resize(out.size() - 1);
		if (out.empty()) {
			out = ".";
		}
	}

	if (_cur_dir_absolute && (out.empty() || out[0] != '/')) {
		out.insert(0, "/");
	}

	fprintf(stderr, "CurrentSiteDir: out='%s'\n", out.c_str());
	return out;
}

int PluginImpl::GetFindData(PluginPanelItem **pPanelItem, int *pItemsNumber, int OpMode)
{
	fprintf(stderr, "NetRocks::GetFindData '%ls'\n", _cur_dir);
	PluginPanelItems ppis;
	try {
		auto *ppi = ppis.Add(L"..");
		ppi->FindData.dwFileAttributes = FILE_ATTRIBUTE_DIRECTORY;

		if (!ValidateConnection()) {
			_cur_dir[0] = 0;
			SitesConfig sc;
			auto *ppi = ppis.Add(G.GetMsgWide(MCreateSiteConnection));
			ppi->FindData.dwFileAttributes = FILE_ATTRIBUTE_DIRECTORY;

			const std::vector<std::string> &sites = sc.EnumSites();
			for (const auto &site : sites) {
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

static void TokenizePath(std::vector<std::string> &components, const std::string &path)
{
	StrExplode(components, path, "/");
}

static void TokenizePath(std::vector<std::string> &components, const wchar_t *path)
{
	TokenizePath(components, Wide2MB(path));
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
	if (!_remote && wcscmp(Dir, G.GetMsgWide(MCreateSiteConnection)) == 0) {
		ByKey_EditSiteConnection(true);
		return FALSE;
	}

	StackedDir sd;
	StackedDirCapture(sd);

	if (*Dir == L'/') {
		do { ++Dir; } while (*Dir == L'/');

		const wchar_t *slash = wcschr(Dir, L'/');
		size_t site_len = slash ? slash - Dir : wcslen(Dir);

		if (_remote && memcmp(_cur_dir, Dir, site_len * sizeof(wchar_t)) == 0
			&& (_cur_dir[site_len] == '/' || _cur_dir[site_len] == 0)) {

			if (!SetDirectoryInternal(L"~", OpMode)) {
				return FALSE;
			}

		} else {
			_remote.reset();
			_cur_dir[0] = 0;

			if (!SetDirectoryInternal(std::wstring(Dir, site_len).c_str(), OpMode)) {
				StackedDirApply(sd);
				return FALSE;
			}
		}

		for (Dir+= site_len; *Dir == L'/'; ++Dir);
	}

	if (*Dir && !SetDirectoryInternal(Dir, OpMode)) {
		StackedDirApply(sd);
		return FALSE;
	}

	return TRUE;
}

int PluginImpl::SetDirectoryInternal(const wchar_t *Dir, int OpMode)
{
	bool absolute;
	std::vector<std::string> components, prev_components;
	TokenizePath(prev_components, _cur_dir);

	TokenizePath(components, Dir);

	if (!components.empty() && components[0] == "~") {
		if (prev_components.empty()) {
			fprintf(stderr, "NetRocks::SetDirectory - can't go home when unconnected: '%ls'\n", Dir);
			return FALSE;
		}

		std::string default_dir = SitesConfig().GetDirectory(prev_components.front());
		std::vector<std::string> default_components;
		TokenizePath(default_components, default_dir);
		components.erase(components.begin());
		components.insert(components.begin(), default_components.begin(), default_components.end());
		components.insert(components.begin(), prev_components.front());
		absolute = (!default_dir.empty() && default_dir[0] == '/');

	} else if (*Dir != L'/') {
		if (components.empty()) {
			return TRUE;
		}
		components.insert(components.begin(), prev_components.begin(), prev_components.end());
		absolute = _cur_dir_absolute;

	} else {
		components.insert(components.begin(), prev_components.front());
		absolute = true;
	}

	if (!PreprocessPathTokens(components) || components.empty()) {
		fprintf(stderr, "NetRocks::SetDirectory - close connection due to: '%ls'\n", Dir);
		_remote.reset();
		_cur_dir[0] = 0;
		UpdatePanelTitle();
		return TRUE;
	}

	if (!_remote) {
		if (!components.empty()) {
			_remote = OpConnect(OpMode, components[0]).Do();
			if (!_remote) {
				fprintf(stderr, "NetRocks::SetDirectory - can't connect to: '%s'\n", components[0].c_str());
				return FALSE;
			}
			_wea_state = std::make_shared<WhatOnErrorState>();

			if (components.size() == 1) {
				std::string default_dir = SitesConfig().GetDirectory(components[0]);
				TokenizePath(components, default_dir);
				absolute = !default_dir.empty() && default_dir[0] == '/';
			}


		} else {
			fprintf(stderr, "NetRocks::SetDirectory - not connected and nowhere to connect to: '%ls'\n", Dir);
			return FALSE;
		}

	}

	std::string dir;
	if (absolute) {
		dir = '/';
	}

	for (const auto &component : components) {
		if (&component != &components[0]) {
			if (!dir.empty() && dir != "/") {
				dir+= '/';
			}
			dir+= component;
		}
	}
	fprintf(stderr, "NetRocks::SetDirectory - dir: '%s'\n", dir.c_str());
	if (!dir.empty()) {
		mode_t mode = 0;
		if (!OpGetMode(OpMode, _remote, dir, _wea_state).Do(mode)) {
			fprintf(stderr, "NetRocks::SetDirectory - can't get mode: '%s'\n", dir.c_str());
			return FALSE; 
		}

		if (!S_ISDIR(mode)) {
			fprintf(stderr, "NetRocks::SetDirectory - not dir mode=0x%x: '%s'\n", mode, dir.c_str());
			return FALSE;
		}
	}

	if (!components.empty()) {
		if (!dir.empty() && dir[0] != '/') {
			dir.insert(0, "/");
		}
		dir.insert(0, components.front());
	}

	std::wstring tmp = StrMB2Wide(dir);
	while (!tmp.empty() && tmp[tmp.size() - 1] == '/') {
		tmp.resize(tmp.size() - 1);
	}
	if (tmp.size() >= ARRAYSIZE(_cur_dir)) {
		fprintf(stderr, "NetRocks::SetDirectory - too long: '%s'\n", dir.c_str());
		return FALSE;
	}

	memcpy(_cur_dir, tmp.c_str(), (tmp.size() + 1) * sizeof(_cur_dir[0]));
	_cur_dir_absolute = absolute;

	fprintf(stderr, "NetRocks::SetDirectory('%ls', %d) OK: %s'%ls'\n", Dir, OpMode, _cur_dir_absolute ? "/" : "", &_cur_dir[0]);

	UpdatePanelTitle();
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
	if (!_cur_dir[0]) {
		// todo: create virtual dir in sites list
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
	if (Key == VK_RETURN && _remote && G.global_config
	 && G.global_config->GetInt("Options", "EnterExecsRemotely", 0) != 0) {
		return ByKey_TryExecuteSelected() ? TRUE : FALSE;
	}

	if ((Key == VK_F5 || Key == VK_F6) && _remote) {
		return ByKey_TryCrossload(Key==VK_F6) ? TRUE : FALSE;
	}

	if (Key == VK_F4 && !_cur_dir[0]
	&& (ControlState == 0 || ControlState == PKF_SHIFT)) {
		ByKey_EditSiteConnection(ControlState == PKF_SHIFT);
		return TRUE;
	}

	if (Key == VK_F3 && !_cur_dir[0]) {
		return TRUE;
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


	PanelInfo pi = {};
	G.info.Control(PANEL_ACTIVE, FCTL_GETPANELINFO, 0, (LONG_PTR)(void *)&pi);
	if (pi.SelectedItemsNumber == 0) {
		fprintf(stderr, "TryCrossSiteCrossload: no files selected\n");
		return true;
	}

	std::vector<PluginPanelItem> items_vector;
	std::vector<PluginPanelItem *> items_to_free;
	for (int i = 0; i < pi.SelectedItemsNumber; ++i) {
		size_t len = G.info.Control(PANEL_ACTIVE, FCTL_GETSELECTEDPANELITEM, i, 0);
		if (len >= sizeof(PluginPanelItem)) {
			PluginPanelItem *pi = (PluginPanelItem *)calloc(1, len + 0x20);
			if (pi == nullptr)
				break;

			G.info.Control(PANEL_ACTIVE, FCTL_GETSELECTEDPANELITEM, i, (LONG_PTR)(void *)pi);
			items_to_free.push_back(pi);
			if (pi->FindData.lpwszFileName)
				items_vector.push_back(*pi);
		}
	}

	if (!items_vector.empty()) {
		auto state = StartXfer(0, _remote, CurrentSiteDir(true), dst_remote, dst_site_dir,
			&items_vector[0], (int)items_vector.size(), mv ? XK_MOVE : XK_COPY, XD_CROSSLOAD);
		if (state == BTS_ACTIVE || state == BTS_PAUSED) {
			_remote = _remote->Clone();
			g_all_netrocks.CloneRemoteOf(plugin);
		}

		G.info.Control(PANEL_ACTIVE, FCTL_UPDATEPANEL, 0, 0);
		G.info.Control(PANEL_PASSIVE, FCTL_UPDATEPANEL, 0, 0);
	}

	for (auto &pi : items_to_free) {
		free(pi);
	}

	return true;
}

bool PluginImpl::ByKey_TryExecuteSelected()
{
	return false;
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
	sd.cur_dir = _cur_dir;
	sd.cur_dir_absolute = _cur_dir_absolute;
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
	wcsncpy(_cur_dir, sd.cur_dir.c_str(), ARRAYSIZE(_cur_dir) - 1);
	_cur_dir_absolute = sd.cur_dir_absolute;
}

int PluginImpl::ProcessEventCommand(const wchar_t *cmd)
{
	if (wcsstr(cmd, L"exit ") == cmd || wcscmp(cmd, L"exit") == 0) {
		if (GetCommandArgument(cmd) == L"far") {
			return FALSE;
		}
		_remote.reset();
		_cur_dir[0] = 0;
		UpdatePanelTitle();

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
			UpdatePanelTitle();
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
	}

	G.info.Control(this, FCTL_SETCMDLINE, 0, (LONG_PTR)L"");
	G.info.Control(PANEL_ACTIVE, FCTL_UPDATEPANEL, 0, 0);
	G.info.Control(PANEL_ACTIVE, FCTL_REDRAWPANEL, 0, 0);
	G.info.Control(PANEL_PASSIVE, FCTL_UPDATEPANEL, 0, 0);
	G.info.Control(PANEL_PASSIVE, FCTL_REDRAWPANEL, 0, 0);

	return TRUE;
}
