#ifndef _COLORER_COMMON_H_
#define _COLORER_COMMON_H_

/// system dependent byte
typedef unsigned char byte;

#include <memory>
#ifdef USESPDLOG
#include <spdlog/spdlog.h>
#endif
#include <colorer/common/Features.h>
#include <colorer/unicode/String.h>
#include <colorer/unicode/SString.h>
#include <colorer/unicode/CString.h>

typedef std::unique_ptr<String> UString;
typedef std::unique_ptr<SString> USString;

#ifdef USESPDLOG
extern std::shared_ptr<spdlog::logger> logger;
#else
class DummyLogger {
public:
  template<typename... Args> void debug(Args... args) {}
  template<typename... Args> void error(Args... args) {}
  template<typename... Args> void warn (Args... args) {}
  template<typename... Args> void trace(Args... args) {}
};
extern std::shared_ptr<DummyLogger> logger;
#endif

#endif

