#pragma once
#include <string>
#include <windows.h>
#include "../DialogUtils.h"
#include "../Defs.h"

class ConfirmOverwrite : protected BaseDialog
{
	int _i_source_size = -1, _i_source_timestamp = -1;
	int _i_destination_size = -1, _i_destination_timestamp = -1;
	int _i_destination = -1, _i_remember = -1, _i_overwrite = -1, _i_skip = -1;
	int _i_overwrite_newer = -1, _i_resume = -1, _i_create_diff_name = -1;

public:
	ConfirmOverwrite(XferKind xk, XferDirection xd, const std::string &destination,
		const timespec &src_ts, unsigned long long src_size, const timespec &dst_ts, unsigned long long dst_size);

	XferOverwriteAction Ask(XferOverwriteAction &default_xoa);
};


class ConfirmOverwriteState
{
public:

	XferOverwriteAction Query(XferKind xk, XferDirection xd, const std::string &destination,
		const timespec &src_ts, unsigned long long src_size, const timespec &dst_ts, unsigned long long dst_size);
};
