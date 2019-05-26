#include <wchar.h>
#include <string>
#include <sudo.h>
#include <utils.h>

bool SplitPathSpecification(const wchar_t *specification, std::wstring &protocol, std::wstring &host, unsigned int &port,
				std::wstring &username, std::wstring &password, std::wstring &directory)
{
	/// protocol://username:password@host:port/dir
	//where username, password, port and dir are optional:
	// protocol://username@host:port/dir
	// protocol://host:port/dir
	// protocol://host/dir
	// protocol://host

	const wchar_t *proto_end = wcschr(specification, ':');
	if (!proto_end) {
		return false;
	}
	
	protocol.assign(specification, proto_end - specification);
	do { ++proto_end; } while (*proto_end == '/');
	if (wcscasecmp(protocol.c_str(), L"file") == 0) {
		char cur_dir[PATH_MAX + 1] = {};
		sdc_getcwd(cur_dir, PATH_MAX);

		if (proto_end[0] == 0 || (proto_end[0] == '.' && proto_end[1] == 0)) {
			directory = MB2Wide(cur_dir);

		} else if (proto_end[0] == '.' && proto_end[1] == '/') {
			directory = MB2Wide(cur_dir);
			directory+= &proto_end[1];
		} else {
			directory = L'/';
			directory+= proto_end;
		}
		return true;
	}

	const wchar_t *at = wcschr(proto_end, '@');
	if (at) {
		username.assign(proto_end, at - proto_end);
		size_t up_div = username.find(':');
		if (up_div!=std::wstring::npos) {
			password = username.substr(up_div);
			username.resize(up_div);
		}
		++at;
	} else {
		at = proto_end;
	}

	if (!*at) {
		return (wcscasecmp(protocol.c_str(), L"smb") == 0);
	}

	host = at;
	size_t spd_div = host.find('/');
	if (spd_div != std::wstring::npos) {
		directory = host.substr(spd_div);
		host.resize(spd_div);
	}

	size_t sp_div = host.find(':');
	if (sp_div != std::wstring::npos) {
		port = atoi(StrWide2MB(host.substr(sp_div + 1)).c_str());
		host.resize(sp_div);
	}

	fprintf(stderr, "SplitPathSpecification('%ls') -> '%ls' '%ls' '%ls' '%ls' %u '%ls'\n", specification,
		protocol.c_str(), username.c_str(), password.c_str(), host.c_str(), port, directory.c_str());
		
	return true;	
}

