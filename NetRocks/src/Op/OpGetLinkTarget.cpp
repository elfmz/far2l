#include "OpGetLinkTarget.h"
#include <utils.h>
#include "../UI/Activities/ConfirmChangeMode.h"
#include "../UI/Activities/SimpleOperationProgress.h"

OpGetLinkTarget::OpGetLinkTarget(int op_mode, std::shared_ptr<IHost> &base_host, const std::string &base_dir, struct PluginPanelItem *item)
	:
	OpBase(op_mode, base_host, base_dir)
{
	_path = base_dir;
	EnsureSlashAtEnd(_path);
	Wide2MB(item->FindData.lpwszFileName, _path, true);
}

bool OpGetLinkTarget::Do(std::wstring &result)
{
	DoInner();
	StrMB2Wide(_result, result);
	return _success;
}

bool OpGetLinkTarget::Do(std::string &result)
{
	DoInner();
	result = _result;
	return _success;
}

void OpGetLinkTarget::DoInner()
{
	if (!StartThread()) {
		;
	} else if (IS_SILENT(_op_mode)) {
		WaitThread();
	} else if (!WaitThread(1000)) {
		SimpleOperationProgress p(SimpleOperationProgress::K_GETLINK, _base_dir, _state);
		p.Show();
		WaitThread();
	}
}

void OpGetLinkTarget::Process()
{
	try {
		_base_host->SymlinkQuery(_path, _result);
		_success = true;
	} catch (std::exception &e) {
		fprintf(stderr, "OpGetLinkTarget: '%s' for '%s'\n", e.what(), _path.c_str());
	} catch (...) {
	}
}
