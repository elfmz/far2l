#pragma once
#include <string>
#include "../../Host/HostLocal.h"

class ProtocolFile : public HostLocal
{
public:
	ProtocolFile(const std::string &host, unsigned int port,
		const std::string &username, const std::string &password, const std::string &options);

	virtual ~ProtocolFile();
};
