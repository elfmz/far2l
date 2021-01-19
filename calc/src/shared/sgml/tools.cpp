//
//  Copyright (c) Cail Lomecb (Igor Ruskih) 1999-2001 <ruiv@uic.nnov.ru>
//  You can use, modify, distribute this code or any other part
//  of this program in sources or in binaries only according
//  to License (see /doc/license.txt for more information).
//
#include<math.h>
#include<float.h>

bool inline isspace(wchar_t c)
{
  if (c==0x20 || c=='\t' || c=='\r' || c=='\n') return true;
  return false;
};
//  modifed GetNumber - sign extension!
bool get_number(wchar_t *str, double *res)
{
double Numr, r, flt;
int pos, Type, Num;
int s, e, i, j, pt, k, ExpS, ExpE;
bool Exp = false, ExpSign = true, sign = false;

  pos = (int)wcslen(str);
  if (!pos) return false;

  s = 0;
  e = pos;
  Type = 3;
  while(1){
    if(str[0] == '0' && (str[1] == 'x' || str[1] == 'X')){
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
    case 0:
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
    case 3:
      for(i = s;i < e;i++)
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
        r = 0;
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
        if (ExpSign)  Numr = Numr*pow(10,flt);
        if (!ExpSign) Numr = Numr/pow(10,flt);
      };
      *res = Numr;
      break;
  };
  if (sign) *res = -(*res);
  return true;
};
