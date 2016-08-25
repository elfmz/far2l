
#pragma once

#include <nbglobals.h>
#include <openssl/pkcs12.h>

class t_server
{
CUSTOM_MEM_ALLOCATION_IMPL
public:
  t_server();
  ~t_server();
  CString host;
  int port;
  CString user, pass, account;
  CString path;
  int nServerType;
  int nPasv;
  int nTimeZoneOffset;
  int nUTF8;
  int nCodePage;
  int iForcePasvIp;
  int iUseMlsd;
  int iDupFF;
  int iUndupFF;
  bool operator<(const t_server &op) const; //Needed by STL map
  X509 * Certificate;
  EVP_PKEY * PrivateKey;
};

const bool operator == (const t_server &a,const t_server &b);
const bool operator != (const t_server &a,const t_server &b);

#include "ServerPath.h"

struct t_transferfile
{
CUSTOM_MEM_ALLOCATION_IMPL

  CString localfile;
  CString remotefile;
  CServerPath remotepath;
  BOOL get;
  __int64 size;
  t_server server;
  int nType;
  void * nUserData;
};


