#pragma once

/*
Copyright (c) 1996 Eugene Roshal
Copyright (c) 2000 Far Group
All rights reserved.

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions
are met:
1. Redistributions of source code must retain the above copyright
   notice, this list of conditions and the following disclaimer.
2. Redistributions in binary form must reproduce the above copyright
   notice, this list of conditions and the following disclaimer in the
   documentation and/or other materials provided with the distribution.
3. The name of the authors may not be used to endorse or promote products
   derived from this software without specific prior written permission.

THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
(INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/

#include "farplug-wide.h"
#include "plclass.hpp"
#include "farplug-mb.h"

typedef void (WINAPI *PLUGINCLOSEPLUGIN)(HANDLE hPlugin);
typedef int (WINAPI *PLUGINCOMPARE)(HANDLE hPlugin,const oldfar::PluginPanelItem *Item1,const oldfar::PluginPanelItem *Item2,unsigned int Mode);
typedef int (WINAPI *PLUGINCONFIGURE)(int ItemNumber);
typedef int (WINAPI *PLUGINDELETEFILES)(HANDLE hPlugin,oldfar::PluginPanelItem *PanelItem,int ItemsNumber,int OpMode);
typedef void (WINAPI *PLUGINEXITFAR)();
typedef int (WINAPI *PLUGINMAYEXITFAR)();
typedef void (WINAPI *PLUGINFREEFINDDATA)(HANDLE hPlugin,oldfar::PluginPanelItem *PanelItem,int ItemsNumber);
typedef void (WINAPI *PLUGINFREEVIRTUALFINDDATA)(HANDLE hPlugin,oldfar::PluginPanelItem *PanelItem,int ItemsNumber);
typedef int (WINAPI *PLUGINGETFILES)(HANDLE hPlugin,oldfar::PluginPanelItem *PanelItem,int ItemsNumber,int Move,char *DestPath,int OpMode);
typedef int (WINAPI *PLUGINGETFINDDATA)(HANDLE hPlugin,oldfar::PluginPanelItem **pPanelItem,int *pItemsNumber,int OpMode);
typedef int (WINAPI *PLUGINMINFARVERSION)();
typedef void (WINAPI *PLUGINGETOPENPLUGININFO)(HANDLE hPlugin,oldfar::OpenPluginInfo *Info);
typedef void (WINAPI *PLUGINGETPLUGININFO)(oldfar::PluginInfo *Info);
typedef int (WINAPI *PLUGINGETVIRTUALFINDDATA)(HANDLE hPlugin,oldfar::PluginPanelItem **pPanelItem,int *pItemsNumber,const char *Path);
typedef int (WINAPI *PLUGINMAKEDIRECTORY)(HANDLE hPlugin,char *Name,int OpMode);
typedef HANDLE(WINAPI *PLUGINOPENFILEPLUGIN)(char *Name,const unsigned char *Data,int DataSize,int OpMode);
typedef HANDLE(WINAPI *PLUGINOPENPLUGIN)(int OpenFrom,INT_PTR Item);
typedef int (WINAPI *PLUGINPROCESSEDITOREVENT)(int Event,void *Param);
typedef int (WINAPI *PLUGINPROCESSEDITORINPUT)(const INPUT_RECORD *Rec);
typedef int (WINAPI *PLUGINPROCESSEVENT)(HANDLE hPlugin,int Event,void *Param);
typedef int (WINAPI *PLUGINPROCESSHOSTFILE)(HANDLE hPlugin,oldfar::PluginPanelItem *PanelItem,int ItemsNumber,int OpMode);
typedef int (WINAPI *PLUGINPROCESSKEY)(HANDLE hPlugin,int Key,unsigned int ControlState);
typedef int (WINAPI *PLUGINPUTFILES)(HANDLE hPlugin,oldfar::PluginPanelItem *PanelItem,int ItemsNumber,int Move,int OpMode);
typedef int (WINAPI *PLUGINSETDIRECTORY)(HANDLE hPlugin,const char *Dir,int OpMode);
typedef int (WINAPI *PLUGINSETFINDLIST)(HANDLE hPlugin,const oldfar::PluginPanelItem *PanelItem,int ItemsNumber);
typedef void (WINAPI *PLUGINSETSTARTUPINFO)(const oldfar::PluginStartupInfo *Info);
typedef int (WINAPI *PLUGINPROCESSVIEWEREVENT)(int Event,void *Param);
typedef int (WINAPI *PLUGINPROCESSDIALOGEVENT)(int Event,void *Param);


class PluginA: public Plugin
{
	private:
		FARString strRootKey;
		std::string mbRootKey;

		PluginInfo PI;
		OpenPluginInfo OPI;

		oldfar::PluginPanelItem  *pFDPanelItemA;
		oldfar::PluginPanelItem  *pVFDPanelItemA;

		PLUGINSETSTARTUPINFO        pSetStartupInfo;
		PLUGINOPENPLUGIN            pOpenPlugin;
		PLUGINOPENFILEPLUGIN        pOpenFilePlugin;
		PLUGINCLOSEPLUGIN           pClosePlugin;
		PLUGINGETPLUGININFO         pGetPluginInfo;
		PLUGINGETOPENPLUGININFO     pGetOpenPluginInfo;
		PLUGINGETFINDDATA           pGetFindData;
		PLUGINFREEFINDDATA          pFreeFindData;
		PLUGINGETVIRTUALFINDDATA    pGetVirtualFindData;
		PLUGINFREEVIRTUALFINDDATA   pFreeVirtualFindData;
		PLUGINSETDIRECTORY          pSetDirectory;
		PLUGINGETFILES              pGetFiles;
		PLUGINPUTFILES              pPutFiles;
		PLUGINDELETEFILES           pDeleteFiles;
		PLUGINMAKEDIRECTORY         pMakeDirectory;
		PLUGINPROCESSHOSTFILE       pProcessHostFile;
		PLUGINSETFINDLIST           pSetFindList;
		PLUGINCONFIGURE             pConfigure;
		PLUGINMAYEXITFAR            pMayExitFAR;
		PLUGINEXITFAR               pExitFAR;
		PLUGINPROCESSKEY            pProcessKey;
		PLUGINPROCESSEVENT          pProcessEvent;
		PLUGINPROCESSEDITOREVENT    pProcessEditorEvent;
		PLUGINCOMPARE               pCompare;
		PLUGINPROCESSEDITORINPUT    pProcessEditorInput;
		PLUGINMINFARVERSION         pMinFarVersion;
		PLUGINPROCESSVIEWEREVENT    pProcessViewerEvent;
		PLUGINPROCESSDIALOGEVENT    pProcessDialogEvent;

	public:

		PluginA(PluginManager *owner,
				const FARString &strModuleName,
				const std::string &settingsName,
				const std::string &moduleID);
		~PluginA();

		bool IsOemPlugin() {return true;}

		bool Load();
		bool LoadFromCache();

		bool SaveToCache();

		int Unload(bool bExitFAR = false);

		bool IsPanelPlugin();

		bool HasOpenPlugin() { return pOpenPlugin!=nullptr; }
		bool HasMakeDirectory() { return pMakeDirectory!=nullptr; }
		bool HasDeleteFiles() { return pDeleteFiles!=nullptr; }
		bool HasPutFiles() { return pPutFiles!=nullptr; }
		bool HasGetFiles() { return pGetFiles!=nullptr; }
		bool HasSetStartupInfo() { return pSetStartupInfo!=nullptr; }
		bool HasOpenFilePlugin() { return pOpenFilePlugin!=nullptr; }
		bool HasClosePlugin() { return pClosePlugin!=nullptr; }
		bool HasGetPluginInfo() { return pGetPluginInfo!=nullptr; }
		bool HasGetOpenPluginInfo() { return pGetOpenPluginInfo!=nullptr; }
		bool HasGetFindData() { return pGetFindData!=nullptr; }
		bool HasFreeFindData() { return pFreeFindData!=nullptr; }
		bool HasGetVirtualFindData() { return pGetVirtualFindData!=nullptr; }
		bool HasFreeVirtualFindData() { return pFreeVirtualFindData!=nullptr; }
		bool HasSetDirectory() { return pSetDirectory!=nullptr; }
		bool HasProcessHostFile() { return pProcessHostFile!=nullptr; }
		bool HasSetFindList() { return pSetFindList!=nullptr; }
		bool HasConfigure() { return pConfigure!=nullptr; }
		bool HasMayExitFAR() { return pMayExitFAR!=nullptr; }
		bool HasExitFAR() { return pExitFAR!=nullptr; }
		bool HasProcessKey() { return pProcessKey!=nullptr; }
		bool HasProcessEvent() { return pProcessEvent!=nullptr; }
		bool HasProcessEditorEvent() { return pProcessEditorEvent!=nullptr; }
		bool HasCompare() { return pCompare!=nullptr; }
		bool HasProcessEditorInput() { return pProcessEditorInput!=nullptr; }
		bool HasMinFarVersion() { return pMinFarVersion!=nullptr; }
		bool HasProcessViewerEvent() { return pProcessViewerEvent!=nullptr; }
		bool HasProcessDialogEvent() { return pProcessDialogEvent!=nullptr; }
		bool HasProcessSynchroEvent() { return false; }
#if defined(PROCPLUGINMACROFUNC)
		bool HasProcessMacroFunc() { return false; }
#endif
		bool HasAnalyse() { return false; }
		bool HasGetCustomData()  { return false; }
		bool HasFreeCustomData() { return false; }

		const FARString &GetModuleName() { return m_strModuleName; }
		const char *GetSettingsName() { return m_strSettingsName.c_str(); }
		DWORD GetSysID() { return SysID; }
		bool CheckWorkFlags(DWORD flags) { return WorkFlags.Check(flags)==TRUE; }
		DWORD GetWorkFlags() { return WorkFlags.Flags; }
		DWORD GetFuncFlags() { return FuncFlags.Flags; }

		bool InitLang(const wchar_t *Path) { return Lang.Init(Path,false); }
		void CloseLang() { Lang.Close(); }
		const char *GetMsgA(int nID) { return Lang.GetMsgMB(nID); }

	public:

		bool SetStartupInfo(bool &bUnloaded);
		bool CheckMinFarVersion(bool &bUnloaded);

		HANDLE OpenPlugin(int OpenFrom, INT_PTR Item);
		HANDLE OpenFilePlugin(const wchar_t *Name, const unsigned char *Data, int DataSize, int OpMode);

		int SetFindList(HANDLE hPlugin, const PluginPanelItem *PanelItem, int ItemsNumber);
		int GetFindData(HANDLE hPlugin, PluginPanelItem **pPanelItem, int *pItemsNumber, int OpMode);
		int GetVirtualFindData(HANDLE hPlugin, PluginPanelItem **pPanelItem, int *pItemsNumber, const wchar_t *Path);
		int SetDirectory(HANDLE hPlugin, const wchar_t *Dir, int OpMode);
		int GetFiles(HANDLE hPlugin, PluginPanelItem *PanelItem, int ItemsNumber, int Move, const wchar_t **DestPath, int OpMode);
		int PutFiles(HANDLE hPlugin, PluginPanelItem *PanelItem, int ItemsNumber, int Move, int OpMode);
		int DeleteFiles(HANDLE hPlugin, PluginPanelItem *PanelItem, int ItemsNumber, int OpMode);
		int MakeDirectory(HANDLE hPlugin, const wchar_t **Name, int OpMode);
		int ProcessHostFile(HANDLE hPlugin, PluginPanelItem *PanelItem, int ItemsNumber, int OpMode);
		int ProcessKey(HANDLE hPlugin, int Key, unsigned int dwControlState);
		int ProcessEvent(HANDLE hPlugin, int Event, PVOID Param);
		int Compare(HANDLE hPlugin, const PluginPanelItem *Item1, const PluginPanelItem *Item2, DWORD Mode);

		int GetCustomData(const wchar_t *FilePath, wchar_t **CustomData) { return 0; }
		void FreeCustomData(wchar_t *CustomData) {}

		void GetOpenPluginInfo(HANDLE hPlugin, OpenPluginInfo *Info);
		void FreeFindData(HANDLE hPlugin, PluginPanelItem *PanelItem, int ItemsNumber);
		void FreeVirtualFindData(HANDLE hPlugin, PluginPanelItem *PanelItem, int ItemsNumber);
		void ClosePlugin(HANDLE hPlugin);

		int ProcessEditorInput(const INPUT_RECORD *D);
		int ProcessEditorEvent(int Event, PVOID Param);
		int ProcessViewerEvent(int Event, PVOID Param);
		int ProcessDialogEvent(int Event, PVOID Param);
		int ProcessSynchroEvent(int Event, PVOID Param) { return 0; }
#if defined(PROCPLUGINMACROFUNC)
		int ProcessMacroFunc(const wchar_t *Name, const FarMacroValue *Params, int nParams, FarMacroValue **Results, int *nResults) {return 0;}
#endif

		int Analyse(const AnalyseData *pData) { return FALSE; }

		bool GetPluginInfo(PluginInfo *pi);
		int Configure(int MenuItem);

		bool MayExitFAR();
		void ExitFAR();

	private:

		void ClearExports();

		void FreePluginInfo();
		void ConvertPluginInfo(oldfar::PluginInfo &Src, PluginInfo *Dest);
		void FreeOpenPluginInfo();
		void ConvertOpenPluginInfo(oldfar::OpenPluginInfo &Src, OpenPluginInfo *Dest);
};
