#ifndef COLORER_CHARACTERCLASS_H
#define COLORER_CHARACTERCLASS_H

#include <memory>
#include <colorer/strings/legacy/UnicodeString.h>
#include <colorer/strings/legacy/BitArray.h>
#include <colorer/strings/legacy/Character.h>

/** Character classes store implementation.
    - CharacterClass allows to store enumerations of characters in compact
      form (two-stage bit-field tables).
    - This class supports logical operations over it's instances,
      char category -> enumeration conversion.
    @ingroup unicode
*/
class CharacterClass
{
private:
  BitArray infoIndex[256];
public:
  CharacterClass();
  ~CharacterClass();

  static std::unique_ptr<CharacterClass> createCharClass(const UnicodeString& ccs, int pos, int* retPos, bool ignore_case);

  void addChar(wchar);
  void add(wchar);
  void clearChar(wchar);
  void addRange(wchar, wchar);
  void clearRange(wchar, wchar);

  void addCategory(ECharCategory);
  void addCategory(const char*);
  void addCategory(const UnicodeString &);
  void clearCategory(ECharCategory);
  void clearCategory(const char*);
  void clearCategory(const UnicodeString &);

  void addClass(const CharacterClass &);
  void clearClass(const CharacterClass &);
  void intersectClass(const CharacterClass &);
  void clear();
  void fill();

  inline bool inClass(wchar c) const
  {
    return infoIndex[(c >> 8) & 0xFF].getBit(c & 0xFF);
  }

  inline bool contains(wchar c) const
  {
    return inClass(c);
  }

  void freeze();

};

#endif
