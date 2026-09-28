#pragma once
#include <string>
#include <vector>

struct Location
{
	enum ServerKind
	{
		SK_URL,
		SK_SITE,
	} server_kind;

	struct URL {
		std::string protocol, host, username, password;
		unsigned int port;
	} url;

	std::string server; // site name or url in form of protocol:username@host:port

	struct Path {
		std::vector<std::string> components;
		bool absolute;

		void ResetToHome();
	} path;

	void PathFromString(const std::string &directory);
	bool FromString(const std::string &standalone_config, const std::string &str);
	std::string ToString(bool with_server) const;
};
