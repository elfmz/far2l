#ifndef COLORER_KEYWORDLIST_H
#define COLORER_KEYWORDLIST_H

#include <array>
#include <climits>
#include <cstdint>
#include "colorer/Common.h"
#include "colorer/Region.h"

/** Information about one parsed keyword.
    Contains keyword, symbol specifier, region reference
    and internal optimization field.
    @ingroup colorer_parsers
*/
struct KeywordInfo
{
  enum class KeywordType { KT_WORD, KT_SYMB };
  std::unique_ptr<const UnicodeString> keyword;
  const Region* region = nullptr;
  bool isSymbol = false;
  int indexOfShorter = -1;
};

/** List of keywords.
    @ingroup colorer_parsers
*/
class KeywordList
{
 public:
  bool matchCase = false;
  bool hasSymbols = false;
  bool hasNonSymbols = false;
  int count = 0;
  int minKeywordLength = INT_MAX;
  std::unique_ptr<CharacterClass> firstChar;
  std::array<uint64_t, 2> firstCharAscii = {};
  std::array<int, 128> asciiBegin = {};
  std::array<int, 128> asciiEnd = {};
  KeywordInfo* kwList = nullptr;
  explicit KeywordList(size_t list_size);
  ~KeywordList();
  void sortList();
  void substrIndex();
};

#endif  //COLORER_KEYWORDLIST_H
