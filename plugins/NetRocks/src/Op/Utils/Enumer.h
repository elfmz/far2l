#pragma once
#include <string>
#include <set>
#include <memory>
#include <farplug-wide.h>
#include "../../UI/Defs.h"
#include "../../UI/Activities/WhatOnError.h"
#include "../../FileInformation.h"
#include "../../Host/Host.h"

class Enumer
{
	Path2FileInformation &_result;
	std::shared_ptr<IHost> _host;
	std::set<std::string> _items;
	bool _no_special_files;
	ProgressState &_state;
	std::shared_ptr<WhatOnErrorState> _wea_state;
	unsigned int _scan_depth_limit = 0;

	void GetSubitems(const std::string &path, Path2FileInformation &subitems);
	void ScanItem(const std::string &path);
	bool OnScanningPath(const std::string &path, const FileInformation *file_info = nullptr);

public:
	Enumer(Path2FileInformation &result, std::shared_ptr<IHost> &host, const std::string &dir,
		const struct PluginPanelItem *items, int items_count, bool no_special_files,
		ProgressState &state, std::shared_ptr<WhatOnErrorState> &wea_state);

	inline const std::set<std::string> &Items() const { return _items; }
	inline std::set<std::string> &Items() { return _items; }

	void Scan(bool recurse = true);
};
