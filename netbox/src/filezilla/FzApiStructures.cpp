
#include "stdafx.h"
#include "FzApiStructures.h"

t_server::t_server()
{
  port = 0;
  nServerType = 0;
  nPasv = 0;
  nTimeZoneOffset = 0;
  nUTF8 = 0;
  nCodePage = 0;
  iForcePasvIp = -1;
  iUseMlsd = -1;
  iDupFF = 0;
  iUndupFF = 0;
  Certificate = NULL;
  PrivateKey = NULL;
}

t_server::~t_server()
{
}

const bool operator == (const t_server & a, const t_server & b)
{
  if (a.host!=b.host)
      return false;
  if (a.port!=b.port)
      return false;
  if (a.user!=b.user)
      return false;
  if (a.account != b.account)
      return false;
  if (a.pass!=b.pass && a.user!=L"anonymous")
      return false;
  if (a.nServerType!=b.nServerType)
      return false;
  if (a.nPasv != b.nPasv)
      return false;
  if (a.nTimeZoneOffset != b.nTimeZoneOffset)
      return false;
  if (a.nUTF8 != b.nUTF8)
      return false;
  if (a.nCodePage != b.nCodePage)
    return false;
  if (a.iForcePasvIp != b.iForcePasvIp)
      return false;
  if (a.iUseMlsd != b.iUseMlsd)
      return false;
  return true;
}

const bool operator != (const t_server & a, const t_server & b)
{
  return !(a == b);
}

bool t_server::operator<(const t_server & op) const
{
  if (host<op.host)
    return true;
  if (port<op.port)
    return true;
  if (user<op.user)
    return true;
  if (account<op.account)
    return true;
  if (pass<op.pass)
    return true;
  if (nServerType<op.nServerType)
    return true;
  if (nPasv < op.nPasv)
    return true;
  if (nTimeZoneOffset < op.nTimeZoneOffset)
    return true;
  if (nUTF8 < op.nUTF8)
    return true;
  if (nCodePage < op.nCodePage)
    return true;
  if (iForcePasvIp < op.iForcePasvIp)
    return true;
  if (iUseMlsd < op.iUseMlsd)
    return true;

  return false;
}
