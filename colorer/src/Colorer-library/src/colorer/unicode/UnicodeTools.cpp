#include <math.h>
#include <float.h>
#include <colorer/unicode/UnicodeTools.h>
#include <colorer/unicode/Character.h>


//  modifed GetNumber - sign extension!
// 1.1123 .123 0x1f #1f $1f
bool UnicodeTools::getNumber(const String* pstr, double* res)
{
  double Numr, r, flt;
  int pos, Type, Num;
  int s, e, i, j, pt, k, ExpS, ExpE;
  bool Exp = false, ExpSign = true, sign = false;

  if (pstr == nullptr || pstr->length() == 0) return false;

  const String &str = *pstr;
  pos = str.length();

  s = 0;
  e = pos;
  Type = 3;
  while (true) {
    if (str.length() > 2 && str[0] == '0' && (str[1] == 'x' || str[1] == 'X')) {
      s = 2;
      Type = 0;
      break;
    }
    if (str[0] == '$' || str[0] == '#') {
      s = 1;
      Type = 0;
      break;
    }
    if (str[0] == '-') {
      Type = 3;
      s = 1;
      sign = true;
      break;
    }
    break;
  }

  switch (Type) {
    case 0: // hex
      Num = 0;
      i = e - 1;
      while (i >= s) {
        j = str[i];
        if (((j < 0x30) || (j > 0x39)) &&
            (((j | 0x20) < 'a') || ((j | 0x20) > 'f')))
          return false;
        if (j > 0x40) j -= 7;
        j &= 15;
        if (i > e - 9) Num |= (j << ((e - i - 1) * 4));
        i--;
      }
      *res = (int)Num;
      break;
    case 3: // double
      for (i = s; i < e; i++)
        if (str[i] == 'e' || str[i] == 'E') {
          Exp = true;
          ExpS = i + 1;
          if (str[i + 1] == '+' || str[i + 1] == '-') {
            ExpS++;
            if (str[i + 1] == '-') ExpSign = false;
          }
          ExpE = e;
          e = i;
        }
      pt = e;
      for (i = s; i < e; i++)
        if (str[i] == '.') {
          pt = i;
          break;
        }
      Numr = 0;
      i = pt - 1;
      while (i >= s) {
        j = str[i];
        if ((j < 0x30) || (j > 0x39))
          return false;
        j &= 15;
        k = pt - i - 1;
        r = (long double)j;
        while (k) {
          k--;
          r *= 10;
        }
        Numr += r;
        i--;
      }
      i = e - 1;
      while (i > pt) {
        j = str[i];
        if ((j < 0x30) || (j > 0x39))
          return false;
        j &= 15;
        k = i - pt;
        r = j;
        while (k) {
          k--;
          r /= 10;
        }
        Numr += r;
        i--;
      }
      if (Exp) {
        flt = 0;
        i = ExpE - 1;
        while (i >= ExpS) {
          j = str[i];
          if ((j < 0x30) || (j > 0x39))
            return false;
          j &= 15;
          k = ExpE - i - 1;
          r = (long double)j;
          while (k) {
            k--;
            r *= 10;
          }
          flt += r;
          i--;
        }
        if (ExpSign)  Numr = Numr * pow(10.0, flt);
        if (!ExpSign) Numr = Numr / pow(10.0, flt);
      }
      *res = Numr;
      break;
  }
  if (sign) *res = -(*res);
  return true;
}

bool UnicodeTools::getNumber(const String* pstr, int* res)
{
  double dres;
  if (getNumber(pstr, &dres)) *res = (int)dres;
  else return false;
  return true;
}

int UnicodeTools::getNumber(const String* pstr)
{
  int r = 1, num = 0;
  if (pstr == nullptr) return -1;
  for (int i = pstr->length() - 1; i >= 0; i--) {
    if ((*pstr)[i] > '9' || (*pstr)[i] < '0') return -1;
    num += ((*pstr)[i] - 0x30) * r;
    r *= 10;
  }
  return num;
}

int UnicodeTools::getHex(wchar c)
{
  c = Character::toLowerCase(c);
  c -= '0';
  if (c >= 'a' - '0' && c <= 'f' - '0') c -= 0x27;
  else if (c > 9) return -1;
  return c;
}
int UnicodeTools::getHexNumber(const String* pstr)
{
  int r = 0, num = 0;
  if (pstr == nullptr) return -1;
  for (int i = (*pstr).length() - 1; i >= 0; i--) {
    int d = getHex((*pstr)[i]);
    if (d == -1) return -1;
    num += d << r;
    r += 4;
  }
  return num;
}

CString* UnicodeTools::getCurlyContent(const String &str, int pos)
{
  if (str[pos] != '{') return nullptr;
  int lpos;
  for (lpos = pos + 1; lpos < str.length(); lpos++) {
    if (str[lpos] == '}')
      break;
    ECharCategory cc = Character::getCategory(str[lpos]);
    // check for what??
    if (Character::isWhitespace(str[lpos]) ||
        cc == CHAR_CATEGORY_Cn || cc == CHAR_CATEGORY_Cc ||
        cc == CHAR_CATEGORY_Cf || cc == CHAR_CATEGORY_Cs)
      return nullptr;
  }
  if (lpos == str.length()) return nullptr;
  return new CString(&str, pos + 1, lpos - pos - 1);
}
wchar UnicodeTools::getEscapedChar(const String &str, int pos, int &retPos)
{
  retPos = pos;
  if (str[pos] == '\\') {
    retPos++;
    if (str[pos + 1] == 'x') {
      if (str[pos + 2] == '{') {
        String* val = getCurlyContent(str, pos + 2);
        if (val == nullptr) return BAD_WCHAR;
        int tmp = getHexNumber(val);
        int val_len = val->length();
        delete val;
        if (tmp < 0 || tmp > 0xFFFF) return BAD_WCHAR;
        retPos += val_len + 2;
        return tmp;
      } else {
        CString dtmp = CString(&str, pos + 2, 2);
        int tmp = getHexNumber(&dtmp);
        if (str.length() <= pos + 2 || tmp == -1) return BAD_WCHAR;
        retPos += 2;
        return tmp;
      }
    }
    return str[pos + 1];
  }
  return str[pos];
}



