#include "OpMakeDirectory.h"
#include "../UI/Confirm.h"

OpMakeDirectory::OpMakeDirectory(std::shared_ptr<IHost> &base_host, int op_mode,
	const std::string &base_dir, const std::string &dir_name)
	:
	OpBase(base_host, op_mode, base_dir),
	_dir_name(dir_name)
{
}

bool OpMakeDirectory::Do()
{
	if (!IS_SILENT(_op_mode)) {
		_dir_name = ConfirmMakeDir(_dir_name.c_str()).Ask();
		if (_dir_name.empty()) {
			fprintf(stderr, "NetRocks::MakeDirectory: cancel\n");
			return false;
		}
	}

	if (_base_dir.empty()) {
		_base_dir = "./";

	} else if (_base_dir[_base_dir.size() - 1] != '.')
		_base_dir+= '/';

	if (!StartThread()) {
		return false;
	}
	if (!WaitThread(IS_SILENT(_op_mode) ? 2000 : 500)) {
		SimpleOperationProgress(SimpleOperationProgress::K_CREATEDIR, _dir_name, _state).Show();
		WaitThread();
	}

	return true;
}


void OpMakeDirectory::Process()
{
	std::string full_path = _base_dir;
	full_path+= _dir_name;
	for (size_t i = _base_dir.size();  i <= full_path.size(); ++i) {
		if (i == full_path.size() || full_path[i] == '/') {
			const std::string &component = full_path.substr(0, i);
			try {
				_base_host->DirectoryCreate(component.c_str(), 0751);

			} catch (std::exception &ex) {
				fprintf(stderr, "NetRocks::MakeDirectory: '%s' while creating '%s'\n", ex.what(), component.c_str());
			}
		}
	}
}
