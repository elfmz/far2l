#include "OpConnect.h"

OpConnect::OpConnect(int op_mode, const std::string &display_name)
	: OpBase(std::make_shared<SiteConnection>(display_name, op_mode), op_mode, display_name)
{
}

std::shared_ptr<SiteConnection> OpConnect::Do()
{
	_succeed = false;

	if (!StartThread()) {
		;

	} else if (IS_SILENT(_op_mode)) {
		WaitThread();

	} else if (!WaitThread(1000)) {
		SimpleOperationProgress(SimpleOperationProgress::K_CONNECT, _base_dir, _state).Show();
		WaitThread();
	}

	return _succeed ? _connection : std::shared_ptr<SiteConnection>();
}

void OpConnect::Process()
{
	_connection->ReInitialize();
	_succeed = true;
}
