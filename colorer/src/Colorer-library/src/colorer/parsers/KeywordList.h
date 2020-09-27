#ifndef _COLORER_KEYWORDLIST_H_
#define _COLORER_KEYWORDLIST_H_

#include <colorer/Common.h>
#include <colorer/Region.h>
#include <colorer/unicode/CharacterClass.h>

/** Information about one parsed keyword.
    Contains keyword, symbol specifier, region reference
    and internal optimization field.
    @ingroup colorer_parsers
*/
struct KeywordInfo {
  std::unique_ptr<const SString> keyword;
  const Region* region;
  bool isSymbol;
  int  ssShorter;
};

/** List of keywords.
    @ingroup colorer_parsers
*/
class KeywordList
{
public:
  int num;
  bool matchCase;
  unsigned int minKeywordLength;
  std::unique_ptr<CharacterClass> firstChar;
  KeywordInfo* kwList;
  KeywordList();
  ~KeywordList();
  void sortList();
  void substrIndex();


};

#endif //_COLORER_KEYWORDLIST_H_


