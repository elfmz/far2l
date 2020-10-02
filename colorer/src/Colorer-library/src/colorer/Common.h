#ifndef _COLORER_COMMON_H_
#define _COLORER_COMMON_H_

/// system dependent byte
typedef unsigned char byte;

#include <memory>
#include <spdlog/spdlog.h>
#include <colorer/common/Features.h>
#include <colorer/unicode/String.h>
#include <colorer/unicode/SString.h>
#include <colorer/unicode/CString.h>

typedef std::unique_ptr<String> UString;
typedef std::unique_ptr<SString> USString;

extern std::shared_ptr<spdlog::logger> logger;

#endif

