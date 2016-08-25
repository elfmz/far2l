#ifndef __linux__
#include "afxdll.h"
#endif
#include <vcl.h>

#include <Common.h>
#include <Sysutils.hpp>
#include <MsgIDs.h>

#include "FarUtil.h"
#include "resource.h"

extern TCustomFarPlugin * CreateFarPlugin(HINSTANCE HInst);

class TFarPluginGuard : public TFarPluginEnvGuard, public TGuard
{
public:
  inline TFarPluginGuard() :
    TGuard(FarPlugin->GetCriticalSection())
  {
  }
};

extern "C"
{

int WINAPI GetMinFarVersionW()
{
  DebugAssert(FarPlugin);
  TFarPluginGuard Guard;
  return static_cast<int>(FarPlugin->GetMinFarVersion());
}

void WINAPI SetStartupInfoW(const struct PluginStartupInfo * psi)
{
  DebugAssert(FarPlugin);
  TFarPluginGuard Guard;
  FarPlugin->SetStartupInfo(psi);
}

void WINAPI ExitFARW()
{
  DebugAssert(FarPlugin);
  TFarPluginGuard Guard;
  FarPlugin->ExitFAR();
}

void WINAPI GetPluginInfoW(PluginInfo * pi)
{
  DebugAssert(FarPlugin);
  TFarPluginGuard Guard;
  FarPlugin->GetPluginInfo(pi);
}

int WINAPI ConfigureW(int item)
{
  DebugAssert(FarPlugin);
  TFarPluginGuard Guard;
  return static_cast<int>(FarPlugin->Configure(static_cast<intptr_t>(item)));
}

HANDLE WINAPI OpenPluginW(int openFrom, intptr_t item)
{
  DebugAssert(FarPlugin);
  TFarPluginGuard Guard;
  return FarPlugin->OpenPlugin(openFrom, item);
}

void WINAPI ClosePluginW(HANDLE Plugin)
{
  DebugAssert(FarPlugin);
  TFarPluginGuard Guard;
  FarPlugin->ClosePlugin(Plugin);
}

void WINAPI GetOpenPluginInfoW(HANDLE Plugin, OpenPluginInfo * pluginInfo)
{
  DebugAssert(FarPlugin);
  TFarPluginGuard Guard;
  FarPlugin->GetOpenPluginInfo(Plugin, pluginInfo);
}

int WINAPI GetFindDataW(HANDLE Plugin, PluginPanelItem ** PanelItem, int * itemsNumber, int OpMode)
{
  DebugAssert(FarPlugin);
  TFarPluginGuard Guard;
  return static_cast<int>(FarPlugin->GetFindData(Plugin, PanelItem, itemsNumber, OpMode));
}

void WINAPI FreeFindDataW(HANDLE Plugin, PluginPanelItem * PanelItem, int itemsNumber)
{
  DebugAssert(FarPlugin);
  TFarPluginGuard Guard;
  FarPlugin->FreeFindData(Plugin, PanelItem, itemsNumber);
}

int WINAPI ProcessHostFileW(HANDLE Plugin,
  struct PluginPanelItem * PanelItem, int ItemsNumber, int OpMode)
{
  DebugAssert(FarPlugin);
  TFarPluginGuard Guard;
  return static_cast<int>(FarPlugin->ProcessHostFile(Plugin, PanelItem, ItemsNumber, OpMode));
}

int WINAPI ProcessKeyW(HANDLE Plugin, int key, unsigned int controlState)
{
  DebugAssert(FarPlugin);
  TFarPluginGuard Guard;
  return static_cast<int>(FarPlugin->ProcessKey(Plugin, key, controlState));
}

int WINAPI ProcessEventW(HANDLE Plugin, int Event, void * Param)
{
  DebugAssert(FarPlugin);
  TFarPluginGuard Guard;
  return static_cast<int>(FarPlugin->ProcessEvent(Plugin, Event, Param));
}

int WINAPI SetDirectoryW(HANDLE Plugin, const wchar_t * Dir, int OpMode)
{
  DebugAssert(FarPlugin);
  TFarPluginGuard Guard;
  int Result = static_cast<int>(FarPlugin->SetDirectory(Plugin, Dir, OpMode));
  return Result;
}

int WINAPI MakeDirectoryW(HANDLE Plugin, const wchar_t ** Name, int OpMode)
{
  DebugAssert(FarPlugin);
  TFarPluginGuard Guard;
  int Result = static_cast<int>(FarPlugin->MakeDirectory(Plugin, Name, OpMode));
  return Result;
}

int WINAPI DeleteFilesW(HANDLE Plugin, PluginPanelItem * PanelItem, int itemsNumber, int OpMode)
{
  DebugAssert(FarPlugin);
  TFarPluginGuard Guard;
  return static_cast<int>(FarPlugin->DeleteFiles(Plugin, PanelItem, itemsNumber, OpMode));
}

int WINAPI GetFilesW(HANDLE Plugin, PluginPanelItem * PanelItem, int itemsNumber,
  int Move, const wchar_t ** destPath, int OpMode)
{
  DebugAssert(FarPlugin);
  TFarPluginGuard Guard;
  return static_cast<int>(FarPlugin->GetFiles(Plugin, PanelItem, itemsNumber,
    Move, destPath, OpMode));
}

int WINAPI PutFilesW(HANDLE Plugin, PluginPanelItem * PanelItem, int itemsNumber, int Move, const wchar_t * SrcPath, int OpMode)
{
  DebugAssert(FarPlugin);
  TFarPluginGuard Guard;
  int Result = static_cast<int>(FarPlugin->PutFiles(Plugin, PanelItem, itemsNumber,
    Move, SrcPath, OpMode));
  return Result;
}

int WINAPI ProcessEditorEventW(int Event, void * Param)
{
  DebugAssert(FarPlugin);
  TFarPluginGuard Guard;
  return static_cast<int>(FarPlugin->ProcessEditorEvent(Event, Param));
}

int WINAPI ProcessEditorInputW(const INPUT_RECORD * Rec)
{
  DebugAssert(FarPlugin);
  TFarPluginGuard Guard;
  return static_cast<int>(FarPlugin->ProcessEditorInput(Rec));
}

HANDLE WINAPI OpenFilePluginW(const wchar_t * fileName, const uint8_t * fileHeader, int fileHeaderSize, int /*OpMode*/)
{
  DebugAssert(FarPlugin);
  TFarPluginGuard Guard;
  if (!fileName)
  {
    return INVALID_HANDLE_VALUE;
  }

  const size_t fileNameLen = wcslen(fileName);
#ifndef __linux__
  if (fileNameLen < 8 || _wcsicmp(fileName + fileNameLen - 7, L".netbox") != 0)
#else
  if (fileNameLen < 8 || wcscmp(fileName + fileNameLen - 7, L".netbox") != 0)
#endif
  {
    return INVALID_HANDLE_VALUE;
  }
  if (fileHeaderSize > 4 && strncmp(reinterpret_cast<const char *>(fileHeader), "<?xml", 5) != 0)
  {
    return INVALID_HANDLE_VALUE;
  }
  HANDLE Handle = static_cast<HANDLE>(FarPlugin->OpenPlugin(OPEN_ANALYSE,
    reinterpret_cast<intptr_t>(fileName)));
  return Handle;
}

#ifndef __linux__
  static int Processes = 0;

BOOL DllProcessAttach(HINSTANCE HInstance)
{
  FarPlugin = CreateFarPlugin(HInstance);

  DebugAssert(!Processes);
  Processes++;
  InitExtensionModule(HInstance);
  return TRUE;
}

BOOL DllProcessDetach()
{
  DebugAssert(Processes);
  Processes--;
  if (!Processes)
  {
    DebugAssert(FarPlugin);
    SAFE_DESTROY(FarPlugin);
    TermExtensionModule();
  }
  return TRUE;
}

BOOL WINAPI DllMain(HINSTANCE HInstance, DWORD Reason, LPVOID /*ptr*/ )
{
  BOOL Result = TRUE;
  switch (Reason)
  {
    case DLL_PROCESS_ATTACH:
      Result = DllProcessAttach(HInstance);
      break;

    case DLL_PROCESS_DETACH:
      Result = DllProcessDetach();
      break;
  }
  return Result;
}
#else
__attribute__((constructor)) void so_init(void)
{
  FarPlugin = CreateFarPlugin(0);
}
#endif

} // extern "C"
