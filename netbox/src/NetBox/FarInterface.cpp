#include <vcl.h>
#pragma hdrstop

#include <Common.h>
#include <Queue.h>
#include <MsgIDs.h>

#include "CoreMain.h"
#include "FarConfiguration.h"
#include "WinSCPPlugin.h"
#include "FarDialog.h"
#include "FarInterface.h"

TConfiguration * CreateConfiguration()
{
  return new TFarConfiguration(FarPlugin);
}

void ShowExtendedException(Exception * E)
{
  DebugAssert(FarPlugin != nullptr);
  TWinSCPPlugin * WinSCPPlugin = NB_STATIC_DOWNCAST(TWinSCPPlugin, FarPlugin);
  DebugAssert(WinSCPPlugin != nullptr);
  WinSCPPlugin->ShowExtendedException(E);
}

UnicodeString GetAppNameString()
{
  return "NetBox";
}

UnicodeString GetRegistryKey()
{
  return "Software" WGOOD_SLASH "Far2" WGOOD_SLASH "Plugins" WGOOD_SLASH "NetBox 2";
}

void Busy(bool /*Start*/)
{
  // nothing
}

UnicodeString GetSshVersionString()
{
  return FORMAT(L"NetBox-FAR-release-%s", GetConfiguration()->GetProductVersion().c_str());
}

DWORD WINAPI threadstartroutine(void * Parameter)
{
  TSimpleThread * SimpleThread = NB_STATIC_DOWNCAST(TSimpleThread, Parameter);
  return TSimpleThread::ThreadProc(SimpleThread);
}

HANDLE BeginThread(void * SecurityAttributes, DWORD StackSize,
  void * Parameter, DWORD CreationFlags,
  DWORD & ThreadId)
{
  HANDLE Result = ::CreateThread(static_cast<LPSECURITY_ATTRIBUTES>(SecurityAttributes),
    static_cast<size_t>(StackSize),
    static_cast<LPTHREAD_START_ROUTINE>(&threadstartroutine),
    Parameter,
    CreationFlags, &ThreadId);
  return Result;
}

HANDLE StartThread(void * SecurityAttributes, DWORD StackSize,
  void * Parameter, DWORD CreationFlags,
  TThreadID & ThreadId)
{
  return BeginThread(SecurityAttributes, StackSize, Parameter,
    CreationFlags, ThreadId);
}

void CopyToClipboard(const UnicodeString & AText)
{
  DebugAssert(FarPlugin != nullptr);
  FarPlugin->FarCopyToClipboard(AText);
}

//from windows/GUITools.cpp
template<class TEditControl>
void ValidateMaskEditT(const UnicodeString & Mask, TEditControl * Edit, int ForceDirectoryMasks)
{
  DebugAssert(Edit != nullptr);
  TFileMasks Masks(ForceDirectoryMasks);
  try
  {
    Masks = Mask;
  }
  catch (EFileMasksException & E)
  {
    ShowExtendedException(&E);
    Edit->SetFocus();
    // This does not work for TEdit and TMemo (descendants of TCustomEdit) anymore,
    // as it re-selects whole text on exception in TCustomEdit.CMExit
//    Edit->SelStart = E.ErrorStart - 1;
//    Edit->SelLength = E.ErrorLen;
    Abort();
  }
}

void ValidateMaskEdit(TFarComboBox * Edit)
{
  ValidateMaskEditT(Edit->GetText(), Edit, -1);
}

void ValidateMaskEdit(TFarEdit * Edit)
{
  ValidateMaskEditT(Edit->GetText(), Edit, -1);
}
