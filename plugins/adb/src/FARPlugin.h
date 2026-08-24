#ifndef FARPLUGIN_H
#define FARPLUGIN_H

#include "farplug-wide.h"

#ifdef __cplusplus
extern "C" {
#endif

// https://api.farmanager.com/ru/panelapi/index.html

int    WINAPI GetMinFarVersionW(void);
void   WINAPI SetStartupInfoW(const PluginStartupInfo *Info);
void   WINAPI GetPluginInfoW(PluginInfo *Info);
HANDLE WINAPI OpenPluginW(int OpenFrom, INT_PTR Item);
void   WINAPI ClosePluginW(HANDLE hPlugin);
int    WINAPI GetFindDataW(HANDLE hPlugin, PluginPanelItem **pPanelItem, int *pItemsNumber, int OpMode);
void   WINAPI FreeFindDataW(HANDLE hPlugin, PluginPanelItem *PanelItem, int ItemsNumber);
void   WINAPI GetOpenPluginInfoW(HANDLE hPlugin, OpenPluginInfo *Info);
int    WINAPI ProcessKeyW(HANDLE hPlugin, int Key, unsigned int ControlState);
int    WINAPI ProcessEventW(HANDLE hPlugin, int Event, void *Param);
int    WINAPI SetDirectoryW(HANDLE hPlugin, const wchar_t *Dir, int OpMode);
int    WINAPI MakeDirectoryW(HANDLE hPlugin, const wchar_t **Name, int OpMode);
int    WINAPI DeleteFilesW(HANDLE hPlugin, PluginPanelItem *PanelItem, int ItemsNumber, int OpMode);
int    WINAPI GetFilesW(HANDLE hPlugin, PluginPanelItem *PanelItem, int ItemsNumber, int Move, const wchar_t **DestPath, int OpMode);
HANDLE WINAPI OpenFilePluginW(const wchar_t *Name, const unsigned char *Data, int DataSize, int OpMode);
int    WINAPI PutFilesW(HANDLE hPlugin, PluginPanelItem *PanelItem, int ItemsNumber, int Move, const wchar_t *SrcPath, int OpMode);
int    WINAPI ProcessHostFileW(HANDLE hPlugin, PluginPanelItem *PanelItem, int ItemsNumber, int OpMode);
int    WINAPI GetLinkTargetW(HANDLE hPlugin, PluginPanelItem *PanelItem, wchar_t *Target, size_t TargetSize, int OpMode);
int    WINAPI ExecuteW(HANDLE hPlugin, PluginPanelItem *PanelItem, int ItemsNumber, int OpMode);
int    WINAPI ConfigureW(int ItemNumber);
void   WINAPI ExitFARW(void);
int    WINAPI MayExitFARW(void);

#ifdef __cplusplus
}
#endif

#endif // FARPLUGIN_H
