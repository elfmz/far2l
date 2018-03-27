#include "Globals.h"
#include "PluginImpl.h"
#include "Dumper.h"
#include "ELFInfo.h"



PluginImpl::PluginImpl(const char *name, uint8_t bitness, uint8_t endianess)
	: _name(name), _elf_info(new ELFInfo)
{
	if (bitness == 2) {
		if (endianess == 2) {
			FillELFInfo<ELF_EndianessReverse, Elf64_Ehdr, Elf64_Phdr, Elf64_Shdr>(*_elf_info, name);
		} else {
			FillELFInfo<ELF_EndianessSame, Elf64_Ehdr, Elf64_Phdr, Elf64_Shdr>(*_elf_info, name);
		}
	} else {
		if (endianess== 2) {
			FillELFInfo<ELF_EndianessReverse, Elf32_Ehdr, Elf32_Phdr, Elf32_Shdr>(*_elf_info, name);
		} else {
			FillELFInfo<ELF_EndianessSame, Elf32_Ehdr, Elf32_Phdr, Elf32_Shdr>(*_elf_info, name);
		}
	}
	fprintf(stderr, "ObjInfo::PluginImpl('%s', %d, %d)\n", name, bitness, endianess);
}

PluginImpl::~PluginImpl()
{
}

bool PluginImpl::GetFindData_ROOT(FP_SizeItemList &il, int OpMode)
{
	if (!AddUnsized(il, STR_DISASM, FILE_ATTRIBUTE_DIRECTORY))
		return false;
	if (!_elf_info->sections.empty() && !AddUnsized(il, STR_SECTIONS, FILE_ATTRIBUTE_DIRECTORY))
		return false;
	if (!_elf_info->phdrs.empty() && !AddUnsized(il, STR_PHDRS, FILE_ATTRIBUTE_DIRECTORY))
		return false;

	if (_elf_info->total_length > _elf_info->elf_length) {
		if (!AddBinFile(il, STR_TAIL, _elf_info->elf_length, _elf_info->total_length - _elf_info->elf_length))
			return false;
	}

	std::set<std::string> commands;
	Root::Commands(commands);
	for (const auto &command : commands) {
		if (!AddUnsized(il, command.c_str(), 0))
			return false;
	}

	return true;
}

bool PluginImpl::GetFindData_DISASM(FP_SizeItemList &il, int OpMode)
{
	std::set<std::string> commands;
	Disasm::Commands(_elf_info->machine, commands);
	for (const auto &command : commands) {
		if (!AddUnsized(il, command.c_str(), 0))
			return false;
	}

	return true;
}

static bool GetFindData_REGIONS(FP_SizeItemList &il, ELFInfo::Regions &rgns)
{
	for (const auto &r : rgns) {
		if (!AddBinFile(il, r.info.c_str(), r.begin, r.length))
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
	} else if (_dir == "/" STR_SECTIONS) {
		out = out && GetFindData_REGIONS(il, _elf_info->sections);
	} else if (_dir == "/" STR_PHDRS) {
		out = out && GetFindData_REGIONS(il, _elf_info->phdrs);
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
	std::string data_file;
	for (int i = 0; i < ItemsNumber; ++i) {
		data_file = DestPath;
		if (!data_file.empty() && data_file[data_file.size() - 1] != '/') {
			data_file+= '/';
		}
		data_file+= PanelItem[i].FindData.cFileName;
		if (IsBinFile(PanelItem[i].FindData.cFileName)) {
			unsigned long long ofs = 0, len = 0;
			sscanf(strrchr(PanelItem[i].FindData.cFileName, '@'), "@%llx.bin", &ofs);
			len = PanelItem[i].FindData.nFileSizeHigh;
			len<<= 32;
			len|= PanelItem[i].FindData.nFileSizeLow;
			Binary::Query(ofs, len, _name, data_file);

		} else if (_dir.empty()) {
			Root::Query(PanelItem[i].FindData.cFileName, _name, data_file);

		} else if (_dir == "/" STR_DISASM ) {
			Disasm::Query(_elf_info->machine, PanelItem[i].FindData.cFileName, _name, data_file);
		}
		fprintf(stderr, "ObjInfo::GetFiles[%i]: %s\n", i, PanelItem[i].FindData.cFileName);
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
	std::string data_file;
	for (int i = 0; i < ItemsNumber; ++i) {
		fprintf(stderr, "ObjInfo::PutFiles[%i]: %s\n", i, PanelItem[i].FindData.cFileName);
		if (IsBinFile(PanelItem[i].FindData.cFileName)) {
			out = FALSE;
			continue;
		}
		if (PanelItem[i].FindData.cFileName[0] != '/') {
			data_file = cd;
			data_file+= '/';
			data_file+= PanelItem[i].FindData.cFileName;
		} else
			data_file = PanelItem[i].FindData.cFileName;

		if (_dir.empty()) {
				Root::Store(PanelItem[i].FindData.cFileName, _name, data_file);

		} else if (_dir == "/" STR_DISASM ) {
				Disasm::Store(_elf_info->machine, PanelItem[i].FindData.cFileName, _name, data_file);
		}
	}

	return out;
}

int PluginImpl::DeleteFiles(struct PluginPanelItem *PanelItem, int ItemsNumber, int OpMode)
{
	if (ItemsNumber == 0 )
		return FALSE;

	for (int i = 0; i < ItemsNumber; ++i) {
		if (_dir.empty()) {
			Root::Clear(PanelItem[i].FindData.cFileName, _name);

		} else if (_dir == "/" STR_DISASM ) {
			Disasm::Clear(_elf_info->machine, PanelItem[i].FindData.cFileName, _name);
		}

		fprintf(stderr, "ObjInfo::DeleteFiles[%i]: %s\n", i, PanelItem[i].FindData.cFileName);
	}

	return TRUE;
}

int PluginImpl::ProcessKey(int Key, unsigned int ControlState)
{
	fprintf(stderr, "ObjInfo::ProcessKey\n");
	return FALSE;
}
