#include "OpCheckDirectory.h"
#include "../UI/Activities/SimpleOperationProgress.h"

OpCheckDirectory::OpCheckDirectory(int op_mode, std::shared_ptr<IHost> &base_host, const std::string &path, std::shared_ptr<WhatOnErrorState> &wea_state)
	:
	OpBase(op_mode, base_host, path, wea_state),
	_final_path(path)
{
}

bool OpCheckDirectory::Do(std::string &final_path)
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

	final_path.swap(_final_path);
	return true;
}


void OpCheckDirectory::Process()
{
	WhatOnErrorWrap<WEK_CHECKDIR>(_wea_state, _state, _base_host.get(), _final_path,
		[&] () mutable
		{
			auto mode = _base_host->GetMode(_final_path);
			if (!S_ISDIR(mode)) {
				fprintf(stderr, "NetRocks: not dir mode=0%o: '%s'\n", mode, _final_path.c_str());
				throw std::runtime_error("Not a directory");
			}
			_succeed = true;
		}
		,
		[this] (bool &recovery) mutable
		{
			if (_op_mode != 0 || _final_path.empty()) {
				recovery = false;
			}

			if (recovery) {
				// Translate to 'upper' path, like:
				//   "/foo/bar" -> "/foo"
				//   "/foo" -> "/"
				//   "/" -> ""
				//   "foo/bar" -> "foo"
				//   "foo" -> ""
				size_t p = _final_path.rfind('/');
				if (p == 0) {
					_final_path.resize((_final_path.size() == 1) ? 0 : 1);

				} else if (p != std::string::npos) {
					_final_path.resize(p);

				} else {
					_final_path.clear();
				}
			}
		}
		,
		_op_mode == 0 && !_final_path.empty()
	);
}
