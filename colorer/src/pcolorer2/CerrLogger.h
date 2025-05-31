#ifndef SIMPLELOGGER_H
#define SIMPLELOGGER_H

#include <colorer/common/Logger.h>

class CerrLogger : public Logger
{
 public:
  static const char* LogLevelStr[];

  CerrLogger();

  ~CerrLogger() override = default;

  void log(Logger::LogLevel level, const char* /*filename_in*/, int /*line_in*/,
           const char* /*funcname_in*/, const char* message) override;
  Logger::LogLevel getLogLevel(std::string_view log_level);

  void flush() override {
    // no need, because we are working with the console.
  };

 private:
  Logger::LogLevel current_level = Logger::LOG_OFF;
};

#endif  // SIMPLELOGGER_H
