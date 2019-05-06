#pragma once
#include "OpBase.h"
#include "./Utils/Enumer.h"
#include "../UI/Defs.h"

class OpXfer : protected OpBase
{
	Path2FileInformation _entries;
	std::shared_ptr<Enumer> _enumer;
	std::shared_ptr<IHost> _dst_host;
	std::string _dst_dir, _diffname_suffix;
	XferOverwriteAction _default_xoa = XOA_ASK;
	WhatOnErrorAction _default_wea = WEA_ASK;
	XferKind _kind;
	XferDirection _direction;

	virtual void Process();

	virtual void ForcefullyAbort();	// IAbortableOperationsHost

	void Transfer();
	void FileCopyLoop(const std::string &path_src, const std::string &path_dst, unsigned long long pos, mode_t mode);

public:
	OpXfer(int op_mode, std::shared_ptr<IHost> &base_host, const std::string &base_dir,
		std::shared_ptr<IHost> &dst_host, const std::string &dst_dir, struct PluginPanelItem *items,
		int items_count, XferKind kind, XferDirection direction);
	bool Do();
};
