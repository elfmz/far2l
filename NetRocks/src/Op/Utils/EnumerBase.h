#pragma once
#pragma once
#include <string>
#include <map>
#include <set>
#include <all_far.h>
#include <fstdlib.h>

#include "../../UI/Defs.h"


class EnumerBase
{
	std::set<std::string> _items;

protected:
	ProgressState &_state;
	bool _no_special_files;
	unsigned int _scan_depth_limit;

	virtual void ScanItem(const std::string &path) = 0;
	virtual bool OnScanningPath(const std::string &path) = 0;

public:
	EnumerBase(ProgressState &state, const std::string &src_dir, const struct PluginPanelItem *items, int items_count, bool no_special_files);

	void Scan();
};
