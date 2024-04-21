#ifndef COLORER_UNICODESTRING_CONTAINER_H
#define COLORER_UNICODESTRING_CONTAINER_H

#include <unordered_map>
#include "colorer/strings/legacy/UnicodeString.h"

namespace std {
// Specializations for unordered containers

template <>
struct hash<UnicodeString>
{
  size_t operator()(const UnicodeString& value) const { return value.hashCode(); }
};
template <>
struct equal_to<UnicodeString>
{
  bool operator()(const UnicodeString& u1, const UnicodeString& u2) const
  {
    return u1.compare(u2) == 0;
  }
};

}  // namespace std
#endif  // COLORER_UNICODESTRING_CONTAINER_H