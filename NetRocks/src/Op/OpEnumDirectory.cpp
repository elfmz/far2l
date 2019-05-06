#include "OpEnumDirectory.h"
#include "../UI/Confirm.h"
#include "../PooledStrings.h"

OpEnumDirectory::OpEnumDirectory(int op_mode, std::shared_ptr<IHost> &base_host, const std::string &base_dir, PluginPanelItems &result)
	:
	OpBase(op_mode, base_host, base_dir),
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

		auto *ppi = _result.Add(name.c_str());
		ppi->FindData.nFileSize = file_info.size;
		ppi->FindData.dwUnixMode = file_info.mode;
		ppi->FindData.dwFileAttributes = WINPORT(EvaluateAttributesA)(file_info.mode, name.c_str());
		ppi->Owner = (wchar_t *)MB2WidePooled(owner);
		ppi->Group = (wchar_t *)MB2WidePooled(group);

		ProgressStateUpdate psu(_state);
		_state.stats.count_complete++;
	}
}
