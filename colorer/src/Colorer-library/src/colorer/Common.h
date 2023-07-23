#ifndef _COLORER_COMMON_H_
#define _COLORER_COMMON_H_

/// system dependent byte
typedef unsigned char byte;

#include <memory>
#ifndef NOSPDLOG
#include <spdlog/spdlog.h>
#endif
#include <colorer/common/Features.h>
#include <colorer/unicode/String.h>
#include <colorer/unicode/SString.h>
#include <colorer/unicode/CString.h>

typedef std::unique_ptr<String> UString;
typedef std::unique_ptr<SString> USString;

#ifndef NOSPDLOG
extern std::shared_ptr<spdlog::logger> logger;
#else
class DummyLogger {
public:
  void debug(const char* message) {}
  void debug(const char* message, int value) {}
  void debug(const char* message, size_t value) {}
  void debug(const char* message, const char* value) {}
  void debug(const char* message, const std::string& value) {}
  void debug(const char* message, const char* value1, const char* value2) {}
  void debug(const char* message, int value1, int value2, const char* value3) {}
  void debug(const std::string& message, int arg1, int arg2) {}
  void debug(const std::string& message, int arg1, int arg2, int arg3) {}
  void debug(const std::string& message, const char* arg1, const char* arg2, const char* arg3) {}

  void error(const char* message) {}
  void error(const char* message, const char* value) {}
  void error(const char* message, const char* value1, const char* value2) {}
  void error(const char* message, const std::string& file, XMLFileLoc line, XMLFileLoc column,
    const std::string& error_message) {}

  void warn(const char* message) {}
  void warn(const char* message, const char* value) {}
  void warn(const char* message, const std::string& value1, const std::string& value2) {}
  void warn(const char* message, const char* arg1, const char* arg2) {}
  void warn(const char* message, const std::string& file, XMLFileLoc line, XMLFileLoc column,
    const std::string& error_message) {}

  void trace(const char* message, size_t value) {}
};
extern std::shared_ptr<DummyLogger> logger;
#endif

#endif

