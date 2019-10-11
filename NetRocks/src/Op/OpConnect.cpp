#include "OpConnect.h"
#include "Host/HostRemote.h"
#include "../UI/Activities/SimpleOperationProgress.h"

static std::shared_ptr<IHost> CreateRemoteHost(const Location &location)
{
	switch (location.server_kind) {
		case Location::SK_CONNECTION:
			return std::make_shared<HostRemote>(location.server);

		case Location::SK_URL:
			return std::make_shared<HostRemote>(location.url.protocol, location.url.host,
				location.url.port, location.url.username, location.url.password);

		default:
			abort();
	}
}


OpConnect::OpConnect(int op_mode, const Location &location)
	: OpBase(op_mode, CreateRemoteHost(location), location.server)
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
		SimpleOperationProgress p(SimpleOperationProgress::K_CONNECT, _base_host->SiteName(), _state);//_base_dir
		p.Show();
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
