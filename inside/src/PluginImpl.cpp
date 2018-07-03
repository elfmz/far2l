#include "Globals.h"
#include "PluginImpl.h"


bool PluginImpl::AddUnsized(FP_SizeItemList &il, const char *name, DWORD attrs)
{
	PluginPanelItem tmp = {};
	strncpy(tmp.FindData.cFileName, name, sizeof(tmp.FindData.cFileName) - 1 );
	tmp.FindData.dwFileAttributes = attrs;
	tmp.Description = tmp.FindData.cFileName;
	return il.Add(&tmp);
}

PluginImpl::PluginImpl(const char *name)
	: _name(name)
{
}

PluginImpl::~PluginImpl()
{
}

int PluginImpl::GetFindData(PluginPanelItem **pPanelItem, int *pItemsNumber, int OpMode)
{
	fprintf(stderr, "Inside::GetFindData %s@%s\n", _dir.c_str(), _name.c_str());
	FP_SizeItemList il(FALSE);
	bool out = true;

	if(!IS_SILENT(OpMode))
	{
		PluginPanelItem tmp = {};
		strcpy(tmp.FindData.cFileName, "..");
		tmp.FindData.dwFileAttributes = FILE_ATTRIBUTE_DIRECTORY;

		if(!IS_SILENT(OpMode))
		{
			tmp.Description = tmp.FindData.cFileName;
			//LPSTR Data[3] = {tmp.FindData.cFileName, tmp.FindData.cFileName, tmp.FindData.cFileName};
			//tmp.CustomColumnNumber = 3;//ARRAYSIZE(Data);
			//tmp.CustomColumnData = Data;
		}
		if (!il.Add(&tmp))
			out = false;
	}

	out = out && OnGetFindData(il, OpMode);

	if (!out) {
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
	fprintf(stderr, "Inside::SetDirectory('%s', %d)\n", Dir, OpMode);
	if (strcmp(Dir, ".") == 0) {
		
	} else if (strcmp(Dir, "..") == 0) {
		size_t n = _dir.rfind('/');
		if (n == std::string::npos)
			return FALSE;
		_dir.resize(n);

	} else if (Dir[0] != '/') {
		_dir+='/';
		_dir+= Dir;

	} else
		_dir = Dir + 1; // skip root slash

	return TRUE;
}

void PluginImpl::GetOpenPluginInfo(struct OpenPluginInfo *Info)
{
	fprintf(stderr, "Inside::GetOpenPluginInfo: '%s' '%s'\n", _dir.c_str(), _name.c_str());
	snprintf(_panel_title, ARRAYSIZE(_panel_title),
	          " Inside: %s@%s ", _dir.c_str(), _name.c_str());

	Info->Flags = OPIF_SHOWPRESERVECASE | OPIF_USEHIGHLIGHTING;
	Info->HostFile = _name.c_str();
	Info->CurDir   = _dir.c_str();
	Info->PanelTitle = _panel_title;
}

int PluginImpl::ProcessHostFile(struct PluginPanelItem *PanelItem, int ItemsNumber, int OpMode)
{
	fprintf(stderr, "Inside::ProcessHostFile\n");
	return FALSE;
}

int PluginImpl::GetFiles(struct PluginPanelItem *PanelItem, int ItemsNumber, int Move, char *DestPath, int OpMode)
{
	fprintf(stderr, "Inside::GetFiles: _dir='%s' DestPath='%s'\n", _dir.c_str(), DestPath);
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

		bool rv = OnGetFile(PanelItem[i].FindData.cFileName, data_path.c_str(), len);
		fprintf(stderr, "Inside::GetFiles[%i]: %s '%s'\n",
			i, rv ? "OK" : "ERROR", PanelItem[i].FindData.cFileName);
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

		bool rv = OnPutFile(PanelItem[i].FindData.cFileName, data_path.c_str());
		fprintf(stderr, "Inside::PutFiles[%i]: %s '%s'\n",
			i, rv ? "OK" : "ERROR", PanelItem[i].FindData.cFileName);

		if (!rv)
			out = FALSE;
	}

	return out;
}

int PluginImpl::DeleteFiles(struct PluginPanelItem *PanelItem, int ItemsNumber, int OpMode)
{
	if (ItemsNumber == 0 )
		return FALSE;

	for (int i = 0; i < ItemsNumber; ++i) {
		bool rv = OnDeleteFile(PanelItem[i].FindData.cFileName);
		fprintf(stderr, "Inside::DeleteFiles[%i]: %s '%s'\n",
			i, rv ? "OK" : "ERROR", PanelItem[i].FindData.cFileName);
	}

	return TRUE;
}

int PluginImpl::ProcessKey(int Key, unsigned int ControlState)
{
	fprintf(stderr, "Inside::ProcessKey\n");
	return FALSE;
}
