#pragma once
#include <string>
#include "../../Host/HostLocal.h"
#include "../ProtocolInitDeinitCmd.h"

class ProtocolFile : public HostLocal
{
	std::unique_ptr<ProtocolInitDeinitCmd> _init_deinit_cmd;

public:
	ProtocolFile(const std::string &host, unsigned int port,
		const std::string &username, const std::string &password, const std::string &options);

	virtual ~ProtocolFile();
};
