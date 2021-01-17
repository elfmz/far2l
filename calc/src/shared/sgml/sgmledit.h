//
//  Copyright (c) Cail Lomecb (Igor Ruskih) 1999-2001 <ruiv@uic.nnov.ru>
//  You can use, modify, distribute this code or any other part
//  of this program in sources or in binaries only according
//  to License (see /doc/license.txt for more information).
//
#ifndef _CSGMLEDIT_
#define _CSGMLEDIT_

#include<sgml/sgml.h>

class CSgmlEdit:public CSgmlEl
{
protected:
  bool  isloop(PSgmlEdit El, PSgmlEdit Parent);
public:
  CSgmlEdit();
  ~CSgmlEdit();

  void setname(wchar_t *newname);

  // parameters add and change
  bool addparam(wchar_t *name, wchar_t *val);
  bool addparam(wchar_t *name, int val);
  bool addparam(wchar_t *name, double val);
  bool changecontent(wchar_t *data, int len);
  bool delparam(wchar_t *name);

  bool move(PSgmlEdit parent, PSgmlEdit after);
  PSgmlEdit copytree(PSgmlEdit el);

  // saving tree into text
  int  getlevelsize(int lev);
  int  savelevel(wchar_t *dest,int lev);
};

#endif
