#include <string.h>
#include <StringConfig.h>
#include "ProtocolFile.h"


std::shared_ptr<IProtocol> CreateProtocol(const std::string &protocol, const std::string &host, unsigned int port,
	const std::string &username, const std::string &password, const std::string &options, int fd_ipc_recv)
{
	return std::make_shared<ProtocolFile>(host, port, username, password, options);
}

ProtocolFile::ProtocolFile(const std::string &host, unsigned int port,
	const std::string &username, const std::string &password, const std::string &options)
{
}

ProtocolFile::~ProtocolFile()
{
}

