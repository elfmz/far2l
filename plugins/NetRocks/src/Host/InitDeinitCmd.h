#pragma once
#include <string>
#include "StringConfig.h"

struct InitDeinitCmd
{
	static InitDeinitCmd *sMake(const std::string &proto, const std::string &host, unsigned int port,
		const std::string &username, const std::string &password, const StringConfig &protocol_options, std::atomic<bool> &aborted);

	virtual ~InitDeinitCmd();
};
