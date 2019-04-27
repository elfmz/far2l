#include "OpEnumDirectory.h"
#include "../UI/Confirm.h"

OpEnumDirectory::OpEnumDirectory(std::shared_ptr<SiteConnection> &connection, int op_mode, const std::string &base_dir, FP_SizeItemList &result)
	:
	OpBase(connection, op_mode, base_dir),
	ProgressStateUpdaterCallback(_state),
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
		DirOperationProgress(DirOperationProgress::K_ENUMDIR, _base_dir, _state).Show();
		WaitThread();
	}

	return true;
}


void OpEnumDirectory::Process()
{
	_connection->DirectoryEnum(_base_dir, _result, _op_mode, this);
}
