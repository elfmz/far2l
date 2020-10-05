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

#include "headers.hpp"


#include "plugins.hpp"
#include "plugapi.hpp"
#include "lang.hpp"
#include "keys.hpp"
#include "scantree.hpp"
#include "chgprior.hpp"
#include "constitle.hpp"
#include "cmdline.hpp"
#include "filepanels.hpp"
#include "panel.hpp"
#include "vmenu.hpp"
#include "dialog.hpp"
#include "rdrwdsk.hpp"
#include "savescr.hpp"
#include "ctrlobj.hpp"
#include "scrbuf.hpp"
#include "udlist.hpp"
#include "fileedit.hpp"
#include "RefreshFrameManager.hpp"
#include "InterThreadCall.hpp"
#include "plclass.hpp"
#include "PluginW.hpp"
#include "registry.hpp"
#include "keyboard.hpp"
#include "message.hpp"
#include "clipboard.hpp"
#include "xlat.hpp"
#include "pathmix.hpp"
#include "dirmix.hpp"
#include "strmix.hpp"
#include "processname.hpp"
#include "mix.hpp"
#include "interf.hpp"
#include "lasterror.hpp"
#include "execute.hpp"
#include "flink.hpp"
#include <string>
#include <list>
#include <vector>

static const wchar_t *wszReg_Preload=L"Preload";
static const wchar_t *wszReg_SysID=L"SysID";

static const wchar_t wszReg_OpenPlugin[]=L"OpenPluginW";
static const wchar_t wszReg_OpenFilePlugin[]=L"OpenFilePluginW";
static const wchar_t wszReg_SetFindList[]=L"SetFindListW";
static const wchar_t wszReg_ProcessEditorInput[]=L"ProcessEditorInputW";
static const wchar_t wszReg_ProcessEditorEvent[]=L"ProcessEditorEventW";
static const wchar_t wszReg_ProcessViewerEvent[]=L"ProcessViewerEventW";
static const wchar_t wszReg_ProcessDialogEvent[]=L"ProcessDialogEventW";
static const wchar_t wszReg_ProcessSynchroEvent[]=L"ProcessSynchroEventW";
#if defined(PROCPLUGINMACROFUNC)
static const wchar_t wszReg_ProcessMacroFunc[]=L"ProcessMacroFuncW";
#endif
static const wchar_t wszReg_SetStartupInfo[]=L"SetStartupInfoW";
static const wchar_t wszReg_ClosePlugin[]=L"ClosePluginW";
static const wchar_t wszReg_GetPluginInfo[]=L"GetPluginInfoW";
static const wchar_t wszReg_GetOpenPluginInfo[]=L"GetOpenPluginInfoW";
static const wchar_t wszReg_GetFindData[]=L"GetFindDataW";
static const wchar_t wszReg_FreeFindData[]=L"FreeFindDataW";
static const wchar_t wszReg_GetVirtualFindData[]=L"GetVirtualFindDataW";
static const wchar_t wszReg_FreeVirtualFindData[]=L"FreeVirtualFindDataW";
static const wchar_t wszReg_SetDirectory[]=L"SetDirectoryW";
static const wchar_t wszReg_GetFiles[]=L"GetFilesW";
static const wchar_t wszReg_PutFiles[]=L"PutFilesW";
static const wchar_t wszReg_DeleteFiles[]=L"DeleteFilesW";
static const wchar_t wszReg_MakeDirectory[]=L"MakeDirectoryW";
static const wchar_t wszReg_ProcessHostFile[]=L"ProcessHostFileW";
static const wchar_t wszReg_Configure[]=L"ConfigureW";
static const wchar_t wszReg_MayExitFAR[]=L"MayExitFARW";
static const wchar_t wszReg_ExitFAR[]=L"ExitFARW";
static const wchar_t wszReg_ProcessKey[]=L"ProcessKeyW";
static const wchar_t wszReg_ProcessEvent[]=L"ProcessEventW";
static const wchar_t wszReg_Compare[]=L"CompareW";
static const wchar_t wszReg_GetMinFarVersion[]=L"GetMinFarVersionW";
static const wchar_t wszReg_Analyse[] = L"AnalyseW";
static const wchar_t wszReg_GetCustomData[] = L"GetCustomDataW";
static const wchar_t wszReg_FreeCustomData[] = L"FreeCustomDataW";

static const char NFMP_OpenPlugin[]="OpenPluginW";
static const char NFMP_OpenFilePlugin[]="OpenFilePluginW";
static const char NFMP_SetFindList[]="SetFindListW";
static const char NFMP_ProcessEditorInput[]="ProcessEditorInputW";
static const char NFMP_ProcessEditorEvent[]="ProcessEditorEventW";
static const char NFMP_ProcessViewerEvent[]="ProcessViewerEventW";
static const char NFMP_ProcessDialogEvent[]="ProcessDialogEventW";
static const char NFMP_ProcessSynchroEvent[]="ProcessSynchroEventW";
#if defined(PROCPLUGINMACROFUNC)
static const char NFMP_ProcessMacroFunc[]="ProcessMacroFuncW";
#endif
static const char NFMP_SetStartupInfo[]="SetStartupInfoW";
static const char NFMP_ClosePlugin[]="ClosePluginW";
static const char NFMP_GetPluginInfo[]="GetPluginInfoW";
static const char NFMP_GetOpenPluginInfo[]="GetOpenPluginInfoW";
static const char NFMP_GetFindData[]="GetFindDataW";
static const char NFMP_FreeFindData[]="FreeFindDataW";
static const char NFMP_GetVirtualFindData[]="GetVirtualFindDataW";
static const char NFMP_FreeVirtualFindData[]="FreeVirtualFindDataW";
static const char NFMP_SetDirectory[]="SetDirectoryW";
static const char NFMP_GetFiles[]="GetFilesW";
static const char NFMP_PutFiles[]="PutFilesW";
static const char NFMP_DeleteFiles[]="DeleteFilesW";
static const char NFMP_MakeDirectory[]="MakeDirectoryW";
static const char NFMP_ProcessHostFile[]="ProcessHostFileW";
static const char NFMP_Configure[]="ConfigureW";
static const char NFMP_MayExitFAR[]="MayExitFARW";
static const char NFMP_ExitFAR[]="ExitFARW";
static const char NFMP_ProcessKey[]="ProcessKeyW";
static const char NFMP_ProcessEvent[]="ProcessEventW";
static const char NFMP_Compare[]="CompareW";
static const char NFMP_GetMinFarVersion[]="GetMinFarVersionW";
static const char NFMP_Analyse[]="AnalyseW";
static const char NFMP_GetCustomData[]="GetCustomDataW";
static const char NFMP_FreeCustomData[]="FreeCustomDataW";


static BOOL PrepareModulePath(const wchar_t *ModuleName)
{
	FARString strModulePath;
	strModulePath = ModuleName;
	CutToSlash(strModulePath); //??
	return FarChDir(strModulePath);
}

static void CheckScreenLock()
{
	if (ScrBuf.GetLockCount() > 0 && !CtrlObject->Macro.PeekKey())
	{
		ScrBuf.SetLockCount(0);
		ScrBuf.Flush();
	}
}

static size_t WINAPI FarKeyToName(int Key,wchar_t *KeyText,size_t Size)
{
	FARString strKT;

	if (!KeyToText(Key,strKT))
		return 0;

	size_t len = strKT.GetLength();

	if (Size && KeyText)
	{
		if (Size <= len) len = Size-1;

		wmemcpy(KeyText, strKT.CPtr(), len);
		KeyText[len] = 0;
	}
	else if (KeyText) *KeyText = 0;

	return (len+1);
}

int WINAPI KeyNameToKeyW(const wchar_t *Name)
{
	FARString strN(Name);
	return (int)KeyNameToKey(strN);
}

PluginW::PluginW(PluginManager *owner, const wchar_t *lpwszModuleName):
	m_owner(owner),
	m_strModuleName(lpwszModuleName),
	m_strCacheName(lpwszModuleName),
	m_hModule(nullptr)
	//more initialization here!!!
{
	ClearExports();
}

PluginW::~PluginW()
{
	Lang.Close();
}

bool PluginW::LoadFromCache(const FAR_FIND_DATA_EX &FindData)
{
	FARString strRegKey;
	strRegKey.Format(FmtPluginsCache_PluginS, m_strCacheName.CPtr());

	strRegKey += L"/Exports";

	if (CheckRegKey(strRegKey))
	{
		if (GetRegKey(strRegKey,wszReg_Preload,0) == 1)   //PF_PRELOAD plugin, skip cache
			return Load();

		{
			FARString strPluginID, strCurPluginID;
			strCurPluginID.Format(
			    L"%llx%x%x",
			    FindData.nFileSize,
			    FindData.ftCreationTime.dwLowDateTime,
			    FindData.ftLastWriteTime.dwLowDateTime
			);
			GetRegKey(strRegKey, L"ID", strPluginID, L"");

			if (StrCmp(strPluginID, strCurPluginID) )   //одинаковые ли бинарники?
				return false;
		}

		SysID=GetRegKey(strRegKey,wszReg_SysID,0);
		pOpenPluginW=(PLUGINOPENPLUGINW)(INT_PTR)GetRegKey(strRegKey,wszReg_OpenPlugin,0);
		pOpenFilePluginW=(PLUGINOPENFILEPLUGINW)(INT_PTR)GetRegKey(strRegKey,wszReg_OpenFilePlugin,0);
		pSetFindListW=(PLUGINSETFINDLISTW)(INT_PTR)GetRegKey(strRegKey,wszReg_SetFindList,0);
		pProcessEditorInputW=(PLUGINPROCESSEDITORINPUTW)(INT_PTR)GetRegKey(strRegKey,wszReg_ProcessEditorInput,0);
		pProcessEditorEventW=(PLUGINPROCESSEDITOREVENTW)(INT_PTR)GetRegKey(strRegKey,wszReg_ProcessEditorEvent,0);
		pProcessViewerEventW=(PLUGINPROCESSVIEWEREVENTW)(INT_PTR)GetRegKey(strRegKey,wszReg_ProcessViewerEvent,0);
		pProcessDialogEventW=(PLUGINPROCESSDIALOGEVENTW)(INT_PTR)GetRegKey(strRegKey,wszReg_ProcessDialogEvent,0);
		pProcessSynchroEventW=(PLUGINPROCESSSYNCHROEVENTW)(INT_PTR)GetRegKey(strRegKey,wszReg_ProcessSynchroEvent,0);
#if defined(PROCPLUGINMACROFUNC)
		pProcessMacroFuncW=(PLUGINPROCESSMACROFUNCW)(INT_PTR)GetRegKey(strRegKey,wszReg_ProcessMacroFunc,0);
#endif
		pConfigureW=(PLUGINCONFIGUREW)(INT_PTR)GetRegKey(strRegKey,wszReg_Configure,0);
		pAnalyseW=(PLUGINANALYSEW)(INT_PTR)GetRegKey(strRegKey, wszReg_Analyse, 0);
		pGetCustomDataW=(PLUGINGETCUSTOMDATAW)(INT_PTR)GetRegKey(strRegKey, wszReg_GetCustomData, 0);
		WorkFlags.Set(PIWF_CACHED); //too much "cached" flags
		return true;
	}

	return false;
}

bool PluginW::SaveToCache()
{
	if (pGetPluginInfoW ||
	        pOpenPluginW ||
	        pOpenFilePluginW ||
	        pSetFindListW ||
	        pProcessEditorInputW ||
	        pProcessEditorEventW ||
	        pProcessViewerEventW ||
	        pProcessDialogEventW ||
	        pProcessSynchroEventW ||
#if defined(PROCPLUGINMACROFUNC)
	        pProcessMacroFuncW ||
#endif
	        pAnalyseW ||
	        pGetCustomDataW
	   )
	{
		PluginInfo Info;
		GetPluginInfo(&Info);
		SysID = Info.SysID; //LAME!!!
		FARString strRegKey;
		strRegKey.Format(FmtPluginsCache_PluginS, m_strCacheName.CPtr());
		DeleteKeyTree(strRegKey);
		{
			bool bPreload = (Info.Flags & PF_PRELOAD);
			SetRegKey(strRegKey, wszReg_Preload, bPreload?1:0);
			WorkFlags.Change(PIWF_PRELOADED, bPreload);

			if (bPreload)
				return true;
		}
		{
			FARString strCurPluginID;
			FAR_FIND_DATA_EX fdata;
			apiGetFindDataEx(m_strModuleName, fdata);
			strCurPluginID.Format(
			    L"%llx%x%x",
			    fdata.nFileSize,
			    fdata.ftCreationTime.dwLowDateTime,
			    fdata.ftLastWriteTime.dwLowDateTime
			);
			SetRegKey(strRegKey, L"ID", strCurPluginID);
		}

		for (int i = 0; i < Info.DiskMenuStringsNumber; i++)
		{
			FARString strValue;
			strValue.Format(FmtDiskMenuStringD, i);
			SetRegKey(strRegKey, strValue, Info.DiskMenuStrings[i]);
		}

		for (int i = 0; i < Info.PluginMenuStringsNumber; i++)
		{
			FARString strValue;
			strValue.Format(FmtPluginMenuStringD, i);
			SetRegKey(strRegKey, strValue, Info.PluginMenuStrings[i]);
		}

		for (int i = 0; i < Info.PluginConfigStringsNumber; i++)
		{
			FARString strValue;
			strValue.Format(FmtPluginConfigStringD, i);
			SetRegKey(strRegKey,strValue,Info.PluginConfigStrings[i]);
		}

		SetRegKey(strRegKey, L"CommandPrefix", NullToEmpty(Info.CommandPrefix));
		SetRegKey(strRegKey, L"Flags", Info.Flags);
		strRegKey += L"/Exports";
		SetRegKey(strRegKey, wszReg_SysID, SysID);
		SetRegKey(strRegKey, wszReg_OpenPlugin, pOpenPluginW!=nullptr);
		SetRegKey(strRegKey, wszReg_OpenFilePlugin, pOpenFilePluginW!=nullptr);
		SetRegKey(strRegKey, wszReg_SetFindList, pSetFindListW!=nullptr);
		SetRegKey(strRegKey, wszReg_ProcessEditorInput, pProcessEditorInputW!=nullptr);
		SetRegKey(strRegKey, wszReg_ProcessEditorEvent, pProcessEditorEventW!=nullptr);
		SetRegKey(strRegKey, wszReg_ProcessViewerEvent, pProcessViewerEventW!=nullptr);
		SetRegKey(strRegKey, wszReg_ProcessDialogEvent, pProcessDialogEventW!=nullptr);
		SetRegKey(strRegKey, wszReg_ProcessSynchroEvent, pProcessSynchroEventW!=nullptr);
#if defined(PROCPLUGINMACROFUNC)
		SetRegKey(strRegKey, wszReg_ProcessMacroFunc, pProcessMacroFuncW!=nullptr);
#endif
		SetRegKey(strRegKey, wszReg_Configure, pConfigureW!=nullptr);
		SetRegKey(strRegKey, wszReg_Analyse, pAnalyseW!=nullptr);
		SetRegKey(strRegKey, wszReg_GetCustomData, pGetCustomDataW!=nullptr);
		return true;
	}

	return false;
}

bool PluginW::Load()
{
	if (WorkFlags.Check(PIWF_DONTLOADAGAIN))
		return false;

	if (m_hModule)
		return true;

	if (!m_hModule)
	{
		FARString strCurPath;
		apiGetCurrentDirectory(strCurPath);
		PrepareModulePath(m_strModuleName);
		m_hModule = WINPORT(LoadLibraryEx)(m_strModuleName,nullptr,LOAD_WITH_ALTERED_SEARCH_PATH);
		GuardLastError Err;
		FarChDir(strCurPath);
	}

	if (!m_hModule)
	{
		if (!Opt.LoadPlug.SilentLoadPlugin) //убрать в PluginSet
		{
			SetMessageHelp(L"ErrLoadPlugin module");
			Message(MSG_WARNING|MSG_ERRORTYPE,1,MSG(MError),MSG(MPlgLoadPluginError),m_strModuleName,MSG(MOk));
		}

		//чтоб не пытаться загрузить опять а то ошибка будет постоянно показываться.
		WorkFlags.Set(PIWF_DONTLOADAGAIN);

		return false;
	}

	WorkFlags.Clear(PIWF_CACHED);
	pSetStartupInfoW=(PLUGINSETSTARTUPINFOW)WINPORT(GetProcAddress)(m_hModule,NFMP_SetStartupInfo);
	pOpenPluginW=(PLUGINOPENPLUGINW)WINPORT(GetProcAddress)(m_hModule,NFMP_OpenPlugin);
	pOpenFilePluginW=(PLUGINOPENFILEPLUGINW)WINPORT(GetProcAddress)(m_hModule,NFMP_OpenFilePlugin);
	pClosePluginW=(PLUGINCLOSEPLUGINW)WINPORT(GetProcAddress)(m_hModule,NFMP_ClosePlugin);
	pGetPluginInfoW=(PLUGINGETPLUGININFOW)WINPORT(GetProcAddress)(m_hModule,NFMP_GetPluginInfo);
	pGetOpenPluginInfoW=(PLUGINGETOPENPLUGININFOW)WINPORT(GetProcAddress)(m_hModule,NFMP_GetOpenPluginInfo);
	pGetFindDataW=(PLUGINGETFINDDATAW)WINPORT(GetProcAddress)(m_hModule,NFMP_GetFindData);
	pFreeFindDataW=(PLUGINFREEFINDDATAW)WINPORT(GetProcAddress)(m_hModule,NFMP_FreeFindData);
	pGetVirtualFindDataW=(PLUGINGETVIRTUALFINDDATAW)WINPORT(GetProcAddress)(m_hModule,NFMP_GetVirtualFindData);
	pFreeVirtualFindDataW=(PLUGINFREEVIRTUALFINDDATAW)WINPORT(GetProcAddress)(m_hModule,NFMP_FreeVirtualFindData);
	pSetDirectoryW=(PLUGINSETDIRECTORYW)WINPORT(GetProcAddress)(m_hModule,NFMP_SetDirectory);
	pGetFilesW=(PLUGINGETFILESW)WINPORT(GetProcAddress)(m_hModule,NFMP_GetFiles);
	pPutFilesW=(PLUGINPUTFILESW)WINPORT(GetProcAddress)(m_hModule,NFMP_PutFiles);
	pDeleteFilesW=(PLUGINDELETEFILESW)WINPORT(GetProcAddress)(m_hModule,NFMP_DeleteFiles);
	pMakeDirectoryW=(PLUGINMAKEDIRECTORYW)WINPORT(GetProcAddress)(m_hModule,NFMP_MakeDirectory);
	pProcessHostFileW=(PLUGINPROCESSHOSTFILEW)WINPORT(GetProcAddress)(m_hModule,NFMP_ProcessHostFile);
	pSetFindListW=(PLUGINSETFINDLISTW)WINPORT(GetProcAddress)(m_hModule,NFMP_SetFindList);
	pConfigureW=(PLUGINCONFIGUREW)WINPORT(GetProcAddress)(m_hModule,NFMP_Configure);
	pExitFARW=(PLUGINEXITFARW)WINPORT(GetProcAddress)(m_hModule,NFMP_ExitFAR);
	pMayExitFARW=(PLUGINMAYEXITFARW)WINPORT(GetProcAddress)(m_hModule,NFMP_MayExitFAR);
	pProcessKeyW=(PLUGINPROCESSKEYW)WINPORT(GetProcAddress)(m_hModule,NFMP_ProcessKey);
	pProcessEventW=(PLUGINPROCESSEVENTW)WINPORT(GetProcAddress)(m_hModule,NFMP_ProcessEvent);
	pCompareW=(PLUGINCOMPAREW)WINPORT(GetProcAddress)(m_hModule,NFMP_Compare);
	pProcessEditorInputW=(PLUGINPROCESSEDITORINPUTW)WINPORT(GetProcAddress)(m_hModule,NFMP_ProcessEditorInput);
	pProcessEditorEventW=(PLUGINPROCESSEDITOREVENTW)WINPORT(GetProcAddress)(m_hModule,NFMP_ProcessEditorEvent);
	pProcessViewerEventW=(PLUGINPROCESSVIEWEREVENTW)WINPORT(GetProcAddress)(m_hModule,NFMP_ProcessViewerEvent);
	pProcessDialogEventW=(PLUGINPROCESSDIALOGEVENTW)WINPORT(GetProcAddress)(m_hModule,NFMP_ProcessDialogEvent);
	pProcessSynchroEventW=(PLUGINPROCESSSYNCHROEVENTW)WINPORT(GetProcAddress)(m_hModule,NFMP_ProcessSynchroEvent);
#if defined(PROCPLUGINMACROFUNC)
	pProcessMacroFuncW=(PLUGINPROCESSMACROFUNCW)WINPORT(GetProcAddress)(m_hModule,NFMP_ProcessMacroFunc);
#endif
	pMinFarVersionW=(PLUGINMINFARVERSIONW)WINPORT(GetProcAddress)(m_hModule,NFMP_GetMinFarVersion);
	pAnalyseW=(PLUGINANALYSEW)WINPORT(GetProcAddress)(m_hModule, NFMP_Analyse);
	pGetCustomDataW=(PLUGINGETCUSTOMDATAW)WINPORT(GetProcAddress)(m_hModule, NFMP_GetCustomData);
	pFreeCustomDataW=(PLUGINFREECUSTOMDATAW)WINPORT(GetProcAddress)(m_hModule, NFMP_FreeCustomData);
	bool bUnloaded = false;

	if (!CheckMinFarVersion(bUnloaded) || !SetStartupInfo(bUnloaded))
	{
		if (!bUnloaded)
			Unload();

		//чтоб не пытаться загрузить опять а то ошибка будет постоянно показываться.
		WorkFlags.Set(PIWF_DONTLOADAGAIN);

		return false;
	}

	FuncFlags.Set(PICFF_LOADED);
	SaveToCache();
	return true;
}

static int WINAPI farExecuteW(const wchar_t *CmdStr, unsigned int flags)
{
	return farExecuteA(Wide2MB(CmdStr).c_str(), flags);
}

static int WINAPI farExecuteLibraryW(const wchar_t *Library, const wchar_t *Symbol, const wchar_t *CmdStr, unsigned int flags)
{
	return farExecuteLibraryA(Wide2MB(Library).c_str(), Wide2MB(Symbol).c_str(), Wide2MB(CmdStr).c_str(), flags);
}

static void farDisplayNotificationW(const wchar_t *action, const wchar_t *object)
{
	DisplayNotification(action, object);
}

static int farDispatchInterThreadCallsW()
{
	return DispatchInterThreadCalls();
}

void CreatePluginStartupInfo(Plugin *pPlugin, PluginStartupInfo *PSI, FarStandardFunctions *FSF)
{
	static PluginStartupInfo StartupInfo{};
	static FarStandardFunctions StandardFunctions{};

	// заполняем структуру StandardFunctions один раз!!!
	if (!StandardFunctions.StructSize)
	{
		StandardFunctions.StructSize=sizeof(StandardFunctions);
		StandardFunctions.snprintf=swprintf;
		StandardFunctions.BoxSymbols=BoxSymbols;
		StandardFunctions.sscanf=swscanf;
		StandardFunctions.qsort=FarQsort;
		StandardFunctions.qsortex=FarQsortEx;
		StandardFunctions.atoi=FarAtoi;
		StandardFunctions.atoi64=FarAtoi64;
		StandardFunctions.itoa=FarItoa;
		StandardFunctions.itoa64=FarItoa64;
		StandardFunctions.bsearch=FarBsearch;
		StandardFunctions.LIsLower = farIsLower;
		StandardFunctions.LIsUpper = farIsUpper;
		StandardFunctions.LIsAlpha = farIsAlpha;
		StandardFunctions.LIsAlphanum = farIsAlphaNum;
		StandardFunctions.LUpper = farUpper;
		StandardFunctions.LUpperBuf = farUpperBuf;
		StandardFunctions.LLowerBuf = farLowerBuf;
		StandardFunctions.LLower = farLower;
		StandardFunctions.LStrupr = farStrUpper;
		StandardFunctions.LStrlwr = farStrLower;
		StandardFunctions.LStricmp = farStrCmpI;
		StandardFunctions.LStrnicmp = farStrCmpNI;
		StandardFunctions.Unquote=Unquote;
		StandardFunctions.LTrim=RemoveLeadingSpaces;
		StandardFunctions.RTrim=RemoveTrailingSpaces;
		StandardFunctions.Trim=RemoveExternalSpaces;
		StandardFunctions.TruncStr=TruncStr;
		StandardFunctions.TruncPathStr=TruncPathStr;
		StandardFunctions.QuoteSpaceOnly=QuoteSpaceOnly;
		StandardFunctions.PointToName=PointToName;
		StandardFunctions.GetPathRoot=farGetPathRoot;
		StandardFunctions.AddEndSlash=AddEndSlash;
		StandardFunctions.CopyToClipboard=CopyToClipboard;
		StandardFunctions.PasteFromClipboard=PasteFromClipboard;
		StandardFunctions.FarKeyToName=FarKeyToName;
		StandardFunctions.FarNameToKey=KeyNameToKeyW;
		StandardFunctions.FarInputRecordToKey=InputRecordToKey;
		StandardFunctions.XLat=Xlat;
		StandardFunctions.GetFileOwner=farGetFileOwner;
		StandardFunctions.GetNumberOfLinks=GetNumberOfLinks;
		StandardFunctions.FarRecursiveSearch=FarRecursiveSearch;
		StandardFunctions.MkTemp=FarMkTemp;
		StandardFunctions.DeleteBuffer=DeleteBuffer;
		StandardFunctions.ProcessName=ProcessName;
		StandardFunctions.MkLink=FarMkLink;
		StandardFunctions.ConvertPath=farConvertPath;
		StandardFunctions.GetReparsePointInfo=farGetReparsePointInfo;
		StandardFunctions.GetCurrentDirectory=farGetCurrentDirectory;
		StandardFunctions.Execute = farExecuteW;
		StandardFunctions.ExecuteLibrary = farExecuteLibraryW;
		StandardFunctions.DisplayNotification = farDisplayNotificationW;
		StandardFunctions.DispatchInterThreadCalls = farDispatchInterThreadCallsW;
	}

	if (!StartupInfo.StructSize)
	{
		StartupInfo.StructSize=sizeof(StartupInfo);
		StartupInfo.Menu=FarMenuFn;
		StartupInfo.GetMsg=FarGetMsgFn;
		StartupInfo.Message=FarMessageFn;
		StartupInfo.Control=FarControl;
		StartupInfo.SaveScreen=FarSaveScreen;
		StartupInfo.RestoreScreen=FarRestoreScreen;
		StartupInfo.GetDirList=FarGetDirList;
		StartupInfo.GetPluginDirList=FarGetPluginDirList;
		StartupInfo.FreeDirList=FarFreeDirList;
		StartupInfo.FreePluginDirList=FarFreePluginDirList;
		StartupInfo.Viewer=FarViewer;
		StartupInfo.Editor=FarEditor;
		StartupInfo.CmpName=FarCmpName;
		StartupInfo.Text=FarText;
		StartupInfo.EditorControl=FarEditorControl;
		StartupInfo.ViewerControl=FarViewerControl;
		StartupInfo.ShowHelp=FarShowHelp;
		StartupInfo.AdvControl=FarAdvControl;
		StartupInfo.DialogInit=FarDialogInit;
		StartupInfo.DialogRun=FarDialogRun;
		StartupInfo.DialogFree=FarDialogFree;
		StartupInfo.SendDlgMessage=FarSendDlgMessage;
		StartupInfo.DefDlgProc=FarDefDlgProc;
		StartupInfo.InputBox=FarInputBox;
		StartupInfo.PluginsControl=farPluginsControl;
		StartupInfo.FileFilterControl=farFileFilterControl;
		StartupInfo.RegExpControl=farRegExpControl;
	}

	*PSI=StartupInfo;
	*FSF=StandardFunctions;
	PSI->FSF=FSF;
	PSI->RootKey=nullptr;
	PSI->ModuleNumber=(INT_PTR)pPlugin;

	if (pPlugin)
	{
		PSI->ModuleName = pPlugin->GetModuleName().CPtr();
	}
}


struct ExecuteStruct
{
	int id; //function id
	union
	{
		INT_PTR nResult;
		HANDLE hResult;
		BOOL bResult;
	};

	union
	{
		INT_PTR nDefaultResult;
		HANDLE hDefaultResult;
		BOOL bDefaultResult;
	};

	bool bUnloaded;
};


#define EXECUTE_FUNCTION(function, es) \
	{ \
		es.nResult = 0; \
		es.nDefaultResult = 0; \
		es.bUnloaded = false; \
		if ( Opt.ExceptRules ) \
		{ \
			function; \
		} \
		else \
			function; \
	}


#define EXECUTE_FUNCTION_EX(function, es) \
	{ \
		es.bUnloaded = false; \
		es.nResult = 0; \
		if ( Opt.ExceptRules ) \
		{ \
			es.nResult = (INT_PTR)function; \
		} \
		else \
			es.nResult = (INT_PTR)function; \
	}

bool PluginW::SetStartupInfo(bool &bUnloaded)
{
	if (pSetStartupInfoW && !ProcessException)
	{
		PluginStartupInfo _info;
		FarStandardFunctions _fsf;
		CreatePluginStartupInfo(this, &_info, &_fsf);
		// скорректирем адреса и плагино-зависимые поля
		strRootKey = Opt.strRegRoot;
		strRootKey += L"/Plugins";
		_info.RootKey = strRootKey.CPtr();
		ExecuteStruct es;
		es.id = EXCEPT_SETSTARTUPINFO;
		EXECUTE_FUNCTION(pSetStartupInfoW(&_info), es);

		if (es.bUnloaded)
		{
			bUnloaded = true;
			return false;
		}
	}

	return true;
}

static void ShowMessageAboutIllegalPluginVersion(const wchar_t* plg,int required)
{
	FARString strMsg1, strMsg2;
	FARString strPlgName;
	strMsg1.Format(MSG(MPlgRequired),
	               (WORD)(HIWORD(required)),(WORD)(LOWORD(required)));
	strMsg2.Format(MSG(MPlgRequired2),
	               (WORD)(HIWORD(FAR_VERSION)),(WORD)(LOWORD(FAR_VERSION)));
	Message(MSG_WARNING,1,MSG(MError),MSG(MPlgBadVers),plg,strMsg1,strMsg2,MSG(MOk));
}


bool PluginW::CheckMinFarVersion(bool &bUnloaded)
{
	if (pMinFarVersionW && !ProcessException)
	{
		ExecuteStruct es;
		es.id = EXCEPT_MINFARVERSION;
		es.nDefaultResult = 0;
		EXECUTE_FUNCTION_EX(pMinFarVersionW(), es);

		if (es.bUnloaded)
		{
			bUnloaded = true;
			return false;
		}

		DWORD FVer = (DWORD)es.nResult;

		if (LOWORD(FVer) >  LOWORD(FAR_VERSION) ||
		        (LOWORD(FVer) == LOWORD(FAR_VERSION) &&
		         HIWORD(FVer) >  HIWORD(FAR_VERSION)))
		{
			ShowMessageAboutIllegalPluginVersion(m_strModuleName,FVer);
			return false;
		}
	}

	return true;
}

int PluginW::Unload(bool bExitFAR)
{
	int nResult = TRUE;

	if (bExitFAR)
		ExitFAR();

	if (!WorkFlags.Check(PIWF_CACHED))
	{
		nResult = WINPORT(FreeLibrary)(m_hModule);
		ClearExports();
	}

	m_hModule = nullptr;
	FuncFlags.Clear(PICFF_LOADED); //??
	return nResult;
}

bool PluginW::IsPanelPlugin()
{
	return pSetFindListW ||
	       pGetFindDataW ||
	       pGetVirtualFindDataW ||
	       pSetDirectoryW ||
	       pGetFilesW ||
	       pPutFilesW ||
	       pDeleteFilesW ||
	       pMakeDirectoryW ||
	       pProcessHostFileW ||
	       pProcessKeyW ||
	       pProcessEventW ||
	       pCompareW ||
	       pGetOpenPluginInfoW ||
	       pFreeFindDataW ||
	       pFreeVirtualFindDataW ||
	       pClosePluginW;
}

int PluginW::Analyse(const AnalyseData *pData)
{
	if (Load() && pAnalyseW && !ProcessException)
	{
		ExecuteStruct es;
		es.id = EXCEPT_ANALYSE;
		es.bDefaultResult = FALSE;
		es.bResult = FALSE;
		EXECUTE_FUNCTION_EX(pAnalyseW(pData), es);
		return es.bResult;
	}

	return FALSE;
}

HANDLE PluginW::OpenPlugin(int OpenFrom, INT_PTR Item)
{
	ChangePriority *ChPriority = new ChangePriority(ChangePriority::NORMAL);

	CheckScreenLock(); //??

	{
//		FARString strCurDir;
//		CtrlObject->CmdLine->GetCurDir(strCurDir);
//		FarChDir(strCurDir);
		g_strDirToSet.Clear();
	}

	HANDLE hResult = INVALID_HANDLE_VALUE;

	if (Load() && pOpenPluginW && !ProcessException)
	{
		//CurPluginItem=this; //BUGBUG
		ExecuteStruct es;
		es.id = EXCEPT_OPENPLUGIN;
		es.hDefaultResult = INVALID_HANDLE_VALUE;
		es.hResult = INVALID_HANDLE_VALUE;
		EXECUTE_FUNCTION_EX(pOpenPluginW(OpenFrom,Item), es);
		hResult = es.hResult;
		//CurPluginItem=nullptr; //BUGBUG
		/*    CtrlObject->Macro.SetRedrawEditor(TRUE); //BUGBUG

		    if ( !es.bUnloaded )
		    {

		      if(OpenFrom == OPEN_EDITOR &&
		         !CtrlObject->Macro.IsExecuting() &&
		         CtrlObject->Plugins.CurEditor &&
		         CtrlObject->Plugins.CurEditor->IsVisible() )
		      {
		        CtrlObject->Plugins.ProcessEditorEvent(EE_REDRAW,EEREDRAW_CHANGE);
		        CtrlObject->Plugins.ProcessEditorEvent(EE_REDRAW,EEREDRAW_ALL);
		        CtrlObject->Plugins.CurEditor->Show();
		      }
		      if (hInternal!=INVALID_HANDLE_VALUE)
		      {
		        PluginHandle *hPlugin=new PluginHandle;
		        hPlugin->InternalHandle=es.hResult;
		        hPlugin->PluginNumber=(INT_PTR)this;
		        return((HANDLE)hPlugin);
		      }
		      else
		        if ( !g_strDirToSet.IsEmpty() )
		        {
							CtrlObject->Cp()->ActivePanel->SetCurDir(g_strDirToSet,TRUE);
		          CtrlObject->Cp()->ActivePanel->Redraw();
		        }
		    } */
	}

	delete ChPriority;

	return hResult;
}

//////////////////////////////////

HANDLE PluginW::OpenFilePlugin(
    const wchar_t *Name,
    const unsigned char *Data,
    int DataSize,
    int OpMode
)
{
	HANDLE hResult = INVALID_HANDLE_VALUE;

	if (Load() && pOpenFilePluginW && !ProcessException)
	{
		ExecuteStruct es;
		es.id = EXCEPT_OPENFILEPLUGIN;
		es.hDefaultResult = INVALID_HANDLE_VALUE;
		EXECUTE_FUNCTION_EX(pOpenFilePluginW(Name, Data, DataSize, OpMode), es);
		hResult = es.hResult;
	}

	return hResult;
}


int PluginW::SetFindList(
    HANDLE hPlugin,
    const PluginPanelItem *PanelItem,
    int ItemsNumber
)
{
	BOOL bResult = FALSE;

	if (pSetFindListW && !ProcessException)
	{
		ExecuteStruct es;
		es.id = EXCEPT_SETFINDLIST;
		es.bDefaultResult = FALSE;
		EXECUTE_FUNCTION_EX(pSetFindListW(hPlugin, PanelItem, ItemsNumber), es);
		bResult = es.bResult;
	}

	return bResult;
}

int PluginW::ProcessEditorInput(
    const INPUT_RECORD *D
)
{
	BOOL bResult = FALSE;

	if (Load() && pProcessEditorInputW && !ProcessException)
	{
		ExecuteStruct es;
		es.id = EXCEPT_PROCESSEDITORINPUT;
		es.bDefaultResult = TRUE; //(TRUE) treat the result as a completed request on exception!
		EXECUTE_FUNCTION_EX(pProcessEditorInputW(D), es);
		bResult = es.bResult;
	}

	return bResult;
}

int PluginW::ProcessEditorEvent(
    int Event,
    PVOID Param
)
{
	if (Load() && pProcessEditorEventW && !ProcessException)
	{
		ExecuteStruct es;
		es.id = EXCEPT_PROCESSEDITOREVENT;
		es.nDefaultResult = 0;
		EXECUTE_FUNCTION_EX(pProcessEditorEventW(Event, Param), es);
		(void)es; // supress 'set but not used' warning
	}

	return 0; //oops!
}

int PluginW::ProcessViewerEvent(
    int Event,
    void *Param
)
{
	if (Load() && pProcessViewerEventW && !ProcessException)
	{
		ExecuteStruct es;
		es.id = EXCEPT_PROCESSVIEWEREVENT;
		es.nDefaultResult = 0;
		EXECUTE_FUNCTION_EX(pProcessViewerEventW(Event, Param), es);
		(void)es; // supress 'set but not used' warning
	}

	return 0; //oops, again!
}

int PluginW::ProcessDialogEvent(
    int Event,
    void *Param
)
{
	BOOL bResult = FALSE;

	if (Load() && pProcessDialogEventW && !ProcessException)
	{
		ExecuteStruct es;
		es.id = EXCEPT_PROCESSDIALOGEVENT;
		es.bDefaultResult = FALSE;
		EXECUTE_FUNCTION_EX(pProcessDialogEventW(Event, Param), es);
		bResult = es.bResult;
	}

	return bResult;
}

int PluginW::ProcessSynchroEvent(
    int Event,
    void *Param
)
{
	if (Load() && pProcessSynchroEventW && !ProcessException)
	{
		ExecuteStruct es;
		es.id = EXCEPT_PROCESSSYNCHROEVENT;
		es.nDefaultResult = 0;
		EXECUTE_FUNCTION_EX(pProcessSynchroEventW(Event, Param), es);
		(void)es; // supress 'set but not used' warning
	}

	return 0; //oops, again!
}

#if defined(PROCPLUGINMACROFUNC)
int PluginW::ProcessMacroFunc(
    const wchar_t *Name,
    const FarMacroValue *Params,
    int nParams,
    FarMacroValue **Results,
    int *nResults
)
{
	int nResult = 0;

	if (Load() && pProcessMacroFuncW && !ProcessException)
	{
		ExecuteStruct es;
		es.id = EXCEPT_PROCESSMACROFUNC;
		es.nDefaultResult = 0;
		EXECUTE_FUNCTION_EX(pProcessMacroFuncW(Name,Params,nParams,Results,nResults), es);
		nResult = (int)es.nResult;
	}

	return nResult;
}
#endif

int PluginW::GetVirtualFindData(
    HANDLE hPlugin,
    PluginPanelItem **pPanelItem,
    int *pItemsNumber,
    const wchar_t *Path
)
{
	BOOL bResult = FALSE;

	if (pGetVirtualFindDataW && !ProcessException)
	{
		ExecuteStruct es;
		es.id = EXCEPT_GETVIRTUALFINDDATA;
		es.bDefaultResult = FALSE;
		EXECUTE_FUNCTION_EX(pGetVirtualFindDataW(hPlugin, pPanelItem, pItemsNumber, Path), es);
		bResult = es.bResult;
	}

	return bResult;
}


void PluginW::FreeVirtualFindData(
    HANDLE hPlugin,
    PluginPanelItem *PanelItem,
    int ItemsNumber
)
{
	if (pFreeVirtualFindDataW && !ProcessException)
	{
		ExecuteStruct es;
		es.id = EXCEPT_FREEVIRTUALFINDDATA;
		EXECUTE_FUNCTION(pFreeVirtualFindDataW(hPlugin, PanelItem, ItemsNumber), es);
		(void)es; // supress 'set but not used' warning
	}
}



int PluginW::GetFiles(
    HANDLE hPlugin,
    PluginPanelItem *PanelItem,
    int ItemsNumber,
    int Move,
    const wchar_t **DestPath,
    int OpMode
)
{
	int nResult = -1;

	if (pGetFilesW && !ProcessException)
	{
		ExecuteStruct es;
		es.id = EXCEPT_GETFILES;
		es.nDefaultResult = -1;
		EXECUTE_FUNCTION_EX(pGetFilesW(hPlugin, PanelItem, ItemsNumber, Move, DestPath, OpMode), es);
		nResult = (int)es.nResult;
	}

	return nResult;
}


int PluginW::PutFiles(
    HANDLE hPlugin,
    PluginPanelItem *PanelItem,
    int ItemsNumber,
    int Move,
    int OpMode
)
{
	int nResult = -1;

	if (pPutFilesW && !ProcessException)
	{
		ExecuteStruct es;
		es.id = EXCEPT_PUTFILES;
		es.nDefaultResult = -1;
		static FARString strCurrentDirectory;
		apiGetCurrentDirectory(strCurrentDirectory);
		EXECUTE_FUNCTION_EX(pPutFilesW(hPlugin, PanelItem, ItemsNumber, Move, strCurrentDirectory, OpMode), es);
		nResult = (int)es.nResult;
	}

	return nResult;
}

int PluginW::DeleteFiles(
    HANDLE hPlugin,
    PluginPanelItem *PanelItem,
    int ItemsNumber,
    int OpMode
)
{
	BOOL bResult = FALSE;

	if (pDeleteFilesW && !ProcessException)
	{
		ExecuteStruct es;
		es.id = EXCEPT_DELETEFILES;
		es.bDefaultResult = FALSE;
		EXECUTE_FUNCTION_EX(pDeleteFilesW(hPlugin, PanelItem, ItemsNumber, OpMode), es);
		bResult = (int)es.bResult;
	}

	return bResult;
}


int PluginW::MakeDirectory(
    HANDLE hPlugin,
    const wchar_t **Name,
    int OpMode
)
{
	int nResult = -1;

	if (pMakeDirectoryW && !ProcessException)
	{
		ExecuteStruct es;
		es.id = EXCEPT_MAKEDIRECTORY;
		es.nDefaultResult = -1;
		EXECUTE_FUNCTION_EX(pMakeDirectoryW(hPlugin, Name, OpMode), es);
		nResult = (int)es.nResult;
	}

	return nResult;
}


int PluginW::ProcessHostFile(
    HANDLE hPlugin,
    PluginPanelItem *PanelItem,
    int ItemsNumber,
    int OpMode
)
{
	BOOL bResult = FALSE;

	if (pProcessHostFileW && !ProcessException)
	{
		ExecuteStruct es;
		es.id = EXCEPT_PROCESSHOSTFILE;
		es.bDefaultResult = FALSE;
		EXECUTE_FUNCTION_EX(pProcessHostFileW(hPlugin, PanelItem, ItemsNumber, OpMode), es);
		bResult = es.bResult;
	}

	return bResult;
}


int PluginW::ProcessEvent(
    HANDLE hPlugin,
    int Event,
    PVOID Param
)
{
	BOOL bResult = FALSE;

	if (pProcessEventW && !ProcessException)
	{
		ExecuteStruct es;
		es.id = EXCEPT_PROCESSEVENT;
		es.bDefaultResult = FALSE;
		EXECUTE_FUNCTION_EX(pProcessEventW(hPlugin, Event, Param), es);
		bResult = es.bResult;
	}

	return bResult;
}


int PluginW::Compare(
    HANDLE hPlugin,
    const PluginPanelItem *Item1,
    const PluginPanelItem *Item2,
    DWORD Mode
)
{
	int nResult = -2;

	if (pCompareW && !ProcessException)
	{
		ExecuteStruct es;
		es.id = EXCEPT_COMPARE;
		es.nDefaultResult = -2;
		EXECUTE_FUNCTION_EX(pCompareW(hPlugin, Item1, Item2, Mode), es);
		nResult = (int)es.nResult;
	}

	return nResult;
}


int PluginW::GetFindData(
    HANDLE hPlugin,
    PluginPanelItem **pPanelItem,
    int *pItemsNumber,
    int OpMode
)
{
	BOOL bResult = FALSE;

	if (pGetFindDataW && !ProcessException)
	{
		ExecuteStruct es;
		es.id = EXCEPT_GETFINDDATA;
		es.bDefaultResult = FALSE;
		EXECUTE_FUNCTION_EX(pGetFindDataW(hPlugin, pPanelItem, pItemsNumber, OpMode), es);
		bResult = es.bResult;
	}

	return bResult;
}


void PluginW::FreeFindData(
    HANDLE hPlugin,
    PluginPanelItem *PanelItem,
    int ItemsNumber
)
{
	if (pFreeFindDataW && !ProcessException)
	{
		ExecuteStruct es;
		es.id = EXCEPT_FREEFINDDATA;
		EXECUTE_FUNCTION(pFreeFindDataW(hPlugin, PanelItem, ItemsNumber), es);
		(void)es; // supress 'set but not used' warning
	}
}

int PluginW::ProcessKey(
    HANDLE hPlugin,
    int Key,
    unsigned int dwControlState
)
{
	BOOL bResult = FALSE;

	if (pProcessKeyW && !ProcessException)
	{
		ExecuteStruct es;
		es.id = EXCEPT_PROCESSKEY;
		es.bDefaultResult = TRUE; // do not pass this key to far on exception
		EXECUTE_FUNCTION_EX(pProcessKeyW(hPlugin, Key, dwControlState), es);
		bResult = es.bResult;
	}

	return bResult;
}


void PluginW::ClosePlugin(
    HANDLE hPlugin
)
{
	if (pClosePluginW && !ProcessException)
	{
		ExecuteStruct es;
		es.id = EXCEPT_CLOSEPLUGIN;
		EXECUTE_FUNCTION(pClosePluginW(hPlugin), es);
		(void)es; // supress 'set but not used' warning
	}

//	m_pManager->m_pCurrentPlugin = (Plugin*)-1;
}


int PluginW::SetDirectory(
    HANDLE hPlugin,
    const wchar_t *Dir,
    int OpMode
)
{
	BOOL bResult = FALSE;

	if (pSetDirectoryW && !ProcessException)
	{
		ExecuteStruct es;
		es.id = EXCEPT_SETDIRECTORY;
		es.bDefaultResult = FALSE;
		EXECUTE_FUNCTION_EX(pSetDirectoryW(hPlugin, Dir, OpMode), es);
		bResult = es.bResult;
	}

	return bResult;
}


void PluginW::GetOpenPluginInfo(
    HANDLE hPlugin,
    OpenPluginInfo *pInfo
)
{
//	m_pManager->m_pCurrentPlugin = this;
	pInfo->StructSize = sizeof(OpenPluginInfo);

	if (pGetOpenPluginInfoW && !ProcessException)
	{
		ExecuteStruct es;
		es.id = EXCEPT_GETOPENPLUGININFO;
		EXECUTE_FUNCTION(pGetOpenPluginInfoW(hPlugin, pInfo), es);
		(void)es; // supress 'set but not used' warning
	}
}


int PluginW::Configure(
    int MenuItem
)
{
	BOOL bResult = FALSE;

	if (Load() && pConfigureW && !ProcessException)
	{
		ExecuteStruct es;
		es.id = EXCEPT_CONFIGURE;
		es.bDefaultResult = FALSE;
		EXECUTE_FUNCTION_EX(pConfigureW(MenuItem), es);
		bResult = es.bResult;
	}

	return bResult;
}


bool PluginW::GetPluginInfo(PluginInfo *pi)
{
	memset(pi, 0, sizeof(PluginInfo));

	if (pGetPluginInfoW && !ProcessException)
	{
		ExecuteStruct es;
		es.id = EXCEPT_GETPLUGININFO;
		EXECUTE_FUNCTION(pGetPluginInfoW(pi), es);

		if (!es.bUnloaded)
			return true;
	}

	return false;
}

int PluginW::GetCustomData(const wchar_t *FilePath, wchar_t **CustomData)
{
	if (Load() && pGetCustomDataW && !ProcessException)
	{
		ExecuteStruct es;
		es.id = EXCEPT_GETCUSTOMDATA;
		es.bDefaultResult = 0;
		es.bResult = 0;
		EXECUTE_FUNCTION_EX(pGetCustomDataW(FilePath, CustomData), es);
		return es.bResult;
	}

	return 0;
}

void PluginW::FreeCustomData(wchar_t *CustomData)
{
	if (Load() && pFreeCustomDataW && !ProcessException)
	{
		ExecuteStruct es;
		es.id = EXCEPT_FREECUSTOMDATA;
		EXECUTE_FUNCTION(pFreeCustomDataW(CustomData), es);
		(void)es; // supress 'set but not used' warning
	}
}

bool PluginW::MayExitFAR()
{

	if (pMayExitFARW && !ProcessException)
	{
		ExecuteStruct es;
		es.id = EXCEPT_MAYEXITFAR;
		es.bDefaultResult = 1;
		EXECUTE_FUNCTION_EX(pMayExitFARW(), es);
		return es.bResult;
	}

	return true;
}

void PluginW::ExitFAR()
{
	if (pExitFARW && !ProcessException)
	{
		ExecuteStruct es;
		es.id = EXCEPT_EXITFAR;
		EXECUTE_FUNCTION(pExitFARW(), es);
		(void)es; // supress 'set but not used' warning
	}
}

void PluginW::ClearExports()
{
	pSetStartupInfoW = nullptr;
	pOpenPluginW = nullptr;
	pOpenFilePluginW = nullptr;
	pClosePluginW = nullptr;
	pGetPluginInfoW = nullptr;
	pGetOpenPluginInfoW = nullptr;
	pGetFindDataW = nullptr;
	pFreeFindDataW = nullptr;
	pGetVirtualFindDataW = nullptr;
	pFreeVirtualFindDataW = nullptr;
	pSetDirectoryW = nullptr;
	pGetFilesW = nullptr;
	pPutFilesW = nullptr;
	pDeleteFilesW = nullptr;
	pMakeDirectoryW = nullptr;
	pProcessHostFileW = nullptr;
	pSetFindListW = nullptr;
	pConfigureW = nullptr;
	pExitFARW = nullptr;
	pMayExitFARW = nullptr;
	pProcessKeyW = nullptr;
	pProcessEventW = nullptr;
	pCompareW = nullptr;
	pProcessEditorInputW = nullptr;
	pProcessEditorEventW = nullptr;
	pProcessViewerEventW = nullptr;
	pProcessDialogEventW = nullptr;
	pProcessSynchroEventW = nullptr;
#if defined(PROCPLUGINMACROFUNC)
	pProcessMacroFuncW = nullptr;
#endif
	pMinFarVersionW = nullptr;
	pAnalyseW = nullptr;
	pGetCustomDataW = nullptr;
	pFreeCustomDataW = nullptr;
}

