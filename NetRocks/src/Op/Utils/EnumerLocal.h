#pragma once
#include <string>
#include <map>
#include <set>
#include <all_far.h>
#include <fstdlib.h>

#include "EnumerBase.h"


typedef std::map<std::string, struct stat> LocalEntries;

class EnumerLocal : public EnumerBase
{
	LocalEntries &_entries;

	virtual void ScanItem(const std::string &path);
	virtual bool OnScanningPath(const std::string &path);

public:
	EnumerLocal(LocalEntries &entries, ProgressState &state, const std::string base_dir, const struct PluginPanelItem *items, int items_count, bool no_special_files);
};
