#include "colorer/parsers/KeywordList.h"
#include <algorithm>

KeywordList::KeywordList(size_t list_size)
{
  firstChar = std::make_unique<CharacterClass>();
  kwList = new KeywordInfo[list_size];
}

KeywordList::~KeywordList()
{
  delete[] kwList;
}

void KeywordList::sortList() // sort and remove dups
{
  std::sort(kwList, kwList + count, [&](const KeywordInfo& a, const KeywordInfo& b) {
      int cmp = a.keyword->compare(*b.keyword);
      if (cmp != 0) {
        return cmp < 0;
      }
      if (a.region != b.region) {
        return a.region < b.region;
      }
      return a.isSymbol < b.isSymbol;
  });
  KeywordInfo* new_end = std::unique(kwList, kwList + count, [&](const KeywordInfo& a, const KeywordInfo& b) {
      return a.region == b.region && a.isSymbol == b.isSymbol && a.keyword->compare(*b.keyword) == 0; // indexOfShorter is irrelevant now
  });
  count = static_cast<int>(new_end - kwList);
}

/* Searches previous elements num with same partial name
   for example:
   3: getParameterName  2
   2: getParameter      1
   1: getParam          0
   0: getPar           -1
*/
void KeywordList::substrIndex()
{
  for (int i = 0; i < count; i++) {
    if (kwList[i].isSymbol) {
      hasSymbols = true;
    } else {
      hasNonSymbols = true;
    }
  }
  firstCharAscii = {};
  asciiBegin = {};
  asciiEnd = {};
  for (int i = count - 1; i > 0; i--) {
    for (int ii = i - 1; ii >= 0; ii--) {
      if ((*kwList[ii].keyword)[0] != (*kwList[i].keyword)[0]) {
        break;
      }
      if (kwList[ii].keyword->length() < kwList[i].keyword->length() &&
          kwList[i].keyword->compare(0, kwList[ii].keyword->length(), *kwList[ii].keyword) == 0)
      {
        kwList[i].indexOfShorter = ii;
        break;
      }
    }
  }
  for (int i = 0; i < count; ) {
    const auto ch = static_cast<uint32_t>((*kwList[i].keyword)[0]);
    int j = i + 1;
    while (j < count && static_cast<uint32_t>((*kwList[j].keyword)[0]) == ch) {
      j++;
    }
    if (ch < 128) {
      firstCharAscii[ch >> 6] |= uint64_t(1) << (ch & 63);
      asciiBegin[ch] = i;
      asciiEnd[ch] = j;
    }
    i = j;
  }
}
