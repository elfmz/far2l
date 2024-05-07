#ifndef COLORER_LOGGER_H
#define COLORER_LOGGER_H

#include "colorer/common/Features.h"

#ifndef COLORER_FEATURE_DUMMYLOGGER

#include <spdlog/spdlog.h>

extern std::shared_ptr<spdlog::logger> logger;

#else // COLORER_FEATURE_DUMMYLOGGER
class DummyLogger
{
 public:
  template <typename... Args>
  void debug(Args... /*args*/)
  {
  }
  template <typename... Args>
  void error(Args... /*args*/)
  {
  }
  template <typename... Args>
  void warn(Args... /*args*/)
  {
  }
  template <typename... Args>
  void trace(Args... /*args*/)
  {
  }
};
extern std::shared_ptr<DummyLogger> logger;
#endif // COLORER_FEATURE_DUMMYLOGGER

#endif  // COLORER_LOGGER_H
