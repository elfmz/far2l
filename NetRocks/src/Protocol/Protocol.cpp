#include <string.h>
#include "Protocol.h"

void ConfigureProtocolSFTP(std::string &options);
void ConfigureProtocolSCP(std::string &options);
void ConfigureProtocolFTP(std::string &options);
void ConfigureProtocolFTPS(std::string &options);
void ConfigureProtocolSMB(std::string &options);
void ConfigureProtocolNFS(std::string &options);
void ConfigureProtocolWebDAV(std::string &options);
void ConfigureProtocolWebDAVs(std::string &options);
void ConfigureProtocolFile(std::string &options);
void ConfigureProtocolSHELL(std::string &options);

static ProtocolInfo s_protocols[] = {
#ifdef HAVE_SFTP
	{ "sftp", "NetRocks-SFTP", 22, true, true, true, ConfigureProtocolSFTP},
	{ "scp", "NetRocks-SFTP", 22, true, true, true, ConfigureProtocolSCP},
#endif
	{ "shell", "NetRocks-SHELL", 22, true, true, true, ConfigureProtocolSHELL},

	{ "ftp", "NetRocks-FTP", 21, true, true, true, ConfigureProtocolFTP},
#ifdef HAVE_OPENSSL
	{ "ftps", "NetRocks-FTP", 990, true, true, true, ConfigureProtocolFTPS},
#endif

#ifdef HAVE_SMB
	{ "smb", "NetRocks-SMB", -1, false, true, false, ConfigureProtocolSMB},
#endif

#ifdef HAVE_NFS
	{ "nfs", "NetRocks-NFS", -1, false, false, false, ConfigureProtocolNFS},
#endif

#ifdef HAVE_WEBDAV
	{ "dav", "NetRocks-WebDAV", 80, true, true, true, ConfigureProtocolWebDAV},
	{ "davs", "NetRocks-WebDAV", 443, true, true, true, ConfigureProtocolWebDAVs},
#endif

	{ "file", "NetRocks-FILE", 0, false, true, false, nullptr},
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

