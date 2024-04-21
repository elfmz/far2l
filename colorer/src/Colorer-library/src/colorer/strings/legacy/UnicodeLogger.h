#ifndef COLORER_UNICODELOGGER_H
#define COLORER_UNICODELOGGER_H

#include "fmt/format.h"
#include <string>
#include "colorer/strings/legacy/UnicodeString.h"

namespace fmt {
template <>
struct formatter<UnicodeString>
{
  template <typename ParseContext>
  constexpr auto parse(ParseContext& ctx)
  {
    return ctx.begin();
  }

  template <typename FormatContext>
  auto format(const UnicodeString& p, FormatContext& ctx)
  {
    std::string result8=p.getChars();
    return format_to(ctx.out(), "{0}", result8);
  }
};

template <>
struct formatter<std::unique_ptr<UnicodeString>>
{
  template <typename ParseContext>
  constexpr auto parse(ParseContext& ctx)
  {
    return ctx.begin();
  }

  template <typename FormatContext>
  auto format(const std::unique_ptr<UnicodeString>& p, FormatContext& ctx)
  {
    std::string result8=p->getChars();
    return format_to(ctx.out(), "{0}", result8);
  }
};
}  // namespace fmt

#endif  // COLORER_UNICODELOGGER_H
