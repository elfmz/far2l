#pragma once

#include "farplug-wide.h"

#ifdef __cplusplus
extern "C" {
#endif

int WINAPI GetMinFarVersionW(void);
void WINAPI SetStartupInfoW(const PluginStartupInfo* info);
void WINAPI GetPluginInfoW(PluginInfo* info);
HANDLE WINAPI OpenPluginW(int open_from, INT_PTR item);
void WINAPI ClosePluginW(HANDLE hPlugin);

int WINAPI GetFindDataW(HANDLE hPlugin, PluginPanelItem** pPanelItem, int* pItemsNumber, int op_mode);
void WINAPI FreeFindDataW(HANDLE hPlugin, PluginPanelItem* panel_item, int items_number);
void WINAPI GetOpenPluginInfoW(HANDLE hPlugin, OpenPluginInfo* info);
int WINAPI ProcessKeyW(HANDLE hPlugin, int key, unsigned int control_state);
int WINAPI ProcessEventW(HANDLE hPlugin, int event, void* param);
int WINAPI SetDirectoryW(HANDLE hPlugin, const wchar_t* dir, int op_mode);

int WINAPI MakeDirectoryW(HANDLE hPlugin, const wchar_t** name, int op_mode);
int WINAPI DeleteFilesW(HANDLE hPlugin, PluginPanelItem* panel_item, int items_number, int op_mode);
int WINAPI GetFilesW(HANDLE hPlugin, PluginPanelItem* panel_item, int items_number, int move, const wchar_t** dest_path, int op_mode);
int WINAPI PutFilesW(HANDLE hPlugin, PluginPanelItem* panel_item, int items_number, int move, const wchar_t* src_path, int op_mode);

HANDLE WINAPI OpenFilePluginW(const wchar_t* name, const unsigned char* data, int data_size, int op_mode);
int WINAPI GetLinkTargetW(HANDLE hPlugin, PluginPanelItem* panel_item, wchar_t* target, size_t target_size, int op_mode);
int WINAPI ExecuteW(HANDLE hPlugin, PluginPanelItem* panel_item, int items_number, int op_mode);
int WINAPI ConfigureW(int item_number);
void WINAPI ExitFARW(void);
int WINAPI MayExitFARW(void);

#ifdef __cplusplus
}
#endif
