#ifndef COLORER_UNICODELOGGER_H
#define COLORER_UNICODELOGGER_H

#include "fmt/format.h"
#include "unicode/unistr.h"

namespace fmt {
template <>
struct formatter<icu::UnicodeString>
{
  template <typename ParseContext>
  constexpr auto parse(ParseContext& ctx)
  {
    return ctx.begin();
  }

  template <typename FormatContext>
  auto format(const icu::UnicodeString& p, FormatContext& ctx)
  {
    std::string result8;
    p.toUTF8String(result8);
    return format_to(ctx.out(), "{0}", result8);
  }
};

template <>
struct formatter<std::unique_ptr<icu::UnicodeString>>
{
  template <typename ParseContext>
  constexpr auto parse(ParseContext& ctx)
  {
    return ctx.begin();
  }

  template <typename FormatContext>
  auto format(const std::unique_ptr<icu::UnicodeString>& p, FormatContext& ctx)
  {
    std::string result8;
    p->toUTF8String(result8);
    return format_to(ctx.out(), "{0}", result8);
  }
};
}  // namespace fmt

#endif  // COLORER_UNICODELOGGER_H
