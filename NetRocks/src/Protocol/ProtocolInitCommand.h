#pragma once
#include <string>
#include "StringConfig.h"

void ProtocolInitCommand(const std::string &host, unsigned int port,
	const std::string &username, const std::string &password, const StringConfig &protocol_options);
