#include "Globals.h"
#include "PluginImpl.h"
#include "Dumper.h"

#define STR_DISASM		"Disassembly"
	
PluginImpl::PluginImpl(uint16_t machine, const char *name)
	: _name(name), _machine(machine)
{
	fprintf(stderr, "ObjInfo::PluginImpl(0x%x, %s)\n", machine, name);
}

PluginImpl::~PluginImpl()
{
}

bool PluginImpl::GetFindData_ROOT(FP_SizeItemList &il, int OpMode)
{
	PluginPanelItem tmp;
	std::set<std::string> commands;
	Root::Commands(commands);
	for (const auto &command : commands) {
		memset(&tmp, 0, sizeof(tmp));
		strncpy(tmp.FindData.cFileName, command.c_str(), sizeof(tmp.FindData.cFileName) - 1);
		tmp.Description = tmp.FindData.cFileName;
		if (!il.Add(&tmp))
			return false;
	}

	memset(&tmp, 0, sizeof(tmp));
	strncpy(tmp.FindData.cFileName, STR_DISASM, sizeof(tmp.FindData.cFileName) - 1);
	tmp.Description = tmp.FindData.cFileName;
	tmp.FindData.dwFileAttributes = FILE_ATTRIBUTE_DIRECTORY;
	if (!il.Add(&tmp))
		return false;

	return true;
}

bool PluginImpl::GetFindData_DISASM(FP_SizeItemList &il, int OpMode)
{
	PluginPanelItem tmp;
	std::set<std::string> commands;
	Disasm::Commands(_machine, commands);
	for (const auto &command : commands) {
		memset(&tmp, 0, sizeof(tmp));
		strncpy(tmp.FindData.cFileName, command.c_str(), sizeof(tmp.FindData.cFileName) - 1);
		tmp.Description = tmp.FindData.cFileName;
		if (!il.Add(&tmp))
			return false;
	}

	return true;
}


int PluginImpl::GetFindData(PluginPanelItem **pPanelItem, int *pItemsNumber, int OpMode)
{
	fprintf(stderr, "ObjInfo::GetFindData %s@%s\n", _dir.c_str(), _name.c_str());
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

	if (_dir.empty()) {
		out = out && GetFindData_ROOT(il, OpMode); 
	} else if (_dir == "/" STR_DISASM) {
		out = out && GetFindData_DISASM(il, OpMode); 
	} else {
	}

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
	fprintf(stderr, "ObjInfo::SetDirectory('%s', %d)\n", Dir, OpMode);
	if (strcmp(Dir, ".") == 0) {
		
	} else if (strcmp(Dir, "..") == 0) {
		size_t n = _dir.rfind('/');
		if (n == std::string::npos)
			return FALSE;
		_dir.resize(n);

	} else {
		_dir+='/';
		_dir+= Dir;
	}
	return TRUE;
}

void PluginImpl::GetOpenPluginInfo(struct OpenPluginInfo *Info)
{
	fprintf(stderr, "ObjInfo::GetOpenPluginInfo\n");
	snprintf(_panel_title, ARRAYSIZE(_panel_title),
	          " ObjInfo: %s@%s ", _dir.c_str(), _name.c_str());

	Info->Flags = OPIF_SHOWPRESERVECASE | OPIF_USEHIGHLIGHTING;
	Info->HostFile = _name.c_str();
	Info->CurDir   = _dir.c_str();
	Info->PanelTitle = _panel_title;
}

int PluginImpl::DeleteFiles(struct PluginPanelItem *PanelItem, int ItemsNumber, int OpMode)
{
	fprintf(stderr, "ObjInfo::DeleteFiles\n");
	return FALSE;
}

int PluginImpl::ProcessHostFile(struct PluginPanelItem *PanelItem, int ItemsNumber, int OpMode)
{
	fprintf(stderr, "ObjInfo::ProcessHostFile\n");
	return FALSE;
}

int PluginImpl::GetFiles(struct PluginPanelItem *PanelItem, int ItemsNumber, int Move, char *DestPath, int OpMode)
{
	fprintf(stderr, "ObjInfo::GetFiles: _dir='%s' DestPath='%s'\n", _dir.c_str(), DestPath);
	if (ItemsNumber == 0 || Move)
		return FALSE;

	BOOL out = TRUE;
	std::string dst;
	for (int i = 0; i < ItemsNumber; ++i) {
		dst = DestPath;
		if (!dst.empty() && dst[dst.size() - 1] != '/') {
			dst+= '/';
		}
		dst+= PanelItem[i].FindData.cFileName;
		if (_dir.empty()) {
			Root::Query(PanelItem[i].FindData.cFileName, _name, dst);

		} else if (_dir == "/" STR_DISASM ) {
			Disasm::Query(_machine, PanelItem[i].FindData.cFileName, _name, dst);
		}
		fprintf(stderr, "ObjInfo::GetFiles[%i]: %s\n", i, PanelItem[i].FindData.cFileName);
	}

	return out;
}

int PluginImpl::PutFiles(struct PluginPanelItem *PanelItem, int ItemsNumber, int Move, int OpMode)
{
	fprintf(stderr, "ObjInfo::PutFiles: _dir='%s'\n", _dir.c_str());
	if (ItemsNumber == 0 || Move)
		return FALSE;

	return FALSE;
}

int PluginImpl::ProcessKey(int Key, unsigned int ControlState)
{
	fprintf(stderr, "ObjInfo::ProcessKey\n");
	return FALSE;
}


