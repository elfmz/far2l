#include <utils.h>
#include "Location.h"
#include "SitesConfig.h"

bool SplitLocationSpecification(const char *specification,
	std::string &protocol, std::string &host, unsigned int &port,
	std::string &username, std::string &password, std::string &directory);


/*
Examples:
<StoredConnection>/etc/dir/file        - absolute path
<StoredConnection>/~/dir/in/home/file  - home-relative path
sftp://server/etc/dir/file             - absolute path
sftp://user@server:port/~/dir/in/home  - home-relative path
Legacy double-slash form (e.g. sftp://server//etc/...) is still accepted as absolute on read.
*/

bool Location::FromString(const std::string &standalone_config, const std::string &str)
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
	size_t lt_pos = str.find('<');
	if (lt_pos != std::string::npos) {
		server_kind = SK_SITE;

		size_t gt_pos = str.find('>', lt_pos);
		if (gt_pos == std::string::npos) {
			return false;
		}
		server = str.substr(lt_pos + 1, gt_pos - lt_pos - 1);
		if (server.empty()) {
			return false;
		}

		if (gt_pos == str.size() - 1) {
			SiteSpecification ss(standalone_config, server);
			directory = SitesConfig(ss.sites_cfg_location).GetDirectory(ss.site);

		} else if (gt_pos < str.size() + 2) {
			// keep the delimiter slash (see SplitLocationSpecification)
			directory = str.substr(gt_pos + 1);
		}

	} else {
		server_kind = SK_URL;

		if (!SplitLocationSpecification(str.c_str(), url.protocol,
			url.host, url.port, url.username, url.password, directory)) {
			return false;
		}

		server = url.protocol;
		server+= ':';
		if (!url.username.empty()) {
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
		PathFromString(directory);
	}

#if 0
	fprintf(stderr,
		"Location::FromString('%s'): server_kind=%d server='%s' directory='%s', result='%s'\n",
		str.c_str(), server_kind, server.c_str(), directory.c_str(), ToString(true).c_str());
#endif

	return true;
}

void Location::PathFromString(const std::string &directory)
{
	std::string dir = directory;

	// home-relative markers: "~", "~/...", "/~", "/~/..."
	bool home_relative = false;
	if (dir == "~" || (dir.size() >= 2 && dir[0] == '~' && dir[1] == '/')) {
		home_relative = true;
		dir.erase(0, 1); // drop '~', leaving "" or "/..."
	} else if (dir == "/~" || dir.compare(0, 3, "/~/") == 0) {
		home_relative = true;
		dir.erase(0, 2); // drop "/~", leaving "" or "/..."
	}

	// leading slash means absolute; StrExplode skips empty tokens so a legacy
	// double-slash ("//etc") collapses to an absolute path automatically
	path.absolute = !home_relative && !dir.empty() && dir[0] == '/';
	path.components.clear();
	StrExplode(path.components, dir, "/");
}

std::string Location::ToString(bool with_server) const
{
	std::string out;

	if (with_server) {
		if (server_kind == SK_SITE) {
			out+= '<';
		}
		out+= server;
		if (server_kind == SK_SITE) {
			out+= '>';
		}
	}

	if (path.absolute) {
		out+= '/';
	} else if (with_server) {
		// home-relative marker, only needed in the round-trippable URL form;
		// the protocol-facing form (with_server == false) stays a raw relative path
		out+= "/~";
		if (!path.components.empty()) {
			out+= '/';
		}
	}

	for (size_t i = 0; i < path.components.size(); ++i) {
		if (i != 0) {
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
