#include<math.h>
#include<float.h>
#include<unicode/UnicodeTools.h>


//  modifed GetNumber - sign extension!
// 1.1123 .123 0x1f #1f $1f
bool UnicodeTools::getNumber(const String *pstr, double *res)
{
double Numr, r, flt;
int pos, Type, Num;
int s, e, i, j, pt, k, ExpS, ExpE;
bool Exp = false, ExpSign = true, sign = false;

  if (pstr == null || pstr->length() == 0) return false;

  const String &str = *pstr;
  pos = str.length();

  s = 0;
  e = pos;
  Type = 3;
  while(true){
    if(str.length() > 2 && str[0] == '0' && (str[1] == 'x' || str[1] == 'X')){
      s = 2;
      Type = 0;
      break;
    };
    if (str[0] == '$' || str[0] == '#'){
      s = 1;
      Type = 0;
      break;
    };
    if (str[0] == '-'){
      Type = 3;
      s = 1;
      sign = true;
      break;
    };
    break;
  };

  switch(Type){
    case 0: // hex
      Num = 0;
      i = e-1;
      while(i >= s){
        j = str[i];
        if(((j < 0x30) || (j > 0x39)) &&
          (((j | 0x20) < 'a') || ((j | 0x20) > 'f')))
            return false;
        if(j > 0x40) j -=7;
        j &=15;
        if(i > e-9) Num |= (j << ((e-i-1)*4) );
        i--;
      };
      *res = (int)Num;
      break;
    case 3: // double
      for(i = s; i < e; i++)
      if (str[i] == 'e' || str[i] =='E'){
        Exp = true;
        ExpS = i+1;
        if (str[i+1] == '+' || str[i+1] == '-'){
          ExpS++;
          if (str[i+1] == '-') ExpSign = false;
        };
        ExpE = e;
        e = i;
      };
      pt = e;
      for(i = s;i < e;i++)
        if (str[i] == '.'){
          pt = i;
          break;
        };
      Numr = 0;
      i = pt-1;
      while(i >= s){
        j = str[i];
        if((j < 0x30)||(j > 0x39))
          return false;
        j &=15;
        k = pt-i-1;
        r = (long double)j;
        while(k){
          k--;
          r *=10;
        };
        Numr += r;
        i--;
      };
      i = e-1;
      while(i > pt){
        j = str[i];
        if((j < 0x30)||(j > 0x39))
          return false;
        j &=15;
        k = i-pt;
        r = j;
        while(k){
          k--;
          r /=10;
        };
        Numr += r;
        i--;
      };
      if (Exp){
        flt = 0;
        i = ExpE-1;
        while(i >= ExpS){
          j = str[i];
          if((j < 0x30)||(j > 0x39))
            return false;
          j &=15;
          k = ExpE-i-1;
          r = (long double)j;
          while(k){
            k--;
            r *=10;
          };
          flt += r;
          i--;
        };
        if (ExpSign)  Numr = Numr * pow(10.0, flt);
        if (!ExpSign) Numr = Numr / pow(10.0, flt);
      };
      *res = Numr;
      break;
  };
  if (sign) *res = -(*res);
  return true;
}

bool UnicodeTools::getNumber(const String *pstr, int *res){
  double dres;
  if (getNumber(pstr, &dres)) *res = (int)dres;
  else return false;
  return true;
}

int UnicodeTools::getNumber(const String *pstr)
{
int r = 1, num = 0;
  if (pstr == null) return -1;
  for(int i = pstr->length()-1; i >=0; i--){
    if ((*pstr)[i] > '9' || (*pstr)[i] < '0') return -1;
    num += ((*pstr)[i] - 0x30)*r;
    r *= 10;
  };
  return num;
}

int UnicodeTools::getHex(wchar c)
{
  c = Character::toLowerCase(c);
  c -= '0';
  if (c >= 'a'-'0' && c <= 'f'-'0') c -= 0x27;
  else if (c > 9) return -1;
  return c;
}
int UnicodeTools::getHexNumber(const String *pstr)
{
int r = 0, num = 0;
  if (pstr == null) return -1;
  for(int i = (*pstr).length()-1; i >= 0; i--){
    int d = getHex((*pstr)[i]);
    if (d == -1) return -1;
    num += d << r;
    r += 4;
  };
  return num;
}

DString *UnicodeTools::getCurlyContent(const String &str, int pos)
{
  if (str[pos] != '{') return null;
  int lpos;
  for(lpos = pos+1; lpos < str.length(); lpos++){
    if (str[lpos] == '}')
      break;
    ECharCategory cc = Character::getCategory(str[lpos]);
    // check for what??
    if (Character::isWhitespace(str[lpos]) ||
        cc == CHAR_CATEGORY_Cn || cc == CHAR_CATEGORY_Cc ||
        cc == CHAR_CATEGORY_Cf || cc == CHAR_CATEGORY_Cs)
      return null;
  };
  if (lpos == str.length()) return null;
  return new DString(&str, pos+1, lpos-pos-1);
}
wchar UnicodeTools::getEscapedChar(const String &str, int pos, int &retPos)
{
retPos = pos;
  if (str[pos] == '\\'){
    retPos++;
    if (str[pos+1] == 'x'){
      if (str[pos+2] == '{'){
        String *val = getCurlyContent(str, pos+2);
        if (val == null) return BAD_WCHAR;
        int tmp = getHexNumber(val);
        int val_len = val->length();
        delete val;
        if (tmp < 0 || tmp > 0xFFFF) return BAD_WCHAR;
        retPos += val_len+2;
        return tmp;
      }else{
		  DString ds(&str, pos+2, 2);
        int tmp = getHexNumber(&ds);
        if (str.length() <=pos+2 || tmp == -1) return BAD_WCHAR;
        retPos += 2;
        return tmp;
      };
    };
    return str[pos+1];
  };
  return str[pos];
}

/* ***** BEGIN LICENSE BLOCK *****
 * Version: MPL 1.1/GPL 2.0/LGPL 2.1
 *
 * The contents of this file are subject to the Mozilla Public License Version
 * 1.1 (the "License"); you may not use this file except in compliance with
 * the License. You may obtain a copy of the License at
 * http://www.mozilla.org/MPL/
 *
 * Software distributed under the License is distributed on an "AS IS" basis,
 * WITHOUT WARRANTY OF ANY KIND, either express or implied. See the License
 * for the specific language governing rights and limitations under the
 * License.
 *
 * The Original Code is the Colorer Library.
 *
 * The Initial Developer of the Original Code is
 * Cail Lomecb <cail@nm.ru>.
 * Portions created by the Initial Developer are Copyright (C) 1999-2005
 * the Initial Developer. All Rights Reserved.
 *
 * Contributor(s):
 *
 * Alternatively, the contents of this file may be used under the terms of
 * either the GNU General Public License Version 2 or later (the "GPL"), or
 * the GNU Lesser General Public License Version 2.1 or later (the "LGPL"),
 * in which case the provisions of the GPL or the LGPL are applicable instead
 * of those above. If you wish to allow use of your version of this file only
 * under the terms of either the GPL or the LGPL, and not to allow others to
 * use your version of this file under the terms of the MPL, indicate your
 * decision by deleting the provisions above and replace them with the notice
 * and other provisions required by the GPL or the LGPL. If you do not delete
 * the provisions above, a recipient may use your version of this file under
 * the terms of any one of the MPL, the GPL or the LGPL.
 *
 * ***** END LICENSE BLOCK ***** */
