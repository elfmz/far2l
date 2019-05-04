#include "OpGetMode.h"
#include "../UI/Confirm.h"

OpGetMode::OpGetMode(std::shared_ptr<IHost> &base_host, int op_mode, const std::string &path)
	:
	OpBase(base_host, op_mode, path)
{
}

bool OpGetMode::Do(mode_t &result)
{
	_succeed = false;

	if (!StartThread()) {
		;

	} else if (IS_SILENT(_op_mode)) {
		WaitThread();

	} else if (!WaitThread(1000)) {
		SimpleOperationProgress(SimpleOperationProgress::K_GETMODE, _base_dir, _state).Show();
		WaitThread();
	}

	if (!_succeed)
		return false;

	result = _result;
	return true;
}


void OpGetMode::Process()
{
	_result = _base_host->GetMode(_base_dir);
	_succeed = true;
}
