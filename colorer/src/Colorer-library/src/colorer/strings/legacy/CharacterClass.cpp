#include <cstring>
#include <colorer/strings/legacy/CharacterClass.h>
#include <colorer/strings/legacy/UnicodeTools.h>
#include <colorer/strings/legacy/x_charcategory_names.h>
#include <colorer/strings/legacy/x_charcategory2.h>

/// macro - number of elements in array
#define ARRAY_SIZE(a) (sizeof(a)/sizeof(*(a)))

CharacterClass::CharacterClass()
{
  infoIndex = new BitArray*[256];
  memset(infoIndex, 0, 256 * sizeof(*infoIndex));
}

CharacterClass::~CharacterClass()
{
  clear();
  delete[] infoIndex;
}

/**
  Creates CharacterClass object from regexp character class syntax.
  Extensions (comparing to Perl):
  inner class substraction [{L}-[{Lu}]], addition [{L}[1234]], intersection [{L}&[{Lu}]]
*/
std::unique_ptr<CharacterClass> CharacterClass::createCharClass(const UnicodeString& ccs, int pos, int* retPos, bool ignore_case)
{
  if (ccs[pos] != '[') return nullptr;

  auto cc = std::make_unique<CharacterClass>();
  CharacterClass cc_temp;
  bool inverse = false;
  wchar prev_char = BAD_WCHAR;

  pos++;
  if (ccs[pos] == '^') {
    inverse = true;
    pos++;
  }
  for (; pos < ccs.length(); pos++) {
    if (ccs[pos] == ']') {
      if (retPos != nullptr) *retPos = pos;
      if (inverse) {
        auto newcc = std::make_unique<CharacterClass>();
        newcc->fill();
        newcc->clearClass(*cc);
        cc=std::move(newcc);
      }
      return cc;
    }
    if (ccs[pos] == '{') {
      auto categ = UnicodeTools::getCurlyContent(ccs, pos);
      if (categ == nullptr) {
        return nullptr;
      }
      if (*categ == UnicodeString("ALL")) cc->fill();
      else if (*categ == UnicodeString("ASSIGNED")) cc->addCategory("");
      else if (*categ == UnicodeString("UNASSIGNED")) {
        cc_temp.clear();
        cc_temp.addCategory("");
        cc->fill();
        cc->clearClass(cc_temp);
      } else if (categ->length()) cc->addCategory(*categ);
      pos += categ->length() + 1;
      prev_char = BAD_WCHAR;
      continue;
    }
    if (ccs[pos] == '\\' && pos + 1 < ccs.length()) {
      int retEnd;
      prev_char = BAD_WCHAR;
      switch (ccs[pos + 1]) {
        case 'd':
          cc->addCategory(CHAR_CATEGORY_Nd);
          break;
        case 'D':
          cc_temp.fill();
          cc_temp.clearCategory(CHAR_CATEGORY_Nd);
          cc->addClass(cc_temp);
          break;
        case 'w':
          cc->addCategory("L");
          cc->addCategory(CHAR_CATEGORY_Nd);
          cc->addChar('_');
          break;
        case 'W':
          cc_temp.fill();
          cc_temp.clearCategory("L");
          cc_temp.clearCategory(CHAR_CATEGORY_Nd);
          cc_temp.clearChar('_');
          cc->addClass(cc_temp);
          break;
        case 's':
          cc->addCategory("Z");
          cc->addChar(0x09);
          cc->addChar(0x0A);
          cc->addChar(0x0C);
          cc->addChar(0x0D);
          break;
        case 'S':
          cc_temp.fill();
          cc_temp.clearCategory("Z");
          cc_temp.clearChar(0x09);
          cc_temp.clearChar(0x0A);
          cc_temp.clearChar(0x0C);
          cc_temp.clearChar(0x0D);
          cc->addClass(cc_temp);
          break;
        case 'l':
          cc->addCategory(CHAR_CATEGORY_Ll);
          if (ignore_case) cc->addCategory(CHAR_CATEGORY_Lu);
          break;
        case 'u':
          cc->addCategory(CHAR_CATEGORY_Lu);
          if (ignore_case) cc->addCategory(CHAR_CATEGORY_Ll);
          break;
        default:
          prev_char = UnicodeTools::getEscapedChar(ccs, pos, retEnd);
          if (prev_char == BAD_WCHAR) break;
          cc->addChar(prev_char);
          if (ignore_case){
            cc->addChar(Character::toLowerCase(prev_char));
            cc->addChar(Character::toUpperCase(prev_char));
            cc->addChar(Character::toTitleCase(prev_char));
          }
          pos = retEnd - 1;
          break;
      }
      pos++;
      continue;
    }
    // substract -[class]
    if (pos + 1 < ccs.length() && ccs[pos] == '-' && ccs[pos + 1] == '[') {
      int retEnd = 0;
      auto scc = createCharClass(ccs, pos + 1, &retEnd, false);
      if (retEnd == ccs.length()) {
        return nullptr;
      }
      if (scc == nullptr) {
        return nullptr;
      }
      cc->clearClass(*scc);
      pos = retEnd;
      prev_char = BAD_WCHAR;
      continue;
    }
    // intersect &&[class]
    if (pos + 2 < ccs.length() && ccs[pos] == '&' && ccs[pos + 1] == '&' && ccs[pos + 2] == '[') {
      int retEnd;
      auto scc = createCharClass(ccs, pos + 2, &retEnd, false);
      if (retEnd == ccs.length()) {
        return nullptr;
      }
      if (scc == nullptr) {
        return nullptr;
      }
      cc->intersectClass(*scc);
      pos = retEnd;
      prev_char = BAD_WCHAR;
      continue;
    }
    // add [class]
    if (ccs[pos] == '[') {
      int retEnd;
      auto scc = createCharClass(ccs, pos, &retEnd, ignore_case);
      if (scc == nullptr) {
        return nullptr;
      }
      cc->addClass(*scc);
      pos = retEnd;
      prev_char = BAD_WCHAR;
      continue;
    }
    if (ccs[pos] == '-' && prev_char != BAD_WCHAR && pos + 1 < ccs.length() && ccs[pos + 1] != ']') {
      int retEnd;
      wchar nextc = UnicodeTools::getEscapedChar(ccs, pos + 1, retEnd);
      if (nextc == BAD_WCHAR) break;
      cc->addRange(prev_char, nextc);
      pos = retEnd;
      continue;
    }
    cc->addChar(ccs[pos]);
    if (ignore_case){
      cc->addChar(Character::toLowerCase(ccs[pos]));
      cc->addChar(Character::toUpperCase(ccs[pos]));
      cc->addChar(Character::toTitleCase(ccs[pos]));
    }
    prev_char = ccs[pos];
  }
  return nullptr;
}

void CharacterClass::addChar(wchar c)
{
  BitArray* tablePos = infoIndex[(c >> 8) & 0xFF];
  if (!tablePos) {
    tablePos = new BitArray();
    infoIndex[(c >> 8) & 0xFF] = tablePos;
  }
  tablePos->setBit(c & 0xFF);
}

void CharacterClass::add(wchar c)
{
  addChar(c);
}

void CharacterClass::clearChar(wchar c)
{
  BitArray* tablePos = infoIndex[(c >> 8) & 0xFF];
  if (!tablePos) return;
  tablePos->clearBit(c & 0xFF);
}

void CharacterClass::addRange(wchar s, wchar e)
{
  for (int ti = (int)(s >> 8) & 0xFF; ti <= (int)((e >> 8) & 0xFF); ti++) {
    if (!infoIndex[ti]) infoIndex[ti] = new BitArray();
    infoIndex[ti]->addRange((ti == (int)(s >> 8)) ? s & 0xFF : 0, (ti == (int)(e >> 8)) ? e & 0xFF : 0xFF);
  }
}

void CharacterClass::clearRange(wchar s, wchar e)
{
  for (int ti = (s >> 8) & 0xFF; ti <= (int)((e >> 8) & 0xFF); ti++) {
    if (!infoIndex[ti]) infoIndex[ti] = new BitArray();
    infoIndex[ti]->clearRange(ti == (int)(s >> 8) ? s & 0xFF : 0, ti == (int)(e >> 8) ? e & 0xFF : 0xFF);
  }
}

void CharacterClass::addCategory(ECharCategory cat)
{
  if (cat == CHAR_CATEGORY_Cn || cat >= CHAR_CATEGORY_LAST) return;
  for (int i = 0; i < 0x100; i++) {
    unsigned short pos = arr_idxCharCategoryIdx[(int(cat) - 1) * 0x100 + i];
    if (!pos) continue;
    BitArray* tablePos = infoIndex[i];
    if (!tablePos) {
      tablePos = new BitArray();
      infoIndex[i] = tablePos;
    }
    tablePos->addBitArray((char*)(arr_idxCharCategory + pos), 8 * 4);
  }
}

void CharacterClass::addCategory(const UnicodeString &cat)
{
  for (size_t pos = 0; pos < ARRAY_SIZE(char_category_names); pos++) {
    int32_t ci;
    for (ci = 0; ci < cat.length() && cat[ci] == char_category_names[pos][ci]; ci++);
    if (ci == cat.length()) addCategory(ECharCategory(pos));
  }
}

void CharacterClass::addCategory(const char* cat)
{
  addCategory(UnicodeString(cat));
}

void CharacterClass::clearCategory(ECharCategory cat)
{
  if (cat == CHAR_CATEGORY_Cn || cat >= CHAR_CATEGORY_LAST) return;
  for (int i = 0; i < 0x100; i++) {
    unsigned short pos = arr_idxCharCategoryIdx[(int(cat) - 1) * 0x100 + i];
    if (!pos) continue;
    BitArray* tablePos = infoIndex[i];
    if (!tablePos) {
      tablePos = new BitArray();
      infoIndex[i] = tablePos;
    }
    tablePos->clearBitArray((char*)(arr_idxCharCategory + pos), 8 * 4);
  }
}

void CharacterClass::clearCategory(const UnicodeString &cat)
{
  for (size_t pos = 0; pos < ARRAY_SIZE(char_category_names); pos++) {
    int32_t ci;
    for (ci = 0; ci < cat.length() && cat[ci] == char_category_names[pos][ci]; ci++);
    if (ci == cat.length()) clearCategory(ECharCategory(pos));
  }
}

void CharacterClass::clearCategory(const char* cat)
{
  clearCategory(UnicodeString(cat));
}

void CharacterClass::addClass(const CharacterClass &cclass)
{
  for (int p = 0; p < 256; p++) {
    if (!infoIndex[p]) infoIndex[p] = new BitArray();
    infoIndex[p]->addBitArray(cclass.infoIndex[p]);
  }
}

void CharacterClass::intersectClass(const CharacterClass &cclass)
{
  for (int p = 0; p < 256; p++) {
    if (infoIndex[p])
      infoIndex[p]->intersectBitArray(cclass.infoIndex[p]);
  }
}

void CharacterClass::clearClass(const CharacterClass &cclass)
{
  for (int p = 0; p < 256; p++)
    if (infoIndex[p])
      infoIndex[p]->clearBitArray(cclass.infoIndex[p]);
}

void CharacterClass::clear()
{
  for (int i = 0; i < 256; i++)
    if (infoIndex[i]) {
      delete infoIndex[i];
      infoIndex[i] = nullptr;
    }
}

void CharacterClass::fill()
{
  for (int i = 0; i < 256; i++) {
    if (!infoIndex[i]) infoIndex[i] = new BitArray();
    infoIndex[i]->addRange(0, 0xFF);
  }
}

bool CharacterClass::inClass(wchar c) const
{
  BitArray* tablePos = infoIndex[(c >> 8) & 0xFF];
  if (!tablePos) return false;
  return tablePos->getBit(c & 0xFF);
}

bool CharacterClass::contains(wchar c) const
{
  return inClass(c);
}

void CharacterClass::freeze() {}
