#include "Globals.h"
#include <utils.h>
#include <TailedStruct.hpp>
#include <KeyFileHelper.h>
#include "PluginImpl.h"
#include "PluginPanelItems.h"
#include "Host/HostLocal.h"
#include "UI/SiteConnectionEditor.h"
#include "Op/OpConnect.h"
#include "Op/OpGetMode.h"
#include "Op/OpXfer.h"
#include "Op/OpRemove.h"
#include "Op/OpMakeDirectory.h"
#include "Op/OpEnumDirectory.h"

PluginImpl::PluginImpl(const wchar_t *path)
{
	_cur_dir[0] = _panel_title[0] = 0;
	_local.reset((IHost *)new HostLocal());
	UpdatePanelTitle();
}

PluginImpl::~PluginImpl()
{
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
		out = Wide2MB(slash + 1);
	}

	if (out.empty()) {
		out = with_ending_slash ? "./" : ".";

	} else if (out[out.size() - 1] != '/') {
		if (with_ending_slash)
			out+= "/";

	} else if (!with_ending_slash) {
		out.resize(out.size() - 1);
		if (out.empty())
			out = ".";
	}
	return out;
}

int PluginImpl::GetFindData(PluginPanelItem **pPanelItem, int *pItemsNumber, int OpMode)
{
	fprintf(stderr, "NetRocks::GetFindData '%ls' G.config='%s'\n", _cur_dir, G.config.c_str());
	PluginPanelItems ppis;
	try {
		if (!ValidateConnection()) {
			_cur_dir[0] = 0;
			KeyFileHelper kfh(G.config.c_str());
			const std::vector<std::string> &sites = kfh.EnumSections();
			for (const auto &site : sites) {
				auto *ppi = ppis.Add(site.c_str());
				ppi->FindData.dwFileAttributes = FILE_ATTRIBUTE_DIRECTORY;
			}

		} else {
			auto *ppi = ppis.Add(L"..");
			ppi->FindData.dwFileAttributes = FILE_ATTRIBUTE_DIRECTORY;
			OpEnumDirectory(_remote, OpMode, CurrentSiteDir(false), ppis).Do();
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

	try {
		std::string tmp;
		Wide2MB(_cur_dir, tmp);

		if (wcscmp(Dir, L"..") == 0) {
			size_t p = tmp.rfind('/');
			if (p == std::string::npos)
				p = 0;
			tmp.resize(p);

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

//			if (p != std::string::npos)
//				tmp.erase(0, p + 1);
//			else
//				tmp.clear();
		}

		if (p != std::string::npos && p < tmp.size() - 1) {
			mode_t mode = 0;
			if (!OpGetMode(_remote, OpMode, tmp.substr(p + 1)).Do(mode))
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


int PluginImpl::GetFiles(struct PluginPanelItem *PanelItem, int ItemsNumber, int Move, const wchar_t *DestPath, int OpMode)
{
	fprintf(stderr, "NetRocks::GetFiles: _dir='%ls' DestPath='%ls' ItemsNumber=%d\n", _cur_dir, DestPath, ItemsNumber);
	if (ItemsNumber <= 0)
		return FALSE;

	std::string dst_dir;
	if (DestPath)
		Wide2MB(DestPath, dst_dir);

	return OpXfer(_remote, OpMode, CurrentSiteDir(true), _local, dst_dir,
		PanelItem, ItemsNumber, Move ? XK_MOVE : XK_COPY, XK_DOWNLOAD).Do() ? TRUE : FALSE;
}

int PluginImpl::PutFiles(struct PluginPanelItem *PanelItem, int ItemsNumber, int Move, const wchar_t *SrcPath, int OpMode)
{
	const std::string &site_dir = CurrentSiteDir(true);
	fprintf(stderr, "NetRocks::GetFiles: _dir='%ls' SrcPath='%ls' site_dir='%s' ItemsNumber=%d\n", _cur_dir, SrcPath, site_dir.c_str(), ItemsNumber);
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

	return OpXfer(_local, OpMode, dst_dir, _remote, CurrentSiteDir(true),
		PanelItem, ItemsNumber, Move ? XK_MOVE : XK_COPY, XK_UPLOAD).Do() ? TRUE : FALSE;
}

int PluginImpl::DeleteFiles(struct PluginPanelItem *PanelItem, int ItemsNumber, int OpMode)
{
	fprintf(stderr, "NetRocks::DeleteFiles: _dir='%ls' ItemsNumber=%d\n", _cur_dir, ItemsNumber);
	if (ItemsNumber <= 0)
		return FALSE;

	return OpRemove(_remote, OpMode, CurrentSiteDir(true), PanelItem, ItemsNumber).Do() ? TRUE : FALSE;
}

int PluginImpl::MakeDirectory(const wchar_t *Name, int OpMode)
{
	fprintf(stderr, "NetRocks::MakeDirectory('%ls', 0x%x)\n", Name, OpMode);
	if (_cur_dir[0]) {
		std::string tmp;
		if (Name)
			Wide2MB(Name, tmp);
		return OpMakeDirectory(_remote, OpMode, CurrentSiteDir(true), Name ? tmp.c_str() : nullptr).Do();
	} else {
		;//todo
	}
	return FALSE;
}

int PluginImpl::ProcessKey(int Key, unsigned int ControlState)
{
	fprintf(stderr, "NetRocks::ProcessKey(0x%x, 0x%x)\n", Key, ControlState);

	if ((Key==VK_F5 || Key==VK_F6) && _remote)
	{
	}

	if (Key==VK_F4 && !_cur_dir[0]
	&& (ControlState == 0 || ControlState == PKF_SHIFT))
	{
		std::string site;

		if (ControlState == 0) {
    			intptr_t size = G.info.Control(this, FCTL_GETSELECTEDPANELITEM, 0, 0);
			if (size >= (intptr_t)sizeof(PluginPanelItem)) {
				TailedStruct<PluginPanelItem> ppi(0x100 + size - sizeof(PluginPanelItem));
				G.info.Control(this, FCTL_GETSELECTEDPANELITEM, 0, (LONG_PTR)(void *)ppi.ptr());
				//if ((ppi->Flags & PPIF_SELECTED) != 0) {
					Wide2MB(ppi->FindData.lpwszFileName, site);
				//}
			}
		}

		SiteConnectionEditor sce(site);
		const bool connect_now = sce.Edit();
		G.info.Control(PANEL_ACTIVE, FCTL_UPDATEPANEL, 0, 0);
		if (connect_now) {
			SetDirectory(StrMB2Wide(sce.DisplayName()).c_str(), 0);
		}


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
