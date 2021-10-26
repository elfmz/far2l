#pragma once
#include <exception>
#include <stdexcept>
#include <string>


extern unsigned char g_netrocks_verbosity;

struct ProtocolError : std::runtime_error
{
	ProtocolError(const char *msg, const char *info, int err = 0);
	ProtocolError(const char *msg, int err);
	ProtocolError(const char *msg);
	ProtocolError(const std::string &msg);
};

struct ProtocolAuthFailedError : std::runtime_error
{
	ProtocolAuthFailedError(const std::string &info = std::string());
};


struct ProtocolUnsupportedError : ProtocolError
{
	ProtocolUnsupportedError(const std::string &msg) : ProtocolError(msg) {}
};

struct ServerIdentityMismatchError : std::runtime_error
{
	ServerIdentityMismatchError(const std::string &identity);
};

struct AbortError : std::runtime_error
{
	AbortError() : std::runtime_error("Operation aborted") {}
};

