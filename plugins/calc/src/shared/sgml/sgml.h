//
//  Copyright (c) Cail Lomecb (Igor Ruskih) 1999-2001 <ruiv@uic.nnov.ru>
//  You can use, modify, distribute this code or any other part
//  of this program in sources or in binaries only according
//  to License (see /doc/license.txt for more information).
//
#ifndef _CSGML_
#define _CSGML_
#include <string>
// sometimes need it...
bool get_number(wchar_t *str, double *res);
#ifdef _UNIX_
extern "C" int stricmp(wchar_t *s1, wchar_t *s2);
extern "C" int strnicmp(wchar_t *s1, wchar_t *s2, int n);
#endif

typedef class CSgmlEl *PSgmlEl;
typedef class CSgmlEdit *PSgmlEdit;
typedef wchar_t  *SParams[2];

#define MAXPARAMS 0x20
#define MAXTAG 0x10
#define SP 1

enum ElType{
  EPLAINEL, ESINGLEEL, EBLOCKEDEL, EBASEEL
};

class CSgmlEl
{
  //for derived classes
  virtual PSgmlEl createnew(ElType type, PSgmlEl parent, PSgmlEl after);
  virtual bool init();

  void destroylevel();
  void insert(PSgmlEl El);
  bool setcontent(const wchar_t *src,int sz);
  void substquote(SParams par, const wchar_t *sstr, wchar_t c);

  wchar_t *content;
  int contentsz;
  wchar_t name[MAXTAG];

  PSgmlEl eparent;
  PSgmlEl enext;
  PSgmlEl eprev;
  PSgmlEl echild;
  int chnum;
  ElType type;

  SParams params[MAXPARAMS];
  int parnum;

public:
  CSgmlEl();
  virtual ~CSgmlEl();

  bool parse(std::string &path);

  virtual PSgmlEl parent();
  virtual PSgmlEl next();
  virtual PSgmlEl prev();
  virtual PSgmlEl child();

  ElType gettype();
  wchar_t *getname();
  wchar_t *getcontent();
  int getcontentsize();

  wchar_t *GetParam(int no);
  wchar_t *GetChrParam(const wchar_t *par);
  bool GetIntParam(const wchar_t *par, int *result);
  bool GetFloatParam(const wchar_t *par, double *result);

  PSgmlEl search(const wchar_t *TagName);
  PSgmlEl enumchilds(int no);
  // in full hierarchy
  virtual PSgmlEl fprev();
  virtual PSgmlEl fnext();
  virtual PSgmlEl ffirst();
  virtual PSgmlEl flast();

  bool readfile(std::string &path, std::wstring &content);
};

#endif
