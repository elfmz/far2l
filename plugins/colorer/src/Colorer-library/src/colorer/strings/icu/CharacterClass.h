#ifndef COLORER_CHARACTERCLASS_H
#define COLORER_CHARACTERCLASS_H

#include "colorer/strings/icu/common_icu.h"
#include <array>
#include <cstdint>

class CharacterClass
{
 public:
  bool contains(UChar32 c) const
  {
    if (static_cast<uint32_t>(c) <= 0x7f) {
      if (!asciiReady) {
        rebuildAscii();
      }
      return (asciiMask[c >> 6] & (uint64_t(1) << (c & 63))) != 0;
    }
    return set.contains(c);
  }

  CharacterClass& add(UChar32 c)
  {
    set.add(c);
    asciiReady = false;
    return *this;
  }

  CharacterClass& add(UChar32 start, UChar32 end)
  {
    set.add(start, end);
    asciiReady = false;
    return *this;
  }

  CharacterClass& add(const UnicodeString& s)
  {
    set.add(s);
    asciiReady = false;
    return *this;
  }

  CharacterClass& addAll(const CharacterClass& other)
  {
    set.addAll(other.set);
    asciiReady = false;
    return *this;
  }

  CharacterClass& addAll(const icu::UnicodeSet& other)
  {
    set.addAll(other);
    asciiReady = false;
    return *this;
  }

  CharacterClass& addAll(const UnicodeString& s)
  {
    set.addAll(s);
    asciiReady = false;
    return *this;
  }

  CharacterClass& remove(const UnicodeString& s)
  {
    set.remove(s);
    asciiReady = false;
    return *this;
  }

  CharacterClass& removeAll(const CharacterClass& other)
  {
    set.removeAll(other.set);
    asciiReady = false;
    return *this;
  }

  CharacterClass& removeAll(const icu::UnicodeSet& other)
  {
    set.removeAll(other);
    asciiReady = false;
    return *this;
  }

  CharacterClass& removeAll(const UnicodeString& s)
  {
    set.removeAll(s);
    asciiReady = false;
    return *this;
  }

  CharacterClass& retainAll(const CharacterClass& other)
  {
    set.retainAll(other.set);
    asciiReady = false;
    return *this;
  }

  CharacterClass& complement()
  {
    set.complement();
    asciiReady = false;
    return *this;
  }

  void freeze()
  {
    set.freeze();
    rebuildAscii();
  }

 private:
  void rebuildAscii() const
  {
    asciiMask = {};
    for (UChar32 c = 0; c < 128; c++) {
      if (set.contains(c)) {
        asciiMask[c >> 6] |= uint64_t(1) << (c & 63);
      }
    }
    asciiReady = true;
  }

  icu::UnicodeSet set;
  mutable std::array<uint64_t, 2> asciiMask {};
  mutable bool asciiReady = false;
};

#endif  // COLORER_CHARACTERCLASS_H
