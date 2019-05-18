#include "Protocol.h"

void ConfigureProtocolSFTP(std::string &options);
std::shared_ptr<IProtocol> CreateProtocolSFTP(const std::string &host, unsigned int port,
	const std::string &username, const std::string &password, const std::string &options) throw (std::runtime_error);

void ConfigureProtocolFile(std::string &options);
std::shared_ptr<IProtocol> CreateProtocolFile(const std::string &host, unsigned int port,
	const std::string &username, const std::string &password, const std::string &options) throw (std::runtime_error);

static ProtocolImplementation s_protocols[] = {
	{ "sftp", 22, ConfigureProtocolSFTP, CreateProtocolSFTP},
	{ "file", 0, ConfigureProtocolFile, CreateProtocolFile},
	{ nullptr, 0, nullptr, nullptr }
};

ProtocolImplementation *g_protocols = &s_protocols[0];
