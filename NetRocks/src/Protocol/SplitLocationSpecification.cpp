#include <string.h>
#include <string>
#include <stdlib.h>
#include <sudo.h>
#include <limits.h>
#include <utils.h>
#include "Protocol.h"

bool SplitLocationSpecification(const char *specification, std::string &protocol, std::string &host, unsigned int &port,
				std::string &username, std::string &password, std::string &directory)
{
	/// protocol://username:password@host:port/dir
	//where username, password, port and dir are optional:
	// protocol://username@host:port/dir
	// protocol://host:port/dir
	// protocol://host/dir
	// protocol://host

	const char *proto_end = strchr(specification, ':');
	if (!proto_end) {
		return false;
	}

	protocol.assign(specification, proto_end - specification);
	do { ++proto_end; } while (*proto_end == '/');
	if (strcasecmp(protocol.c_str(), "file") == 0) {
		char cur_dir[PATH_MAX + 1] = {};
		sdc_getcwd(cur_dir, PATH_MAX);

		if (proto_end[0] == 0 || (proto_end[0] == '.' && proto_end[1] == 0)) {
			directory = cur_dir;

		} else if (proto_end[0] == '.' && proto_end[1] == '/') {
			directory = cur_dir;
			directory+= &proto_end[1];
		} else {
			directory = '/';
			directory+= proto_end;
		}
		return true;
	}

	const char *at = strchr(proto_end, '@');
	if (at) {
		username.assign(proto_end, at - proto_end);
		size_t up_div = username.find(':');
		if (up_div!=std::string::npos) {
			password = username.substr(up_div);
			username.resize(up_div);
		}
		++at;
	} else {
		at = proto_end;
	}

	if (!*at) {
		auto pi = ProtocolInfoLookup(protocol.c_str());
		return (pi && !pi->require_server);
	}

	host = at;
	size_t spd_div = host.find('/');
	if (spd_div != std::string::npos) {
		directory = host.substr(spd_div + 1);
		host.resize(spd_div);
	}

	size_t sp_div = host.find(':');
	if (sp_div != std::string::npos) {
		port = atoi(host.substr(sp_div + 1).c_str());
		host.resize(sp_div);
	}

	//fprintf(stderr, "SplitLocationSpecification('%s') -> '%s' '%s' '%s' '%s' %u '%s'\n", specification,
	//	protocol.c_str(), username.c_str(), password.c_str(), host.c_str(), port, directory.c_str());

	return true;
}

