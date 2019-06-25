#include "OpGetMode.h"
#include "../UI/Activities/SimpleOperationProgress.h"

OpGetMode::OpGetMode(int op_mode, std::shared_ptr<IHost> &base_host, const std::string &path, std::shared_ptr<WhatOnErrorState> &wea_state)
	:
	OpBase(op_mode, base_host, path, wea_state)
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
		SimpleOperationProgress p(SimpleOperationProgress::K_GETMODE, _base_dir, _state);
		p.Show();
		WaitThread();
	}

	if (!_succeed)
		return false;

	result = _result;
	return true;
}


void OpGetMode::Process()
{
	WhatOnErrorWrap<WEK_QUERYINFO>(_wea_state, _state, _base_host.get(), _base_dir,
		[&] () mutable 
		{
			_result = _base_host->GetMode(_base_dir);
			_succeed = true;
		}
	);
}
