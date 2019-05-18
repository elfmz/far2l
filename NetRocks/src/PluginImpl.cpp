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
		
		wcsncpy(_cur_dir, StrMB2Wide(_remote->SiteName()).c_str(), ARRAYSIZE(_cur_dir) - 1 );
		if (!directory.empty()) {
			wcsncat(_cur_dir, directory.c_str(), ARRAYSIZE(_cur_dir) - 1 );
			_cur_dir_absolute = (directory[0] == '/');
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
	try {
		if (!_remote)
			return false;

		if (_remote->IsBroken()) 
			throw std::runtime_error("Connection broken");

	} catch (std::exception &e) {
		fprintf(stderr, "NetRocks::ValidateConnection: %s\n", e.what());
		_remote.reset();
		return false;
	}

	return true;
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
			auto *ppi = ppis.Add(L"..");
			ppi->FindData.dwFileAttributes = FILE_ATTRIBUTE_DIRECTORY;
			OpEnumDirectory(OpMode, _remote, CurrentSiteDir(false), ppis).Do();
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

int PluginImpl::SetDirectory(const wchar_t *Dir, int OpMode)
{
	fprintf(stderr, "NetRocks::SetDirectory('%ls', %d)\n", Dir, OpMode);
	if (wcscmp(Dir, L".") == 0)
		return TRUE;

	if (!_remote && wcscmp(Dir, G.GetMsgWide(MCreateSiteConnection)) == 0) {
		ByKey_EditSiteConnection(true);
		return FALSE;
	}

	try {
		std::string tmp;
		Wide2MB(_cur_dir, tmp);

		if (wcscmp(Dir, L"..") == 0) {
			size_t p = tmp.rfind('/');
			if (p == std::string::npos)
				p = 0;
			tmp.resize(p);

			if (tmp.empty()) {
				_remote.reset();
				_cur_dir[0] = 0;
				UpdatePanelTitle();
				return TRUE;
			}

		} else if (*Dir == L'/') {
			Wide2MB(Dir + 1, tmp);

		} else {
			if (!tmp.empty())
				tmp+= '/';
			tmp+= Wide2MB(Dir);
		}
		while (!tmp.empty() && tmp[tmp.size() - 1] == '/')
			tmp.resize(tmp.size() - 1);

		size_t p = tmp.find('/');
		if (!_remote) {
			const std::string &site = tmp.substr(0, p);
			_remote = OpConnect(OpMode, site).Do();
			if (!_remote) {
				return FALSE;
			}

			std::string root_dir = SitesConfig().GetDirectory(site);
			if (!root_dir.empty()) {
				_cur_dir_absolute = (root_dir[0] == '/');

				if (root_dir[root_dir.size() - 1] != '/')
					root_dir+= '/';

				if (_cur_dir_absolute)
					root_dir.erase(0, 1);

				if (!root_dir.empty()) {
					if (p == std::string::npos) {
						p = tmp.size();
						tmp+= '/';
					}
					tmp.insert(p + 1, root_dir);
				}
			} else {
				 _cur_dir_absolute = false;
			}
		}

//		fprintf(stderr, "tmp='%s'\n", tmp.c_str());

		if (p < tmp.size() - 1) {
			mode_t mode = 0;
			std::string chk_path = tmp.substr(p + 1);
			if (_cur_dir_absolute)
				chk_path.insert(0, "/");
			fprintf(stderr, "chk_path='%s'\n", chk_path.c_str());

			if (!OpGetMode(OpMode, _remote, chk_path).Do(mode))
				throw std::runtime_error("Get mode failed");

			if (!S_ISDIR(mode))
				throw std::runtime_error("Not a directory");
		}

		const std::wstring &tmp_ws = StrMB2Wide(tmp);

		if (tmp_ws.size() >= ARRAYSIZE(_cur_dir))
			throw std::runtime_error("Too long path");

		wcscpy(_cur_dir, tmp_ws.c_str());

	} catch (std::exception &e) {
		fprintf(stderr, "NetRocks::SetDirectory('%ls', %d) ERROR: %s\n", Dir, OpMode, e.what());
		return FALSE;
	}

	UpdatePanelTitle();

	fprintf(stderr, "NetRocks::SetDirectory('%ls', %d) OK: '%ls'\n", Dir, OpMode, &_cur_dir[0]);

	return TRUE;
}

void PluginImpl::GetOpenPluginInfo(struct OpenPluginInfo *Info)
{
	fprintf(stderr, "NetRocks::GetOpenPluginInfo: '%ls' \n", &_cur_dir[0]);
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
	fprintf(stderr, "NetRocks::GetFiles: _dir='%ls' DestPath='%ls' ItemsNumber=%d\n", _cur_dir, DestPath, ItemsNumber);
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
	fprintf(stderr, "NetRocks::GetFiles: _dir='%ls' SrcPath='%ls' site_dir='%s' ItemsNumber=%d\n", _cur_dir, SrcPath, site_dir.c_str(), ItemsNumber);
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

int PluginImpl::MakeDirectory(const wchar_t *Name, int OpMode)
{
	fprintf(stderr, "NetRocks::MakeDirectory('%ls', 0x%x)\n", Name, OpMode);
	if (!_remote) {
		return FALSE;
	}
	if (_cur_dir[0]) {
		std::string tmp;
		if (Name) {
			Wide2MB(Name, tmp);
		}
		return OpMakeDirectory(OpMode, _remote, CurrentSiteDir(true), Name ? tmp.c_str() : nullptr).Do();
	} else {
		;//todo
	}
	return FALSE;
}

int PluginImpl::ProcessKey(int Key, unsigned int ControlState)
{
//	fprintf(stderr, "NetRocks::ProcessKey(0x%x, 0x%x)\n", Key, ControlState);

	if ((Key == VK_F5 || Key == VK_F6) && _remote)
	{
		return ByKey_TryCrossload(Key==VK_F6) ? TRUE : FALSE;
	}

	if (Key == VK_F4 && !_cur_dir[0]
	&& (ControlState == 0 || ControlState == PKF_SHIFT))
	{
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
		SetDirectory(StrMB2Wide(sce.DisplayName()).c_str(), 0);
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
