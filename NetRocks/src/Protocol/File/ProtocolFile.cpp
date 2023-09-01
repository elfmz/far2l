#include <string.h>
#include <StringConfig.h>
#include "ProtocolFile.h"


std::shared_ptr<IProtocol> CreateProtocol(const std::string &protocol, const std::string &host, unsigned int port,
	const std::string &username, const std::string &password, const std::string &options)
{
	return std::make_shared<ProtocolFile>(host, port, username, password, options);
}

ProtocolFile::ProtocolFile(const std::string &host, unsigned int port,
	const std::string &username, const std::string &password, const std::string &options)
{
	StringConfig protocol_options(options);
	_init_deinit_cmd.reset(ProtocolInitDeinitCmd::Make("file", host, port, username, password, protocol_options));
}

ProtocolFile::~ProtocolFile()
{
}

