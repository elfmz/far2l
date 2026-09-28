#include "OpMakeDirectory.h"
#include "../UI/Activities/Confirm.h"
#include "../UI/Activities/SimpleOperationProgress.h"

OpMakeDirectory::OpMakeDirectory(int op_mode,std::shared_ptr<IHost> &base_host,
	const std::string &base_dir, const std::string &dir_name)
	:
	OpBase(op_mode, base_host, base_dir),
	_dir_name(dir_name)
{
}

bool OpMakeDirectory::Do()
{
	if (_base_dir.empty()) {
		_base_dir = "./";

	} else if (_base_dir[_base_dir.size() - 1] != '/')
		_base_dir+= '/';

	if (!StartThread()) {
		return false;
	}
	if (!WaitThreadBeforeShowProgress()) {
		SimpleOperationProgress p(SimpleOperationProgress::K_CREATEDIR, _dir_name, _state);
		p.Show();
		WaitThread();
	}

	return true;
}


void OpMakeDirectory::Process()
{
	std::string full_path = _base_dir;
	full_path+= _dir_name;
	for (size_t i = _base_dir.size(); i <= full_path.size(); ++i) {
		if (i == full_path.size() || full_path[i] == '/') {
			const std::string &component = full_path.substr(0, i);
			WhatOnErrorWrap<WEK_MAKEDIR>(_wea_state, _state, _base_host.get(), _base_dir,
				[&] () mutable
				{
					_base_host->DirectoryCreate(component.c_str(), 0751);
				}
			);
		}
	}
}
