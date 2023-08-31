#pragma once
#include <string>
#include "StringConfig.h"

struct ProtocolInitDeinitCmd
{
	static ProtocolInitDeinitCmd *Make(const char *proto, const std::string &host, unsigned int port,
		const std::string &username, const std::string &password, const StringConfig &protocol_options);

	virtual ~ProtocolInitDeinitCmd();
};
