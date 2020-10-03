#ifndef _COLORER_CHARACTERCLASS_H_
#define _COLORER_CHARACTERCLASS_H_

#include <colorer/unicode/String.h>
#include <colorer/unicode/BitArray.h>
#include <colorer/unicode/Character.h>

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
  BitArray** infoIndex;
public:
  CharacterClass();
  CharacterClass(const CharacterClass &);
  ~CharacterClass();

  static CharacterClass* createCharClass(const String& ccs, int pos, int* retPos, bool ignore_case);

  void addChar(wchar);
  void clearChar(wchar);
  void addRange(wchar, wchar);
  void clearRange(wchar, wchar);

  void addCategory(ECharCategory);
  void addCategory(const char*);
  void addCategory(const String &);
  void clearCategory(ECharCategory);
  void clearCategory(const char*);
  void clearCategory(const String &);

  void addClass(const CharacterClass &);
  void clearClass(const CharacterClass &);
  void intersectClass(const CharacterClass &);
  void clear();
  void fill();

  bool inClass(wchar c) const;

};

#endif


