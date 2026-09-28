#include "colorer/common/Logger.h"

Logger* Log::logger = nullptr;

void details::print_impl_inner(std::ostream& out, const std::string_view format, const size_t arg_count, const Argument** arguments)
{
  std::size_t a = 0;

  for (std::size_t i = 0, max = format.size(); i != max; ++i) {
    switch (format[i]) {
      case '%':
        if (a == arg_count) {
          throw std::invalid_argument {"Too few arguments"};
        }
        arguments[a]->print(out);
        ++a;
        break;
      case '\\':
        ++i;
        if (i == max) {
          throw std::invalid_argument {"Invalid format string: stray \\ at end of string"};
        }
        [[fallthrough]];
      default:
        out << format[i];
    }
  }
}