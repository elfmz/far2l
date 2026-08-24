#pragma once
#include "../../FileInformation.h"

bool ParseMLsxLine(const char *line, const char *end,
		FileInformation &file_info, uid_t *uid = nullptr, gid_t *gid = nullptr,
		std::string *name = nullptr, std::string *lnkto = nullptr);
