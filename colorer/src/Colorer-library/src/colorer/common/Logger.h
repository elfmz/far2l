#ifndef COLORER_LOGGER_H
#define COLORER_LOGGER_H

#include <sstream>

namespace details {

class Argument
{
 public:
  virtual void print(std::ostream& out) const = 0;

 protected:
  ~Argument() = default;
};

void print_impl_inner(std::ostream& out, std::string_view format, size_t arg_count, const Argument** arguments);

template <typename T>
class ArgumentT final : public Argument
{
 public:
  explicit ArgumentT(T const& t) : mData(t) {}

  void print(std::ostream& out) const override { out << mData; }

 private:
  T const& mData;
};

template <typename T>
ArgumentT<T> make_argument(T const& t)
{
  return ArgumentT<T>(t);
}

template <typename... Args>
void print_impl_outer(std::ostream& out, const std::string_view format, Args const&... args)
{
  constexpr size_t n = sizeof...(args);
  Argument const* array[n + 1] = {static_cast<Argument const*>(&args)...};
  print_impl_inner(out, format, n, array);
}

}  // namespace details

template <typename... Args>
void format_log_string(std::ostream& out, std::string_view format, Args&&... args)
{
  details::print_impl_outer(out, format, details::make_argument(std::forward<Args>(args))...);
}

#include "colorer/common/Features.h"
#ifdef COLORER_FEATURE_ICU

#include "unicode/unistr.h"
namespace details {
template <> inline
void ArgumentT<icu::UnicodeString>::print(std::ostream& out) const
{
  std::string result8;
  mData.toUTF8String(result8);
  out << result8;
}
}  // namespace details
#else
#include "colorer/strings/legacy/strings.h"
namespace details {
template <> inline
void details::ArgumentT<UnicodeString>::print(std::ostream& out) const
{
  std::string const result8 = mData.getChars();
  out << result8;
}
}
#endif

class Logger
{
 public:
  enum LogLevel { LOG_OFF, LOG_ERROR, LOG_WARN, LOG_INFO, LOG_DEBUG, LOG_TRACE };

  virtual ~Logger() = default;
  virtual void log(LogLevel level, const char* filename_in, int line_in, const char* funcname_in,
                   const char* message) = 0;
};

class Log
{
 public:

  static void registerLogger(Logger& logger_) { logger = &logger_; }

  template <typename... Args>
  static void log(const Logger::LogLevel level, const char* filename_in, const int line_in, const char* funcname_in,
                  const std::string_view fmt, Args&&... args)
  {
    if (logger != nullptr) {
      std::stringstream buf;
      format_log_string(buf, fmt, std::forward<Args>(args)...);
      logger->log(level, filename_in, line_in, funcname_in, buf.str().c_str());
    }
  }

 private:
  static Logger* logger;
};

#define COLORER_LOGGER_PRINTF(level, ...) Log::log(level, __FILE__, __LINE__, static_cast<const char *>(__FUNCTION__), __VA_ARGS__)
#define COLORER_LOG_ERROR(...) COLORER_LOGGER_PRINTF(Logger::LogLevel::LOG_ERROR, __VA_ARGS__)
#define COLORER_LOG_WARN(...) COLORER_LOGGER_PRINTF(Logger::LogLevel::LOG_WARN, __VA_ARGS__)
#define COLORER_LOG_INFO(...) COLORER_LOGGER_PRINTF(Logger::LogLevel::LOG_INFO, __VA_ARGS__)
#define COLORER_LOG_DEBUG(...) COLORER_LOGGER_PRINTF(Logger::LogLevel::LOG_DEBUG, __VA_ARGS__)
#define COLORER_LOG_TRACE(...) COLORER_LOGGER_PRINTF(Logger::LogLevel::LOG_TRACE, __VA_ARGS__)

#ifdef COLORER_USE_DEEPTRACE
#define COLORER_LOG_DEEPTRACE(...) COLORER_LOGGER_PRINTF(Logger::LogLevel::LOG_TRACE, __VA_ARGS__)
#else
#define COLORER_LOG_DEEPTRACE(...)
#endif


#endif  // COLORER_LOGGER_H
