#include "stdafx.h"
#include "afxdll.h"

HINSTANCE HInst = NULL;

void InitExtensionModule(HINSTANCE HInst)
{
  ::HInst = HInst;
  AFX_MANAGE_STATE(AfxGetModuleState());
  afxCurrentResourceHandle = ::HInst;
}

void TermExtensionModule()
{
}
