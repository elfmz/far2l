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

				WINPORT(FileTime_UnixToWin32)(file_info.access_time, &ppi->FindData.ftLastAccessTime);
				WINPORT(FileTime_UnixToWin32)(file_info.modification_time, &ppi->FindData.ftLastWriteTime);
				if (file_info.status_change_time.tv_sec) { // libssh often returns zero attributes->createtime (their bug?)
					WINPORT(FileTime_UnixToWin32)(file_info.status_change_time, &ppi->FindData.ftCreationTime);
				} else {
					file_info.status_change_time = file_info.modification_time;
				}

				ProgressStateUpdate psu(_state);
				_state.stats.count_complete++;
			}
		}
		,
		[this] (bool &recovery) mutable
		{
			recovery = false;
			_result.Shrink(_initial_result_count);
			std::unique_lock<std::mutex> locker(_state.mtx);
			_state.stats.count_complete = _initial_count_complete;
		}
	);

	// Now for those of them which are symlinks check if they point to directory and
	// set FILE_ATTRIBUTE_DIRECTORY to tell far2l that they're 'enterable' directories
	// note that not care about possible faults for a reason to do not bother user with
	// annoying errors if some directories target's will appear inaccessible.
	std::vector<std::string> paths;
	std::vector<mode_t> modes;
	std::vector<DWORD *> pattrs;
	size_t paths_len = 0;
	for (int i = _initial_result_count; ; ++i) {
		if (paths_len >= 1024 || i >= _result.count) {
			if (!paths.empty()) {
				_base_host->GetModes(true, paths.size(), paths.data(), modes.data());
				for (size_t j = 0; j < paths.size(); ++j) {
					if (modes[j] != (mode_t)-1 && S_ISDIR(modes[j])) {
						(*pattrs[j])|= FILE_ATTRIBUTE_DIRECTORY;
					}
				}
				paths.clear();
				modes.clear();
				pattrs.clear();
				paths_len = 0;
			}
			if (i >= _result.count) break;
		}
		auto &entry = _result.items[i];
		if (S_ISLNK(entry.FindData.dwUnixMode)) {
			paths.emplace_back(_base_dir);
			if (!_base_dir.empty() && _base_dir.back() != '/') {
				paths.back()+= '/';
			}
			paths.back()+= Wide2MB(entry.FindData.lpwszFileName);
			paths_len+= paths.back().size();
			modes.emplace_back(~(mode_t)0);
			pattrs.emplace_back(&entry.FindData.dwFileAttributes);
		}
		ProgressStateUpdate psu(_state); // check for pause/abort
	}
}
