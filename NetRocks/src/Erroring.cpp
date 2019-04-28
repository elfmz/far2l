#include <stdlib.h>
#include "Erroring.h"

static std::string FormatProtocolError(const char *msg, const char *info = nullptr, int err = 0)
{
	std::string s = msg;
	if (info) {
		s+= ": ";
		s+= info;
	}
	if (err) {
		char sz[64];
		snprintf(sz, sizeof(sz), " (%d)", err);
		s+= sz;
	}
	return s;
}

ProtocolError::ProtocolError(const char *msg, const char *info, int err)
	: std::runtime_error(FormatProtocolError(msg, info, err))
{
}

ProtocolError::ProtocolError(const char *msg, int err)
	: std::runtime_error(FormatProtocolError(msg, nullptr, err))
{
}

ProtocolError::ProtocolError(const char *msg)
	: std::runtime_error(msg)
{
}

ProtocolError::ProtocolError(const std::string &msg)
	: std::runtime_error(msg)
{
}
////////////
ProtocolAuthFailedError::ProtocolAuthFailedError()
	: ProtocolError("Authorization failed")
{
}
////////////
static std::string FormatIPCError(const char *msg, unsigned int code)
{
	std::string s = msg;
	char sz[32];
	snprintf(sz, sizeof(sz) - 1, " (0x%x)", code);
	s+= sz;
	return s;
}

IPCError::IPCError(const char *msg, unsigned int code)
	: std::runtime_error(FormatIPCError(msg, code))
{
}
