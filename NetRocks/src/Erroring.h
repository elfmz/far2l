#pragma once
#include <exception>
#include <stdexcept>
#include <string>
#include <cstdarg>
#include <cstdio>
#include <ctime>


extern unsigned char g_netrocks_verbosity;

__attribute__((format(printf, 5, 6)))
void NetrocksLog(const int threshold, const char* file, const int line, const char* caller, const char* fmt, ...);

#define NR_LOG(threshold, fmt, ...) do { \
	constexpr const char* file = __FILE__; \
	const char* short_file = file; \
	for (const char* p = file; *p; ++p) if (*p == '/' || *p == '\\') short_file = p + 1; \
	NetrocksLog(threshold, short_file, __LINE__, __func__, fmt, ##__VA_ARGS__); \
} while(0)

#define NR_ERR(fmt, ...)  NR_LOG(0, fmt, ##__VA_ARGS__)
#define NR_WARN(fmt, ...) NR_LOG(1, fmt, ##__VA_ARGS__)
#define NR_DBG(fmt, ...)  NR_LOG(2, fmt, ##__VA_ARGS__)
#define NR_VDBG(fmt, ...) NR_LOG(3, fmt, ##__VA_ARGS__)


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

