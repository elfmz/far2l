#include "colorer/strings/icu/UnicodeTools.h"
#include "colorer/strings/icu/Character.h"

int UnicodeTools::getHex(UChar c)
{
  c = Character::toLowerCase(c);
  c -= '0';
  if (c >= 'a' - '0' && c <= 'f' - '0') {
    c -= 0x27;
  }
  else if (c > 9) {
    return -1;
  }
  return c;
}

int UnicodeTools::getHexNumber(const UnicodeString* pstr)
{
  int r = 0;
  int num = 0;
  if (pstr == nullptr) {
    return -1;
  }
  for (int i = (*pstr).length() - 1; i >= 0; i--) {
    int d = getHex((*pstr)[i]);
    if (d == -1) {
      return -1;
    }
    num += d << r;
    r += 4;
  }
  return num;
}

int UnicodeTools::getNumber(const UnicodeString* pstr)
{
  int r = 1, num = 0;
  if (pstr == nullptr) {
    return -1;
  }
  for (int i = pstr->length() - 1; i >= 0; i--) {
    if ((*pstr)[i] > '9' || (*pstr)[i] < '0') {
      return -1;
    }
    num += ((*pstr)[i] - 0x30) * r;
    r *= 10;
  }
  return num;
}

UChar UnicodeTools::getEscapedChar(const UnicodeString& str, int pos, int& retPos)
{
  retPos = pos;
  if (str[pos] == '\\') {
    retPos++;
    if (str[pos + 1] == 'x') {
      if (str[pos + 2] == '{') {
        auto val = getCurlyContent(str, pos + 2);
        if (val == nullptr) {
          return BAD_WCHAR;
        }
        int tmp = getHexNumber(val.get());
        int val_len = val->length();
        if (tmp < 0 || tmp > 0xFFFF) {
          return BAD_WCHAR;
        }
        retPos += val_len + 2;
        return static_cast<UChar>(tmp);
      }
      else {
        auto dtmp = UnicodeString(str, pos + 2, 2);
        int tmp = getHexNumber(&dtmp);
        if (str.length() <= pos + 2 || tmp == -1) {
          return BAD_WCHAR;
        }
        retPos += 2;
        return static_cast<UChar>(tmp);
      }
    }
    return str[pos + 1];
  }
  return str[pos];
}

uUnicodeString UnicodeTools::getCurlyContent(const UnicodeString& str, int pos)
{
  if (str[pos] != '{') {
    return nullptr;
  }
  int lpos;
  for (lpos = pos + 1; lpos < str.length(); lpos++) {
    if (str[lpos] == '}') {
      break;
    }
    if (!u_isgraph(str[lpos])) {
      return nullptr;
    }
  }
  if (lpos == str.length()) {
    return nullptr;
  }
  return std::make_unique<UnicodeString>(str, pos + 1, lpos - pos - 1);
}
