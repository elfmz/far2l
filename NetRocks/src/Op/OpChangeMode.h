#pragma once
#include "OpBase.h"
#include "./Utils/Enumer.h"

class OpChangeMode : protected OpBase
{
	virtual void Process();
	bool _recurse;
	mode_t _mode_set, _mode_clear;

	Path2FileInformation _entries;
	std::shared_ptr<Enumer> _enumer;

	void ChangeModeOfPath(const std::string &path, mode_t prev_mode);

public:
	OpChangeMode(std::shared_ptr<IHost> &base_host, const std::string &base_dir,
		struct PluginPanelItem *items, int items_count);

	void Do();
};
