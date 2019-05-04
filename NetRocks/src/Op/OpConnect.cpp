#include "OpConnect.h"
#include "Host/HostRemote.h"

OpConnect::OpConnect(int op_mode, const std::string &display_name)
	: OpBase(std::make_shared<HostRemote>(display_name, op_mode), op_mode, display_name)
{
}

std::shared_ptr<IHost> OpConnect::Do()
{
	_succeed = false;
//	fprintf(stderr, "Connect START\n");
	if (!StartThread()) {
		;

	} else if (IS_SILENT(_op_mode)) {
		WaitThread();

	} else if (!WaitThread(1000)) {
		SimpleOperationProgress(SimpleOperationProgress::K_CONNECT, _base_dir, _state).Show();
		WaitThread();
	}

//	fprintf(stderr, "Connect END\n");
	return _succeed ? _base_host : std::shared_ptr<IHost>();
}

void OpConnect::Process()
{
	_base_host->ReInitialize();
	_succeed = true;
//	fprintf(stderr, "Connect OK\n");
}
