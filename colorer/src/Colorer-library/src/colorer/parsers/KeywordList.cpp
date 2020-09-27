#include <colorer/parsers/KeywordList.h>

KeywordList::KeywordList()
{
  num = 0;
  matchCase = false;
  minKeywordLength = 0;
  kwList = nullptr;
  firstChar.reset(new CharacterClass());
}

KeywordList::~KeywordList()
{
  delete[] kwList;
}

int kwCompare(const void* e1, const void* e2)
{
  return ((KeywordInfo*)e1)->keyword->compareTo(*((KeywordInfo*)e2)->keyword);
}

int kwCompareI(const void* e1, const void* e2)
{
  return ((KeywordInfo*)e1)->keyword->compareToIgnoreCase(*((KeywordInfo*)e2)->keyword);
}

void KeywordList::sortList()
{
  if (num < 2) {
    return;
  }

  if (matchCase) {
    qsort((void*)kwList, num, sizeof(KeywordInfo), &kwCompare);
  } else {
    qsort((void*)kwList, num, sizeof(KeywordInfo), &kwCompareI);
  }
}

/* Searches previous elements num with same partial name
   for example:
   3: getParameterName  0
   2: getParameter      0
   1: getParam          0
   0: getPar            -1
*/
void KeywordList::substrIndex()
{
  for (int i = num - 1; i > 0; i--)
    for (int ii = i - 1; ii != 0; ii--) {
      if ((*kwList[ii].keyword)[0] != (*kwList[i].keyword)[0]) {
        break;
      }
      if (kwList[ii].keyword->length() < kwList[i].keyword->length() &&
          CString(kwList[i].keyword.get(), 0, kwList[ii].keyword->length()).equals(kwList[ii].keyword.get())) {
        kwList[i].ssShorter = ii;
        break;
      }
    }
}
