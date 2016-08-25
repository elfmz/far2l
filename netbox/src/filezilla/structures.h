#pragma once

#include <Classes.hpp>

class CServerPath;

class t_directory : public TObject
{
public:
  t_directory();
  t_directory(const t_directory &a);
  ~t_directory();
  t_server server;
  CServerPath path;
  int num;
  class t_direntry : public TObject
  {
  public:
    t_direntry();
    CString linkTarget;
    CString name;
    CString permissionstr;
    CString humanpermstr; // RFC format
    CString ownergroup;
    __int64 size;
    bool bUnsure; // Set by CFtpControlSocket::FileTransfer when uploads fail after sending STOR/APPE
    bool dir;
    bool bLink;
    class t_date : public TObject
    {
    public:
      t_date();
      int year,month,day,hour,minute,second;
      bool hastime;
      bool hasseconds;
      bool hasdate;
      bool utc;
    } date;
  } * direntry;
  t_directory & operator=(const t_directory & a);
};

