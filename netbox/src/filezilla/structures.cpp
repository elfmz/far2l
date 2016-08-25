
#include "stdafx.h"


#if defined(__BORLANDC__)
AFX_COMDAT int _afxInitDataA[] = { -1, 0, 0, 0 };
AFX_COMDAT CStringDataA* _afxDataNilA = (CStringDataA*)&_afxInitDataA;
AFX_COMDAT LPCSTR _afxPchNilA = (LPCSTR)(((BYTE*)&_afxInitDataA)+sizeof(CStringDataA));
#endif

t_directory::t_directory() :
  num(0),
  direntry(0)
{
}

t_directory::t_directory(const t_directory & a) :
  num(0),
  direntry(0)
{
  this->operator=(a);
}

t_directory::~t_directory()
{
  if (direntry)
    delete [] direntry;
}

t_directory& t_directory::operator=(const t_directory &a)
{
  if (&a==this)
    return *this;

  if (direntry)
    delete [] direntry;
  direntry=0;
  path=a.path;
  num=a.num;
  server=a.server;
  if (num)
    direntry=new t_directory::t_direntry[num];
  for (int i=0;i<num;i++)
    direntry[i]=a.direntry[i];
  return *this;
}

t_directory::t_direntry::t_direntry()
{
  dir=FALSE;
  size=0;
  bUnsure=TRUE;
  bLink=FALSE;
}

t_directory::t_direntry::t_date::t_date()
{
  year=month=day=hour=minute=second=0;
  hasdate=hastime=hasseconds=utc=FALSE;
}
