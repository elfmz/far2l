#ifndef COLORER_UNICODESTRING_CONTAINER_H
#define COLORER_UNICODESTRING_CONTAINER_H

#include <memory>
#include "unicode/unistr.h"

namespace std {
// Specializations for unordered containers

template <>
struct hash<icu::UnicodeString>
{
  size_t operator()(const icu::UnicodeString& value) const
  {
    return static_cast<std::size_t>(value.hashCode());
  }
};

template <>
struct equal_to<icu::UnicodeString>
{
  bool operator()(const icu::UnicodeString& u1, const icu::UnicodeString& u2) const
  {
    return u1.compare(u2) == 0;
  }
};

}  // namespace std

#endif  // COLORER_UNICODESTRING_CONTAINER_H
