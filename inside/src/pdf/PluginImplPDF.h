#pragma once
#include <vector>
#include <map>
#include <memory>
#include <all_far.h>
#include <fstdlib.h>
#include "PluginImpl.h"

struct ELFInfo;

class PluginImplPDF : public PluginImpl
{
protected:
	virtual bool OnGetFindData(FP_SizeItemList &il, int OpMode);
	virtual bool OnGetFile(const char *item_file, const char *data_path, uint64_t len);
	virtual bool OnPutFile(const char *item_file, const char *data_path);
	virtual bool OnDeleteFile(const char *item_file);

public:
	PluginImplPDF(const char *name);
};
