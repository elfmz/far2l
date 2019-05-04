#include "OpEnumDirectory.h"
#include "../UI/Confirm.h"
#include "../PooledStrings.h"

OpEnumDirectory::OpEnumDirectory(std::shared_ptr<IHost> &base_host, int op_mode, const std::string &base_dir, FP_SizeItemList &result)
	:
	OpBase(base_host, op_mode, base_dir),
	_result(result)
{
}

bool OpEnumDirectory::Do()
{
	if (!StartThread()) {
		return false;
	}

	if (IS_SILENT(_op_mode)) {
		WaitThread();

	} else if (!WaitThread(1000)) {
		SimpleOperationProgress(SimpleOperationProgress::K_ENUMDIR, _base_dir, _state).Show();
		WaitThread();
	}

	return true;
}


void OpEnumDirectory::Process()
{
	std::shared_ptr<IDirectoryEnumer> enumer = _base_host->DirectoryEnum(_base_dir);
	std::string name, owner, group;
	FileInformation file_info;
	for (;;) {
		if (!enumer->Enum(name, owner, group, file_info)) {
			break;
		}

		PluginPanelItem tmp = {};
		strncpy(tmp.FindData.cFileName, name.c_str(), sizeof(tmp.FindData.cFileName) - 1);
		tmp.FindData.nFileSizeLow = (DWORD)(file_info.size & 0xffffffff);
		tmp.FindData.nFileSizeHigh = (DWORD)(file_info.size >> 32);
		tmp.FindData.dwUnixMode = file_info.mode;
		tmp.FindData.dwFileAttributes = WINPORT(EvaluateAttributesA)(file_info.mode, name.c_str());
		tmp.Owner = (char *)PooledString(owner);
		tmp.Group = (char *)PooledString(group);
		if (!_result.Add(&tmp)) {
			throw std::runtime_error("our of memory");
		}

		ProgressStateUpdate psu(_state);
		_state.stats.count_complete++;
	}
}
