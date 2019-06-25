#include <utils.h>
#include "OpEnumDirectory.h"
#include "../UI/Activities/Confirm.h"
#include "../UI/Activities/SimpleOperationProgress.h"
#include "../PooledStrings.h"

OpEnumDirectory::OpEnumDirectory(int op_mode, std::shared_ptr<IHost> &base_host, const std::string &base_dir, PluginPanelItems &result, std::shared_ptr<WhatOnErrorState> &wea_state)
	:
	OpBase(op_mode, base_host, base_dir, wea_state),
	_result(result)
{
	_initial_result_count = _result.count;
	std::unique_lock<std::mutex> locker(_state.mtx);
	_initial_count_complete = _state.stats.count_complete;
}

bool OpEnumDirectory::Do()
{
	if (!StartThread()) {
		return false;
	}

	if (IS_SILENT(_op_mode)) {
		WaitThread();

	} else if (!WaitThread(1000)) {
		SimpleOperationProgress p(SimpleOperationProgress::K_ENUMDIR, _base_dir, _state);
		p.Show();
		WaitThread();
	}

	return true;
}


void OpEnumDirectory::Process()
{
	WhatOnErrorWrap<WEK_ENUMDIR>(_wea_state, _state, _base_host.get(), _base_dir,
		[&] () mutable
		{
			std::shared_ptr<IDirectoryEnumer> enumer = _base_host->DirectoryEnum(_base_dir);
			std::string name, owner, group;
			FileInformation file_info;
			for (;;) {
				if (!enumer->Enum(name, owner, group, file_info)) {
					break;
				}

				auto *ppi = _result.Add(name.c_str());
				ppi->FindData.nFileSize = file_info.size;
				ppi->FindData.dwUnixMode = file_info.mode;
				ppi->FindData.dwFileAttributes = WINPORT(EvaluateAttributesA)(file_info.mode, name.c_str());
				ppi->Owner = (wchar_t *)MB2WidePooled(owner);
				ppi->Group = (wchar_t *)MB2WidePooled(group);

				WINPORT(FileTime_UnixToWin32)(file_info.access_time, &ppi->FindData.ftCreationTime);
				WINPORT(FileTime_UnixToWin32)(file_info.access_time, &ppi->FindData.ftLastAccessTime);
				WINPORT(FileTime_UnixToWin32)(file_info.modification_time, &ppi->FindData.ftLastWriteTime);

				ProgressStateUpdate psu(_state);
				_state.stats.count_complete++;
			}
		}
		,
		[this] () mutable 
		{
			_result.Shrink(_initial_result_count);
			std::unique_lock<std::mutex> locker(_state.mtx);
			_state.stats.count_complete = _initial_count_complete;
		}
	);

	// Now for those of them which are symlinks check if they point to directory and
	// set FILE_ATTRIBUTE_DIRECTORY to tell far2l that they're 'enterable' directories
	// note that not using WhatOnErrorWrap for a reason to do not bother user with
	// annoying errors if some directories target's will appear inaccessble.
	std::string path_name;
	for (int i = _initial_result_count; i < _result.count; ++i) {
		auto &entry = _result.items[i];
		if (S_ISLNK(entry.FindData.dwUnixMode)) try {
			path_name = _base_dir;
			if (!path_name.empty() && path_name[path_name.size() - 1] != '/') {
				path_name+= '/';
			}
			path_name+= Wide2MB(entry.FindData.lpwszFileName);
			mode_t mode = _base_host->GetMode(path_name);
			if (S_ISDIR(mode)) {
				entry.FindData.dwFileAttributes|= FILE_ATTRIBUTE_DIRECTORY;
			}
		} catch (std::exception &ex) {
			fprintf(stderr,
				"NetRocks::OpEnumDirectory: '%s' at target of '%s'\n",
				ex.what(), path_name.c_str());
		}

		ProgressStateUpdate psu(_state); // check for pause/abort
	}


}
