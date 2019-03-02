#pragma once
#include <exception>
#include <stdexcept>
#include <string>

struct ProtocolError : std::runtime_error
{
	ProtocolError(const char *msg, const char *info, int err = 0);
	ProtocolError(const char *msg, int err);
	ProtocolError(const char *msg);
	ProtocolError(const std::string &msg);
};

struct IPCError : std::runtime_error
{
	IPCError(const char *msg, unsigned int code);
};

struct AbortError : std::runtime_error
{
	AbortError() : std::runtime_error("Operation aborted") {}
};
