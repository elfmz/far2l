#pragma once
#include <string>
#include <windows.h>
#include "../DialogUtils.h"

class ProtocolOptionsSFTP : protected BaseDialog
{
	int _i_ok = -1, _i_cancel = -1;
	int _i_max_io_block_size = -1;
	int _i_tcp_nodelay = -1;
	//int _i_enable_sandbox = -1;

public:
	ProtocolOptionsSFTP();

	void Ask(std::string &options);
};
