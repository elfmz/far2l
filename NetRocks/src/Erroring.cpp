#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include "Erroring.h"

unsigned char InitNetrocksVerbosity()
{
	const char *verbose_env = getenv("NETROCKS_VERBOSE");
	if (verbose_env) {
		switch (*verbose_env) {
			case 'y': case 'Y': case 't': case 'T': return 1;
			default:
				if (*verbose_env >= '1' && *verbose_env <= '9') {
					return 1 + (*verbose_env - '1');
				}
		}
	}

	return 0;
}

unsigned char g_netrocks_verbosity = InitNetrocksVerbosity();

void NetrocksLog(const int threshold, const char* file, const int line, const char* caller, const char* fmt, ...)
{
	if (g_netrocks_verbosity > threshold)
	{
		std::time_t now = std::time(nullptr);
		std::tm* timeinfo = std::localtime(&now);
		char timestamp[20];
		std::strftime(timestamp, sizeof(timestamp), "%Y-%m-%d %H:%M:%S", timeinfo);
		va_list args;
		va_start(args, fmt);
		std::fprintf(stderr, "%s [%s:%d %s] ", timestamp, file, line, caller);
		std::vfprintf(stderr, fmt, args);
		std::fprintf(stderr, "\n");
		va_end(args);
	}
}

static std::string FormatProtocolError(const char *msg, const char *info = nullptr, int err = 0)
{
	std::string s = msg;
	if (info) {
		s+= " - ";
		s+= info;
	}
	if (err) {
		char sz[64];
		snprintf(sz, sizeof(sz), " (%d)", err);
		s+= sz;
	}
	return s;
}

static const std::string &TeeStr(const std::string &s)
{
	fprintf(stderr, "%d: %s\n", getpid(), s.c_str());
	return s;
}

static const char *TeeSZ(const char *s)
{
	fprintf(stderr, "%d: %s\n", getpid(), s);
	return s;
}


ProtocolError::ProtocolError(const char *msg, const char *info, int err)
	: std::runtime_error(TeeStr(FormatProtocolError(msg, info, err)))
{
}

ProtocolError::ProtocolError(const char *msg, int err)
	: std::runtime_error(TeeStr(FormatProtocolError(msg, nullptr, err)))
{
}

ProtocolError::ProtocolError(const char *msg)
	: std::runtime_error(TeeSZ(msg))
{
}

ProtocolError::ProtocolError(const std::string &msg)
	: std::runtime_error(TeeStr(msg))
{
}

////////////

ProtocolAuthFailedError::ProtocolAuthFailedError(const std::string &info)
	: std::runtime_error(info)
{
}

////////////

ServerIdentityMismatchError::ServerIdentityMismatchError(const std::string &identity)
	: std::runtime_error(identity)
{
}

////////////
