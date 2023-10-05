#pragma once
#include <vector>
#include <map>
#include <memory>
#include <all_far.h>
#include "PluginImpl.h"

struct ELFInfo;

class PluginImplELF : public PluginImpl
{
	std::shared_ptr<ELFInfo> _elf_info;

	bool GetFindData_ROOT(FP_SizeItemList &il, int OpMode);
	bool GetFindData_DISASM(FP_SizeItemList &il, int OpMode);

protected:
	virtual bool OnGetFindData(FP_SizeItemList &il, int OpMode);
	virtual bool OnGetFile(const char *item_file, const char *data_path, uint64_t len);
	virtual bool OnPutFile(const char *item_file, const char *data_path);
	virtual bool OnDeleteFile(const char *item_file);


public:
	PluginImplELF(const char *name, uint8_t bitness, uint8_t endianness);
};
