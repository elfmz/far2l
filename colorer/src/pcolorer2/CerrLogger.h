#ifndef SIMPLELOGGER_H
#define SIMPLELOGGER_H

#include <ctime>
#include <iostream>
#include <locale>

class CerrLogger : public Logger
{
 public:
  static const char *LogLevelStr[];

  CerrLogger();

  ~CerrLogger() override = default;

  void log(Logger::LogLevel level, const char* /*filename_in*/, int /*line_in*/,
           const char* /*funcname_in*/, const char* message) override;
  Logger::LogLevel getLogLevel(const std::string& log_level);

  void flush() override {};

 private:
  Logger::LogLevel current_level = Logger::LOG_OFF;
};

#endif  // SIMPLELOGGER_H
