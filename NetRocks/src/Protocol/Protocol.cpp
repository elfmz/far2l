#include <string.h>
#include "Protocol.h"

#ifdef HAVE_SFTP
void ConfigureProtocolSFTP(std::string &options);
std::shared_ptr<IProtocol> CreateProtocolSFTP(const std::string &host, unsigned int port,
	const std::string &username, const std::string &password, const std::string &options) throw (std::runtime_error);
#endif

#ifdef HAVE_SMB
void ConfigureProtocolSMB(std::string &options);
std::shared_ptr<IProtocol> CreateProtocolSMB(const std::string &host, unsigned int port,
	const std::string &username, const std::string &password, const std::string &options) throw (std::runtime_error);
#endif

#ifdef HAVE_NFS
void ConfigureProtocolNFS(std::string &options);
std::shared_ptr<IProtocol> CreateProtocolNFS(const std::string &host, unsigned int port,
	const std::string &username, const std::string &password, const std::string &options) throw (std::runtime_error);
#endif


#ifdef HAVE_WEBDAV
void ConfigureProtocolDav(std::string &options) {};
void ConfigureProtocolDavS(std::string &options) {};
std::shared_ptr<IProtocol> CreateProtocolDav(const std::string &host, unsigned int port,
	const std::string &username, const std::string &password, const std::string &options) throw (std::runtime_error);
std::shared_ptr<IProtocol> CreateProtocolDavS(const std::string &host, unsigned int port,
	const std::string &username, const std::string &password, const std::string &options) throw (std::runtime_error);
#endif


void ConfigureProtocolFile(std::string &options);
std::shared_ptr<IProtocol> CreateProtocolFile(const std::string &host, unsigned int port,
	const std::string &username, const std::string &password, const std::string &options) throw (std::runtime_error);

static ProtocolInfo s_protocols[] = {
#ifdef HAVE_SFTP
	{ "sftp", 22, true, true, ConfigureProtocolSFTP, CreateProtocolSFTP},
#endif
#ifdef HAVE_SMB
	{ "smb", -1, false, true, ConfigureProtocolSMB, CreateProtocolSMB},
#endif
#ifdef HAVE_NFS
	{ "nfs", -1, false, false, ConfigureProtocolNFS, CreateProtocolNFS},
#endif
#ifdef HAVE_WEBDAV
	{ "dav", 80, true, true, ConfigureProtocolDav, CreateProtocolDav},
	{ "davs", 443, true, true, ConfigureProtocolDavS, CreateProtocolDavS},
#endif
	{ "file", 0, false, true, ConfigureProtocolFile, CreateProtocolFile},
	{ }
};

const ProtocolInfo *ProtocolInfoHead()
{
	return &s_protocols[0];
}

const ProtocolInfo *ProtocolInfoLookup(const char *name)
{
	for (const ProtocolInfo *pi = ProtocolInfoHead(); pi->name; ++pi) {
		if (strcasecmp(name, pi->name) == 0) {
			return pi;
		}
	}

	return nullptr;
}
