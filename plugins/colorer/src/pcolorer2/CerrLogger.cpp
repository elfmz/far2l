#include "CerrLogger.h"
#include <iostream>
#include <locale>
#include "FarEditorSet.h"
#include "pcolorer.h"

const char* CerrLogger::LogLevelStr[] {"off", "error", "warning", "info", "debug", "trace"};

CerrLogger::CerrLogger()
{
  const char* verbose_env = getenv("COLORER_VERBOSE");
  if (verbose_env) {
    current_level = getLogLevel(verbose_env);
  }
}

void CerrLogger::log(Logger::LogLevel level, const char* /*filename_in*/, int /*line_in*/,
                     const char* /*funcname_in*/, const char* message)
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

Logger::LogLevel CerrLogger::getLogLevel(const std::string_view log_level)
{
  int i = 0;
  for (const auto it : LogLevelStr) {
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
