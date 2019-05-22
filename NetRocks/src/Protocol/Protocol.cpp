#include "Protocol.h"

#ifdef HAVE_SFTP
void ConfigureProtocolSFTP(std::string &options);
std::shared_ptr<IProtocol> CreateProtocolSFTP(const std::string &host, unsigned int port,
	const std::string &username, const std::string &password, const std::string &options) throw (std::runtime_error);
#endif

#ifdef HAVE_SMB
void ConfigureProtocolSMB(std::string &options) {};
std::shared_ptr<IProtocol> CreateProtocolSMB(const std::string &host, unsigned int port,
	const std::string &username, const std::string &password, const std::string &options) throw (std::runtime_error);
#endif

void ConfigureProtocolFile(std::string &options);
std::shared_ptr<IProtocol> CreateProtocolFile(const std::string &host, unsigned int port,
	const std::string &username, const std::string &password, const std::string &options) throw (std::runtime_error);

static ProtocolImplementation s_protocols[] = {
#ifdef HAVE_SFTP
	{ "sftp", 22, ConfigureProtocolSFTP, CreateProtocolSFTP},
#endif
#ifdef HAVE_SMB
	{ "smb", 445, ConfigureProtocolSMB, CreateProtocolSMB},
#endif
	{ "file", 0, ConfigureProtocolFile, CreateProtocolFile},
	{ nullptr, 0, nullptr, nullptr }
};

ProtocolImplementation *g_protocols = &s_protocols[0];
