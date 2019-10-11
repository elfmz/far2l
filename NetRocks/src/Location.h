#pragma once
#include <string>
#include <vector>

struct Location
{
	enum ServerKind
	{
		SK_URL,
		SK_CONNECTION,
	} server_kind;

	struct URL {
		std::string protocol, host, username, password;
		unsigned int port;
	} url;

	std::string server; // connection name or url in form of protocol:username@host:port

	struct Path {
		std::vector<std::string> components;
		bool absolute;

		void ResetToHome();
	} path;

	bool FromString(const std::string &str);
	std::string ToString(bool with_server) const;
};
