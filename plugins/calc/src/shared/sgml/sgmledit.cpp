//
//  Copyright (c) Cail Lomecb (Igor Ruskih) 1999-2001 <ruiv@uic.nnov.ru>
//  You can use, modify, distribute this code or any other part
//  of this program in sources or in binaries only according
//  to License (see /doc/license.txt for more information).
//

// lets you modify objects...

#include<sgml/sgml.cpp>
#include<sgml/sgmledit.h>

CSgmlEdit::CSgmlEdit(){ };
CSgmlEdit::~CSgmlEdit()
{
  if (eparent){
    eparent->chnum--;
    if (eparent->echild == this) eparent->echild = enext;
  };
  if (enext) enext->eprev = eprev;
  if (eprev) eprev->enext = enext;
};

void CSgmlEdit::setname(wchar_t *newname)
{
/*  if (name){
    delete name;
    name = 0;
  };*/
  if (newname){
    // name = new wchar_t[wcslen(newname)+1];
    // !!! length
    wcscpy(name,newname);
  };
};
bool CSgmlEdit::addparam(wchar_t *name, wchar_t *val)
{
int i;
  if (parnum == MAXPARAMS) return false;
  i = parnum;
  parnum++;
  for (i = 0; i < parnum - 1; i++)
    if (!wcscasecmp(params[i][0],name)){
      delete params[i][0];
      delete params[i][1];
      parnum--;
      break;
    };
  params[i][0] = new wchar_t[wcslen(name)+1];
  params[i][1] = new wchar_t[wcslen(val)+1];
  wcsrcpy(params[i][0], name);
  wcscpy(params[i][1], val);
  return true;
};
bool CSgmlEdit::addparam(wchar_t *name, int val)
{
wchar_t IntVal[20];
  swprintf(IntVal,L"%d",val);
  return addparam(name,IntVal);
};
bool CSgmlEdit::addparam(wchar_t *name, double val)
{
wchar_t FltVal[20];
  swprintf(FltVal,L"%.2f",val);
  return addparam(name,FltVal);
};
bool CSgmlEdit::delparam(wchar_t *name)
{
  for (int i = 0; i < parnum; i++)
    if (!wcscasecmp(params[i][0],name)){
      delete params[i][0];
      delete params[i][1];
      params[i][0] = params[parnum-1][0];
      params[i][1] = params[parnum-1][1];
      parnum--;
      return true;
    };
  return false;
};
bool CSgmlEdit::changecontent(wchar_t *data, int len)
{
  if (type != EPLAINEL) return false;
  if (content) delete[] content;
  content = new wchar_t[len];
  memmove(content, data, len);
  contentsz = len;
  return true;
};

bool CSgmlEdit::isloop(PSgmlEdit El, PSgmlEdit Parent)
{
  while(Parent){
    if (El == Parent) return true;
    Parent = (PSgmlEdit)Parent->eparent;
  };
  return false;
};
bool CSgmlEdit::move(PSgmlEdit parent, PSgmlEdit after)
{
  if (isloop(this,parent)) return false;
  if (after && isloop(this,(PSgmlEdit)after->eparent)) return false;
  if (after){
    if (enext) enext->eprev = eprev;
    if (eprev) eprev->enext = enext;
    if (this->eparent->echild == this)
      this->eparent->echild = this->enext;
    this->eparent->chnum--;

    after->insert(this);
    this->eparent = after->eparent;
    if (this->eparent) this->eparent->chnum++;
    return true;
  }else
  if (parent){
    if (enext) enext->eprev = eprev;
    if (eprev) eprev->enext = enext;
    if (this->eparent->echild == this)
      this->eparent->echild = this->enext;
    this->eparent->chnum--;
    this->eparent = parent;
    enext = parent->echild;
    eprev = 0;
    this->eparent->echild = this;
    this->eparent->chnum++;
    if (enext) enext->eprev = this;
    return true;
  };
  return false;
};

int CSgmlEdit::getlevelsize(int Lev)
{
int Pos = 0;
PSgmlEdit tmp = this;
  do{
    if (tmp->gettype() != EPLAINEL)
      Pos +=Lev*SP;
    if (tmp->name[0])
      Pos += wcslen(tmp->name)+1;
    for (int i = 0;i < tmp->parnum;i++){
      Pos +=wcslen(tmp->params[i][0])+2;
      Pos +=wcslen(tmp->params[i][1])+2;
    };
    if (tmp->name[0]) Pos +=3;
    if (tmp->gettype() == EPLAINEL && tmp->content)
      Pos += wcslen(tmp->content)+2;
    if (tmp->echild)
      Pos += PSgmlEdit(tmp->echild)->getlevelsize(Lev+1);
    if (tmp->gettype() == EBLOCKEDEL && tmp->name){
      Pos += Lev*SP+5;
      Pos += wcslen(tmp->name);
    };
    tmp = (PSgmlEdit)tmp->enext;
  }while(tmp);
  return Pos;
};

int CSgmlEdit::savelevel(wchar_t *Dest,int Lev)
{
int i,Pos = 0;
PSgmlEdit tmp = this;
  do{
    if (tmp->gettype() != EPLAINEL)
      for(i = 0; i < Lev*SP; i++)
        Pos += swprintf(Dest+Pos,L" ");
    if (tmp->name[0])
      Pos += swprintf(Dest+Pos,L"<%ls",tmp->name);
    for (i = 0; i < tmp->parnum; i++){
      Pos += swprintf(Dest+Pos,L" %ls=",tmp->params[i][0]);
      Pos += swprintf(Dest+Pos,L"\"%ls\"",tmp->params[i][1]);
    }
    if (tmp->name[0])
      Pos += swprintf(Dest+Pos,L">\r\n");
    if (tmp->gettype() == EPLAINEL)
      Pos += swprintf(Dest+Pos,L"%ls\r\n", tmp->content);
    if (tmp->echild)
      Pos += PSgmlEdit(tmp->echild)->savelevel(Dest+Pos,Lev+1);
    if (tmp->gettype() == EBLOCKEDEL){
      for(i = 0; i < Lev*SP; i++)
        Pos += swprintf(Dest+Pos,L" ");
      Pos += swprintf(Dest+Pos,L"</");
      if (tmp->name) Pos += swprintf(Dest+Pos,L"%ls",tmp->name);
      Pos += swprintf(Dest+Pos,L">\r\n");
    };
    tmp = (PSgmlEdit)tmp->enext;
  }while(tmp);
  return Pos;
};
