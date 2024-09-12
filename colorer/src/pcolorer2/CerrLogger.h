#ifndef SIMPLELOGGER_H
#define SIMPLELOGGER_H

#include <ctime>
#include <iostream>
#include <locale>

class CerrLogger : public Logger
{
 public:
  static constexpr std::string_view LogLevelStr[] {"off",  "error", "warning",
                                                   "info", "debug", "trace"};

  CerrLogger()
  {
    const char* verbose_env = getenv("COLORER_VERBOSE");
    if (verbose_env) {
      current_level = getLogLevel(verbose_env);
    }
  }

  ~CerrLogger() override = default;

  void log(Logger::LogLevel level, const char* /*filename_in*/, int /*line_in*/,
           const char* /*funcname_in*/, const char* message) override
  {
    if (level > current_level) {
      return;
    }
    std::time_t const t = std::time(nullptr);
    char mbstr[30];
    std::strftime(mbstr, sizeof(mbstr), "%FT%T", std::localtime(&t));
    std::cerr << "[" << mbstr << "] [FarColorer] [" << LogLevelStr[level] << "] ";
    std::cerr << message << '\n';
  }

  Logger::LogLevel getLogLevel(const std::string& log_level)
  {
    int i = 0;
    for (auto it : LogLevelStr) {
      if (log_level == it) {
        return static_cast<Logger::LogLevel>(i);
      }
      i++;
    }
    if (log_level == "warn") {
      current_level = Logger::LOG_WARN;
    }
    return Logger::LOG_OFF;
  }

 private:
  Logger::LogLevel current_level = Logger::LOG_OFF;
};

#endif  // SIMPLELOGGER_H
