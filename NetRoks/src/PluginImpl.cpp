#include "Globals.h"
#include <KeyFileHelper.h>
#include "PluginImpl.h"


PluginImpl::PluginImpl(const std::string &path)
{
	_cur_dir[0] = _panel_title[0] = 0;
}

PluginImpl::~PluginImpl()
{
}

void PluginImpl::UpdatePanelTitle()
{
	std::string tmp;
	if (_connection) {
		tmp = _connection->SiteInfo();
//		tmp+= '/';
		tmp+= _cur_dir;
	} else {
		tmp = "NetRoks connections list";
	}

	if (tmp.size() >= sizeof(_panel_title)) {
		size_t rmlen = 4 + (tmp.size() - sizeof(_panel_title));
		tmp.replace((tmp.size() - rmlen) / 2 , rmlen, "...");
	}
	
	memcpy(_panel_title, tmp.c_str(), tmp.size() + 1);
}

int PluginImpl::GetFindData(PluginPanelItem **pPanelItem, int *pItemsNumber, int OpMode)
{
	fprintf(stderr, "NetRoks::GetFindData %s\n", _cur_dir);
	FP_SizeItemList il(FALSE);
	try {
		if (!_connection || _connection->IsBroken()) {
			_connection.reset();
			_cur_dir[0] = 0;
			KeyFileHelper kfh(G.config.c_str());
			const std::vector<std::string> &sites = kfh.EnumSections();
			for (const auto &site : sites) {
				PluginPanelItem tmp = {};
				strncpy(tmp.FindData.cFileName, site.c_str(), sizeof(tmp.FindData.cFileName));
				tmp.FindData.dwFileAttributes = FILE_ATTRIBUTE_DIRECTORY;
				if (!il.Add(&tmp))
					throw std::runtime_error("Can't add list entry");
			}

		} else {
			_connection->DirectoryEnum(_cur_dir, il, OpMode);
		}

	} catch (std::exception &e) {
		fprintf(stderr, "NetRoks::GetFindData('%s', %d) - %s\n", _cur_dir, OpMode, e.what());
		il.Free(il.Items(), il.Count() );
		*pPanelItem = nullptr;
		*pItemsNumber = 0;
		return FALSE;
	}

	*pPanelItem = il.Items();
	*pItemsNumber = il.Count();
	return TRUE;
}

void PluginImpl::FreeFindData(PluginPanelItem *PanelItem, int ItemsNumber)
{
	FP_SizeItemList::Free(PanelItem, ItemsNumber);
}

int PluginImpl::SetDirectory(const char *Dir,int OpMode)
{
	fprintf(stderr, "NetRoks::SetDirectory('%s', %d)\n", Dir, OpMode);
	if (strcmp(Dir, ".") == 0)
		return TRUE;

	try {
		std::string tmp = _cur_dir;

		if (strcmp(Dir, "..") == 0) {
			size_t p = tmp.rfind('/');
			if (p == std::string::npos)
				p = 0;
			tmp.resize(p);

		} else if (*Dir == '/') {
			tmp = Dir;

		} else {
			tmp+= '/';
			tmp+= Dir;
		}

		if (!_connection) {
			size_t p = tmp.find('/');
			const std::string &site = tmp.substr(0, p);
			try {
				_connection.reset(new SiteConnection(site, OpMode));

			} catch (std::runtime_error &e) {
				fprintf(stderr,
					"NetRoks::SetDirectory: new SiteConnection('%s') - %s\n",
					site.c_str(), e.what());

				return FALSE;
			}

			if (p != std::string::npos)
				tmp.erase(0, p + 1);
			else
				tmp.clear();
		}

		if (!tmp.empty() && tmp != "/") {
			mode_t mode = _connection->GetMode(tmp);
			if (!S_ISDIR(mode))
				throw std::runtime_error("Not a directory");
		}

		if (tmp.size() >= sizeof(_cur_dir))
			throw std::runtime_error("Too long path");

		memcpy(_cur_dir, tmp.c_str(), tmp.size() + 1);

	} catch (std::exception &e) {
		fprintf(stderr, "NetRoks::SetDirectory('%s', %d) - %s\n", Dir, OpMode, e.what());
		return FALSE;
	}

	return TRUE;
}

void PluginImpl::GetOpenPluginInfo(struct OpenPluginInfo *Info)
{
	fprintf(stderr, "Inside::GetOpenPluginInfo: '%s' \n", _cur_dir);
//	snprintf(_panel_title, ARRAYSIZE(_panel_title),
//	          " Inside: %s@%s ", _dir.c_str(), _name.c_str());

	Info->Flags = OPIF_SHOWPRESERVECASE | OPIF_USEHIGHLIGHTING;
	Info->HostFile = NULL;
	Info->CurDir = _cur_dir;
	Info->PanelTitle = _panel_title;
}

int PluginImpl::GetFiles(struct PluginPanelItem *PanelItem, int ItemsNumber, int Move, char *DestPath, int OpMode)
{
	fprintf(stderr, "Inside::GetFiles: _dir='%s' DestPath='%s'\n", _cur_dir, DestPath);
	if (ItemsNumber == 0 || Move)
		return FALSE;

	BOOL out = TRUE;
	std::string data_path;
	for (int i = 0; i < ItemsNumber; ++i) {
		data_path = DestPath;
		if (!data_path.empty() && data_path[data_path.size() - 1] != '/') {
			data_path+= '/';
		}
		data_path+= PanelItem[i].FindData.cFileName;

		uint64_t len = PanelItem[i].FindData.nFileSizeHigh;
		len<<= 32;
		len|= PanelItem[i].FindData.nFileSizeLow;

		//bool rv = OnGetFile(PanelItem[i].FindData.cFileName, data_path.c_str(), len);
		//fprintf(stderr, "Inside::GetFiles[%i]: %s '%s'\n",
		//	i, rv ? "OK" : "ERROR", PanelItem[i].FindData.cFileName);
	}

	return out;
}


int PluginImpl::PutFiles(struct PluginPanelItem *PanelItem, int ItemsNumber, int Move, int OpMode)
{
	if (ItemsNumber == 0 || Move)
		return FALSE;

	BOOL out = TRUE;
	char cd[0x1000] = {};
	sdc_getcwd(cd, sizeof(cd) - 1);
	std::string data_path;
	for (int i = 0; i < ItemsNumber; ++i) {
		if (PanelItem[i].FindData.cFileName[0] != '/') {
			data_path = cd;
			data_path+= '/';
			data_path+= PanelItem[i].FindData.cFileName;
		} else
			data_path = PanelItem[i].FindData.cFileName;

		//bool rv = OnPutFile(PanelItem[i].FindData.cFileName, data_path.c_str());
		//fprintf(stderr, "Inside::PutFiles[%i]: %s '%s'\n",
		//	i, rv ? "OK" : "ERROR", PanelItem[i].FindData.cFileName);

		//if (!rv)
		//	out = FALSE;
	}

	return out;
}

int PluginImpl::DeleteFiles(struct PluginPanelItem *PanelItem, int ItemsNumber, int OpMode)
{
	if (ItemsNumber == 0 )
		return FALSE;

	for (int i = 0; i < ItemsNumber; ++i) {
		//bool rv = OnDeleteFile(PanelItem[i].FindData.cFileName);
		//fprintf(stderr, "Inside::DeleteFiles[%i]: %s '%s'\n",
		//	i, rv ? "OK" : "ERROR", PanelItem[i].FindData.cFileName);
	}

	return TRUE;
}

int PluginImpl::ProcessKey(int Key, unsigned int ControlState)
{
	fprintf(stderr, "Inside::ProcessKey\n");
	return FALSE;
}
