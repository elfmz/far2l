#include "colorer/parsers/KeywordList.h"

KeywordList::KeywordList(size_t list_size)
{
  firstChar = std::make_unique<CharacterClass>();
  kwList = new KeywordInfo[list_size];
}

KeywordList::~KeywordList()
{
  delete[] kwList;
}

int kwCompare(const void* e1, const void* e2)
{
  return ((KeywordInfo*) e1)->keyword->compare(*((KeywordInfo*) e2)->keyword);
}

int kwCompareI(const void* e1, const void* e2)
{
  return UStr::caseCompare(*((KeywordInfo*) e1)->keyword, *((KeywordInfo*) e2)->keyword);
}

void KeywordList::sortList()
{
  if (count < 2) {
    return;
  }

  if (matchCase) {
    qsort((void*) kwList, count, sizeof(KeywordInfo), &kwCompare);
  } else {
    qsort((void*) kwList, count, sizeof(KeywordInfo), &kwCompareI);
  }
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
  for (int i = count - 1; i > 0; i--) {
    for (int ii = i - 1; ii >= 0; ii--) {
      if (matchCase) {
        if ((*kwList[ii].keyword)[0] != (*kwList[i].keyword)[0]) {
          break;
        }
        if (kwList[ii].keyword->length() < kwList[i].keyword->length() &&
            UnicodeString(*kwList[i].keyword, 0, kwList[ii].keyword->length()).compare(*kwList[ii].keyword) == 0)
        {
          kwList[i].indexOfShorter = ii;
          break;
        }
      }
      else {
        if (Character::toLowerCase((*kwList[ii].keyword)[0]) != Character::toLowerCase((*kwList[i].keyword)[0])) {
          break;
        }
        if (kwList[ii].keyword->length() < kwList[i].keyword->length() &&
            UStr::caseCompare(UnicodeString(*kwList[i].keyword, 0, kwList[ii].keyword->length()),
                              *kwList[ii].keyword) == 0)
        {
          kwList[i].indexOfShorter = ii;
          break;
        }
      }
    }
  }
}
