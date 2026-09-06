#include "Globals.h"
#include "PluginImplELF.h"
#include "Dumper.h"
#include "ELFInfo.h"
#include <utils.h>


#define STR_DISASM		"Disassembly"
#define STR_SECTIONS	"Sections"
#define STR_PHDRS		"ProgramHeaders"
#define STR_TAIL		"TailData"

static bool AddBinFile(FP_SizeItemList &il, const char *prefix, uint64_t ofs, uint64_t len)
{
	PluginPanelItem tmp;
	ZeroFill(tmp); // ensure zeroed padding after tmp.FindData.cFileName
	snprintf(tmp.FindData.cFileName, sizeof(tmp.FindData.cFileName) - 1,
		"%s@%llx.bin", prefix, (unsigned long long)ofs);
	tmp.FindData.nFileSize = len;
	tmp.Description = tmp.FindData.cFileName;
	return il.Add(&tmp);
}

static bool IsBinFile(const char *name)
{
	size_t l = strlen(name);
	return (l > 5 && strcmp(name + l - 4, ".bin") == 0 && strchr(name, '@') != nullptr);
}


PluginImplELF::PluginImplELF(const char *name, uint8_t bitness, uint8_t endianness)
	: PluginImpl(name), _elf_info(new ELFInfo)
{
	if (bitness == 2) {
		if (endianness == 2) {
			FillELFInfo<ELF_EndiannessBig, Elf64_Ehdr, Elf64_Phdr, Elf64_Shdr>(*_elf_info, name);
		} else {
			FillELFInfo<ELF_EndiannessLittle, Elf64_Ehdr, Elf64_Phdr, Elf64_Shdr>(*_elf_info, name);
		}
	} else {
		if (endianness== 2) {
			FillELFInfo<ELF_EndiannessBig, Elf32_Ehdr, Elf32_Phdr, Elf32_Shdr>(*_elf_info, name);
		} else {
			FillELFInfo<ELF_EndiannessLittle, Elf32_Ehdr, Elf32_Phdr, Elf32_Shdr>(*_elf_info, name);
		}
	}
	fprintf(stderr, "Inside::PluginImplELF('%s', %d, %d)\n", name, bitness, endianness);
}

bool PluginImplELF::GetFindData_ROOT(FP_SizeItemList &il, int OpMode)
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

bool PluginImplELF::GetFindData_DISASM(FP_SizeItemList &il, int OpMode)
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


bool PluginImplELF::OnGetFindData(FP_SizeItemList &il, int OpMode)
{
	if (_dir.empty())
		return GetFindData_ROOT(il, OpMode);

	if (_dir == "/" STR_DISASM)
		return GetFindData_DISASM(il, OpMode);

	if (_dir == "/" STR_SECTIONS)
		return GetFindData_REGIONS(il, _elf_info->sections);

	if (_dir == "/" STR_PHDRS)
		return GetFindData_REGIONS(il, _elf_info->phdrs);

	return false;
}

bool PluginImplELF::OnGetFile(const char *item_file, const char *data_path, uint64_t len)
{
	if (IsBinFile(item_file)) {
		unsigned long long ofs = 0;
		sscanf(strrchr(item_file, '@'), "@%llx.bin", &ofs);
		Binary::Query(ofs, len, _name, data_path);
		return true;
	}

	if (_dir.empty()) {
		Root::Query(item_file, _name, data_path);
		return true;
	}

	if (_dir == "/" STR_DISASM ) {
		Disasm::Query(_elf_info->machine, item_file, _name, data_path);
		return true;
	}

	return false;
}


bool PluginImplELF::OnPutFile(const char *item_file, const char *data_path)
{
	if (IsBinFile(item_file))
		return false;

	if (_dir.empty()) {
		Root::Store(item_file, _name, data_path);
		return true;
	}

	if (_dir == "/" STR_DISASM ) {
		Disasm::Store(_elf_info->machine, item_file, _name, data_path);
		return true;
	}

	return false;
}

bool PluginImplELF::OnDeleteFile(const char *item_file)
{
	if (_dir.empty()) {
		Root::Clear(item_file, _name);
		return true;
	}

	if (_dir == "/" STR_DISASM ) {
		Disasm::Clear(_elf_info->machine, item_file, _name);
		return true;
	}

	return false;
}
