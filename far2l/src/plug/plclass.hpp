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

#include <farplug-wide.h>
#include "language.hpp"
#include "bitflags.hpp"
#include "FARString.hpp"
#include <string>

struct AnalyseData
{
	int StructSize;
	const wchar_t *lpwszFileName;
	const unsigned char *pBuffer;
	DWORD dwBufferSize;
	int OpMode;
};


class PluginManager;

class Plugin
{
		void *m_hModule = nullptr;
		void *GetModulePFN(const char *fn);

	protected:
		PluginManager *m_owner; //BUGBUG

		FARString m_strModuleName;
		std::string m_strSettingsName;
		std::string m_strModuleID;
		FARString strRootKey;

		BitFlags WorkFlags{};      // рабочие флаги текущего плагина
		BitFlags FuncFlags{};      // битовые маски вызова эксп.функций плагина

		bool m_Loaded = false;

		/* $ 21.09.2000 SVS
		   поле - системный идентификатор плагина
		   Плагин должен сам задавать, например для
		   Network      = 0x5774654E (NetW)
		   PrintManager = 0x6E614D50 (PMan)  SYSID_PRINTMANAGER
		*/
		DWORD SysID = 0;

		Language Lang;

		bool OpenModule();
		void CloseModule();

		template <class TFN>
			void GetModuleFN(TFN &fn, const char *api)
		{
			fn = (TFN)GetModulePFN(api);
		}

	public:
		Plugin(PluginManager *owner,
			const FARString &strModuleName,
			const std::string &settingsName,
			const std::string &moduleID);

		virtual ~Plugin();

		virtual bool IsOemPlugin() = 0;

		virtual bool Load() = 0;
		virtual bool LoadFromCache() = 0;

		virtual bool SaveToCache() = 0;

		virtual int Unload(bool bExitFAR = false) = 0;

		virtual bool IsPanelPlugin() = 0;

		virtual bool HasOpenPlugin() = 0;
		virtual bool HasMakeDirectory() = 0;
		virtual bool HasDeleteFiles() = 0;
		virtual bool HasPutFiles() = 0;
		virtual bool HasGetFiles() = 0;
		virtual bool HasSetStartupInfo() = 0;
		virtual bool HasOpenFilePlugin() = 0;
		virtual bool HasClosePlugin() = 0;
		virtual bool HasGetPluginInfo() = 0;
		virtual bool HasGetOpenPluginInfo() = 0;
		virtual bool HasGetFindData() = 0;
		virtual bool HasFreeFindData() = 0;
		virtual bool HasGetVirtualFindData() = 0;
		virtual bool HasFreeVirtualFindData() = 0;
		virtual bool HasSetDirectory() = 0;
		virtual bool HasProcessHostFile() = 0;
		virtual bool HasSetFindList() = 0;
		virtual bool HasConfigure() = 0;
		virtual bool HasMayExitFAR() = 0;
		virtual bool HasExitFAR() = 0;
		virtual bool HasProcessKey() = 0;
		virtual bool HasProcessEvent() = 0;
		virtual bool HasProcessEditorEvent() = 0;
		virtual bool HasCompare() = 0;
		virtual bool HasProcessEditorInput() = 0;
		virtual bool HasMinFarVersion() = 0;
		virtual bool HasProcessViewerEvent() = 0;
		virtual bool HasProcessDialogEvent() = 0;
		virtual bool HasProcessSynchroEvent() = 0;
		virtual bool HasAnalyse() = 0;
		virtual bool HasGetCustomData() = 0;
		virtual bool HasFreeCustomData() = 0;
#if defined(PROCPLUGINMACROFUNC)
		virtual bool HasProcessMacroFunc() = 0;
#endif

		virtual const FARString &GetModuleName() = 0;
		virtual const char *GetSettingsName() = 0;
		virtual DWORD GetSysID() = 0;
		virtual bool CheckWorkFlags(DWORD flags) = 0;
		virtual DWORD GetWorkFlags() = 0;
		virtual DWORD GetFuncFlags() = 0;

		virtual bool InitLang(const wchar_t *Path) = 0;
		virtual void CloseLang() = 0;

		virtual bool SetStartupInfo(bool &bUnloaded) = 0;
		virtual bool CheckMinFarVersion(bool &bUnloaded) = 0;

		virtual HANDLE OpenPlugin(int OpenFrom, INT_PTR Item) = 0;
		virtual HANDLE OpenFilePlugin(const wchar_t *Name, const unsigned char *Data, int DataSize, int OpMode) = 0;

		virtual int SetFindList(HANDLE hPlugin, const PluginPanelItem *PanelItem, int ItemsNumber) = 0;
		virtual int GetFindData(HANDLE hPlugin, PluginPanelItem **pPanelItem, int *pItemsNumber, int OpMode) = 0;
		virtual int GetVirtualFindData(HANDLE hPlugin, PluginPanelItem **pPanelItem, int *pItemsNumber, const wchar_t *Path) = 0;
		virtual int SetDirectory(HANDLE hPlugin, const wchar_t *Dir, int OpMode) = 0;
		virtual int GetFiles(HANDLE hPlugin, PluginPanelItem *PanelItem, int ItemsNumber, int Move, const wchar_t **DestPath, int OpMode) = 0;
		virtual int PutFiles(HANDLE hPlugin, PluginPanelItem *PanelItem, int ItemsNumber, int Move, int OpMode) = 0;
		virtual int DeleteFiles(HANDLE hPlugin, PluginPanelItem *PanelItem, int ItemsNumber, int OpMode) = 0;
		virtual int MakeDirectory(HANDLE hPlugin, const wchar_t **Name, int OpMode) = 0;
		virtual int ProcessHostFile(HANDLE hPlugin, PluginPanelItem *PanelItem, int ItemsNumber, int OpMode) = 0;
		virtual int ProcessKey(HANDLE hPlugin, int Key, unsigned int dwControlState) = 0;
		virtual int ProcessEvent(HANDLE hPlugin, int Event, PVOID Param) = 0;
		virtual int Compare(HANDLE hPlugin, const PluginPanelItem *Item1, const PluginPanelItem *Item2, DWORD Mode) = 0;

		virtual int GetCustomData(const wchar_t *FilePath, wchar_t **CustomData) = 0;
		virtual void FreeCustomData(wchar_t *CustomData) = 0;

		virtual void GetOpenPluginInfo(HANDLE hPlugin, OpenPluginInfo *Info) = 0;
		virtual void FreeFindData(HANDLE hPlugin, PluginPanelItem *PanelItem, int ItemsNumber) = 0;
		virtual void FreeVirtualFindData(HANDLE hPlugin, PluginPanelItem *PanelItem, int ItemsNumber) = 0;
		virtual void ClosePlugin(HANDLE hPlugin) = 0;

		virtual int ProcessEditorInput(const INPUT_RECORD *D) = 0;
		virtual int ProcessEditorEvent(int Event, PVOID Param) = 0;
		virtual int ProcessViewerEvent(int Event, PVOID Param) = 0;
		virtual int ProcessDialogEvent(int Event, PVOID Param) = 0;
		virtual int ProcessSynchroEvent(int Event, PVOID Param) = 0;
#if defined(PROCPLUGINMACROFUNC)
		virtual int ProcessMacroFunc(const wchar_t *Name, const FarMacroValue *Params, int nParams, FarMacroValue **Results, int *nResults) = 0;
#endif

		virtual int Analyse(const AnalyseData *pData) = 0;

		virtual bool GetPluginInfo(PluginInfo *pi) = 0;
		virtual int Configure(int MenuItem) = 0;

		virtual bool MayExitFAR() = 0;
		virtual void ExitFAR() = 0;
};
