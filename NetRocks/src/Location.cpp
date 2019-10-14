#include <utils.h>
#include "Location.h"
#include "SitesConfig.h"

bool SplitLocationSpecification(const char *specification,
	std::string &protocol, std::string &host, unsigned int &port,
	std::string &username, std::string &password, std::string &directory);


/*
Examples:
<StoredConnection>//etc/dir/file
<StoredConnection>/dir/in/home/file
sftp://server//etc/dir/file
sftp://user@server:port/dir/in/home/file
*/

bool Location::FromString(const std::string &str)
{
	server.clear();

	url.protocol.clear();
	url.host.clear();
	url.username.clear();
	url.password.clear();
	url.port = 0;

	path.components.clear();
	path.absolute = false;

	std::string directory;

	if (str.size() > 2 && str[0] == '<') {
		server_kind = SK_CONNECTION;

		size_t p = str.find('>', 1);
		if (p == std::string::npos) {
			return false;
		}
		server = str.substr(1, p - 1);
		if (server.empty()) {
			return false;
		}
		if (p == str.size() - 1) {
			directory = SitesConfig().GetDirectory(server);

		} else if (p < str.size() + 2) {
			directory = str.substr(p + 2);
		}

	} else {
		server_kind = SK_URL;

		if (!SplitLocationSpecification(str.c_str(), url.protocol,
			url.host, url.port, url.username, url.password, directory)) {
			return false;
		}

		server = url.protocol;
		server+= ':';
		if (!url.username.empty())  {
			server+= url.username;
			server+= '@';
		}
		server+= url.host;
		if (url.port) {
			server+= StrPrintf(":%u", url.port);
		}
	}

	if (directory.empty()) {
		path.absolute = !str.empty() && str[str.size() - 1] == '/';

	} else {
		path.absolute = directory[0] == '/';
		StrExplode(path.components, directory, "/");
	}

#if 0
	fprintf(stderr,
		"Location::FromString('%s'): server_kind=%d server='%s' directory='%s', result='%s'\n",
		str.c_str(), server_kind, server.c_str(), directory.c_str(), ToString(true).c_str());
#endif

	return true;
}

std::string Location::ToString(bool with_server) const
{
	std::string out;

	if (with_server) {
		if (server_kind == SK_CONNECTION) {
			out+= '<';
		}
		out+= server;
		if (server_kind == SK_CONNECTION) {
			out+= '>';
		}
	}

	if (path.absolute) {
		out+= '/';
	}

	for (size_t i = 0; i < path.components.size(); ++i) {
		if (i != 0 || with_server) {
			out+= '/';
		}
		out+= path.components[i];
	}

	return out;
}

void Location::Path::ResetToHome()
{
	components.clear();
	absolute = false;
}
