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
#include "lang.hpp"
#include "keys.hpp"
#include "codepage.hpp"
// #include "flink.hpp"
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
// #include "farexcpt.hpp"
#include "fileedit.hpp"
#include "RefreshFrameManager.hpp"
#include "InterThreadCall.hpp"
#include "plclass.hpp"
#include "PluginA.hpp"
// #include "localOEM.hpp"
#include "plugapi.hpp"
#include "keyboard.hpp"
#include "message.hpp"
#include "interf.hpp"
#include "clipboard.hpp"
#include "xlat.hpp"
#include "fileowner.hpp"
#include "pathmix.hpp"
#include "dirmix.hpp"
#include "processname.hpp"
#include "mix.hpp"
#include "execute.hpp"
#include "flink.hpp"
#include "ConfigRW.hpp"
#include "wrap.cpp"
#include <KeyFileHelper.h>

#include "farversion.h"

static const char *szCache_Preload = "Preload";
static const char *szCache_Preopen = "Preopen";
static const char *szCache_SysID = "SysID";

static const char szCache_OpenPlugin[] = "OpenPlugin";
static const char szCache_OpenFilePlugin[] = "OpenFilePlugin";
static const char szCache_SetFindList[] = "SetFindList";
static const char szCache_ProcessEditorInput[] = "ProcessEditorInput";
static const char szCache_ProcessEditorEvent[] = "ProcessEditorEvent";
static const char szCache_ProcessViewerEvent[] = "ProcessViewerEvent";
static const char szCache_ProcessDialogEvent[] = "ProcessDialogEvent";
static const char szCache_Configure[] = "Configure";
static const char szCache_GetFiles[] = "GetFiles";
static const char szCache_ProcessHostFile[] = "ProcessHostFile";

static const char NFMP_OpenPlugin[] = "OpenPlugin";
static const char NFMP_OpenFilePlugin[] = "OpenFilePlugin";
static const char NFMP_SetFindList[] = "SetFindList";
static const char NFMP_ProcessEditorInput[] = "ProcessEditorInput";
static const char NFMP_ProcessEditorEvent[] = "ProcessEditorEvent";
static const char NFMP_ProcessViewerEvent[] = "ProcessViewerEvent";
static const char NFMP_ProcessDialogEvent[] = "ProcessDialogEvent";
static const char NFMP_SetStartupInfo[] = "SetStartupInfo";
static const char NFMP_ClosePlugin[] = "ClosePlugin";
static const char NFMP_GetPluginInfo[] = "GetPluginInfo";
static const char NFMP_GetOpenPluginInfo[] = "GetOpenPluginInfo";
static const char NFMP_GetFindData[] = "GetFindData";
static const char NFMP_FreeFindData[] = "FreeFindData";
static const char NFMP_GetVirtualFindData[] = "GetVirtualFindData";
static const char NFMP_FreeVirtualFindData[] = "FreeVirtualFindData";
static const char NFMP_SetDirectory[] = "SetDirectory";
static const char NFMP_GetFiles[] = "GetFiles";
static const char NFMP_PutFiles[] = "PutFiles";
static const char NFMP_DeleteFiles[] = "DeleteFiles";
static const char NFMP_MakeDirectory[] = "MakeDirectory";
static const char NFMP_ProcessHostFile[] = "ProcessHostFile";
static const char NFMP_Configure[] = "Configure";
static const char NFMP_MayExitFAR[] = "MayExitFAR";
static const char NFMP_ExitFAR[] = "ExitFAR";
static const char NFMP_ProcessKey[] = "ProcessKey";
static const char NFMP_ProcessEvent[] = "ProcessEvent";
static const char NFMP_Compare[] = "Compare";
static const char NFMP_GetMinFarVersion[] = "GetMinFarVersion";

static void CheckScreenLock()
{
	if (ScrBuf.GetLockCount() > 0 && !CtrlObject->Macro.PeekKey()) {
		ScrBuf.SetLockCount(0);
		ScrBuf.Flush();
	}
}

PluginA::PluginA(PluginManager *owner, const FARString &strModuleName, const std::string &settingsName,
		const std::string &moduleID)
	:
	Plugin(owner, strModuleName, settingsName, moduleID), pFDPanelItemA(nullptr), pVFDPanelItemA(nullptr)
// more initialization here!!!
{
	ClearExports();
	memset(&PI, 0, sizeof(PI));
	memset(&OPI, 0, sizeof(OPI));
}

PluginA::~PluginA()
{
	FreePluginInfo();
	FreeOpenPluginInfo();
}

bool PluginA::LoadFromCache()
{
	KeyFileReadSection kfh(PluginsIni(), GetSettingsName());

	if (!kfh.SectionLoaded())
		return false;

	// PF_PRELOAD plugin, skip cache
	if (kfh.GetInt(szCache_Preload) != 0)
		return Load();

	// одинаковые ли бинарники?
	if (kfh.GetString("ID") != m_strModuleID)
		return false;

	SysID = kfh.GetUInt(szCache_SysID, 0);
	pOpenPlugin = (PLUGINOPENPLUGIN)(INT_PTR)kfh.GetUInt(szCache_OpenPlugin, 0);
	pOpenFilePlugin = (PLUGINOPENFILEPLUGIN)(INT_PTR)kfh.GetUInt(szCache_OpenFilePlugin, 0);
	pSetFindList = (PLUGINSETFINDLIST)(INT_PTR)kfh.GetUInt(szCache_SetFindList, 0);
	pProcessEditorInput = (PLUGINPROCESSEDITORINPUT)(INT_PTR)kfh.GetUInt(szCache_ProcessEditorInput, 0);
	pProcessEditorEvent = (PLUGINPROCESSEDITOREVENT)(INT_PTR)kfh.GetUInt(szCache_ProcessEditorEvent, 0);
	pProcessViewerEvent = (PLUGINPROCESSVIEWEREVENT)(INT_PTR)kfh.GetUInt(szCache_ProcessViewerEvent, 0);
	pProcessDialogEvent = (PLUGINPROCESSDIALOGEVENT)(INT_PTR)kfh.GetUInt(szCache_ProcessDialogEvent, 0);
	pConfigure = (PLUGINCONFIGURE)(INT_PTR)kfh.GetUInt(szCache_Configure, 0);
	pGetFiles = (PLUGINGETFILES)(INT_PTR)kfh.GetUInt(szCache_GetFiles, 0);
	pProcessHostFile = (PLUGINPROCESSHOSTFILE)(INT_PTR)kfh.GetUInt(szCache_ProcessHostFile, 0);
	WorkFlags.Set(PIWF_CACHED);		// too much "cached" flags

	if (kfh.GetInt(szCache_Preopen) != 0)
		OpenModule();

	return true;
}

bool PluginA::SaveToCache()
{
	if (!pGetPluginInfo && !pOpenPlugin && !pOpenFilePlugin && !pSetFindList && !pProcessEditorInput
			&& !pProcessEditorEvent && !pProcessViewerEvent && !pProcessDialogEvent) {
		return false;
	}

	KeyFileHelper kfh(PluginsIni());
	kfh.RemoveSection(GetSettingsName());

	const std::string &module = m_strModuleName.GetMB();

	struct stat st{};
	if (stat(module.c_str(), &st) == -1) {
		fprintf(stderr, "%s: stat('%s') error %u\n", __FUNCTION__, module.c_str(), errno);
		return false;
	}

	kfh.SetString(GetSettingsName(), "Module", module.c_str());

	PluginInfo Info{};
	GetPluginInfo(&Info);
	SysID = Info.SysID;		// LAME!!!

	kfh.SetInt(GetSettingsName(), szCache_Preopen, ((Info.Flags & PF_PREOPEN) != 0));

	if ((Info.Flags & PF_PRELOAD) != 0) {
		kfh.SetInt(GetSettingsName(), szCache_Preload, 1);
		WorkFlags.Change(PIWF_PRELOADED, TRUE);
		return true;
	}
	WorkFlags.Change(PIWF_PRELOADED, FALSE);

	kfh.SetString(GetSettingsName(), "ID", m_strModuleID.c_str());

	for (int i = 0; i < Info.DiskMenuStringsNumber; i++) {
		kfh.SetString(GetSettingsName(), StrPrintf(FmtDiskMenuStringD, i).c_str(), Info.DiskMenuStrings[i]);
	}

	for (int i = 0; i < Info.PluginMenuStringsNumber; i++) {
		kfh.SetString(GetSettingsName(), StrPrintf(FmtPluginMenuStringD, i).c_str(),
				Info.PluginMenuStrings[i]);
	}

	for (int i = 0; i < Info.PluginConfigStringsNumber; i++) {
		kfh.SetString(GetSettingsName(), StrPrintf(FmtPluginConfigStringD, i).c_str(),
				Info.PluginConfigStrings[i]);
	}

	kfh.SetString(GetSettingsName(), "CommandPrefix", Info.CommandPrefix);
	kfh.SetUInt(GetSettingsName(), "Flags", Info.Flags);

	kfh.SetUInt(GetSettingsName(), szCache_SysID, SysID);
	kfh.SetUInt(GetSettingsName(), szCache_OpenPlugin, pOpenPlugin != nullptr);
	kfh.SetUInt(GetSettingsName(), szCache_OpenFilePlugin, pOpenFilePlugin != nullptr);
	kfh.SetUInt(GetSettingsName(), szCache_SetFindList, pSetFindList != nullptr);
	kfh.SetUInt(GetSettingsName(), szCache_ProcessEditorInput, pProcessEditorInput != nullptr);
	kfh.SetUInt(GetSettingsName(), szCache_ProcessEditorEvent, pProcessEditorEvent != nullptr);
	kfh.SetUInt(GetSettingsName(), szCache_ProcessViewerEvent, pProcessViewerEvent != nullptr);
	kfh.SetUInt(GetSettingsName(), szCache_ProcessDialogEvent, pProcessDialogEvent != nullptr);
	kfh.SetUInt(GetSettingsName(), szCache_Configure, pConfigure != nullptr);
	kfh.SetUInt(GetSettingsName(), szCache_GetFiles, pGetFiles!=nullptr);
	kfh.SetUInt(GetSettingsName(), szCache_ProcessHostFile, pProcessHostFile!=nullptr);
	return true;
}

bool PluginA::Load()
{
	if (m_Loaded)
		return true;

	if (!OpenModule())
		return false;

	m_Loaded = true;

	WorkFlags.Clear(PIWF_CACHED);
	GetModuleFN(pSetStartupInfo, NFMP_SetStartupInfo);
	GetModuleFN(pOpenPlugin, NFMP_OpenPlugin);
	GetModuleFN(pOpenFilePlugin, NFMP_OpenFilePlugin);
	GetModuleFN(pClosePlugin, NFMP_ClosePlugin);
	GetModuleFN(pGetPluginInfo, NFMP_GetPluginInfo);
	GetModuleFN(pGetOpenPluginInfo, NFMP_GetOpenPluginInfo);
	GetModuleFN(pGetFindData, NFMP_GetFindData);
	GetModuleFN(pFreeFindData, NFMP_FreeFindData);
	GetModuleFN(pGetVirtualFindData, NFMP_GetVirtualFindData);
	GetModuleFN(pFreeVirtualFindData, NFMP_FreeVirtualFindData);
	GetModuleFN(pSetDirectory, NFMP_SetDirectory);
	GetModuleFN(pGetFiles, NFMP_GetFiles);
	GetModuleFN(pPutFiles, NFMP_PutFiles);
	GetModuleFN(pDeleteFiles, NFMP_DeleteFiles);
	GetModuleFN(pMakeDirectory, NFMP_MakeDirectory);
	GetModuleFN(pProcessHostFile, NFMP_ProcessHostFile);
	GetModuleFN(pSetFindList, NFMP_SetFindList);
	GetModuleFN(pConfigure, NFMP_Configure);
	GetModuleFN(pExitFAR, NFMP_ExitFAR);
	GetModuleFN(pMayExitFAR, NFMP_MayExitFAR);
	GetModuleFN(pProcessKey, NFMP_ProcessKey);
	GetModuleFN(pProcessEvent, NFMP_ProcessEvent);
	GetModuleFN(pCompare, NFMP_Compare);
	GetModuleFN(pProcessEditorInput, NFMP_ProcessEditorInput);
	GetModuleFN(pProcessEditorEvent, NFMP_ProcessEditorEvent);
	GetModuleFN(pProcessViewerEvent, NFMP_ProcessViewerEvent);
	GetModuleFN(pProcessDialogEvent, NFMP_ProcessDialogEvent);
	GetModuleFN(pMinFarVersion, NFMP_GetMinFarVersion);

	bool bUnloaded = false;
	if (!CheckMinFarVersion(bUnloaded) || !SetStartupInfo(bUnloaded)) {
		if (!bUnloaded)
			Unload();

		// чтоб не пытаться загрузить опять а то ошибка будет постоянно показываться.
		WorkFlags.Set(PIWF_DONTLOADAGAIN);

		return false;
	}

	FuncFlags.Set(PICFF_LOADED);
	SaveToCache();
	return true;
}

static void farDisplayNotificationA(const char *action, const char *object)
{
	DisplayNotification(action, object);
}

static int farDispatchInterThreadCallsA()
{
	return DispatchInterThreadCalls();
}

static void WINAPI farBackgroundTaskA(const char *Info, BOOL Started)
{
	if (Started)
		CtrlObject->Plugins.BackgroundTaskStarted(MB2Wide(Info).c_str());
	else
		CtrlObject->Plugins.BackgroundTaskFinished(MB2Wide(Info).c_str());
}

static size_t WINAPI farStrCellsCountA(const char *Str, size_t CharsCount)
{
	std::wstring ws;
	MB2Wide(Str, CharsCount, ws);
	return StrCellsCount(ws.c_str(), ws.size());
}

static size_t WINAPI farStrSizeOfCellsA(const char *Str, size_t CharsCount, size_t *CellsCount, BOOL RoundUp)
{
	std::wstring ws;
	MB2Wide(Str, CharsCount, ws);
	size_t cnt = StrSizeOfCells(ws.c_str(), ws.size(), *CellsCount, RoundUp != FALSE);
	ws.resize(cnt);
	return StrWide2MB(ws).size();
}

static void
CreatePluginStartupInfoA(PluginA *pPlugin, oldfar::PluginStartupInfo *PSI, oldfar::FarStandardFunctions *FSF)
{
	static oldfar::PluginStartupInfo StartupInfo{};
	static oldfar::FarStandardFunctions StandardFunctions{};

	// заполняем структуру StandardFunctions один раз!!!
	if (!StandardFunctions.StructSize) {
		StandardFunctions.StructSize = sizeof(StandardFunctions);
		StandardFunctions.sprintf = sprintf;
		StandardFunctions.snprintf = snprintf;
		StandardFunctions.sscanf = sscanf;
		StandardFunctions.qsort = FarQsort;
		StandardFunctions.qsortex = FarQsortEx;
		StandardFunctions.atoi = FarAtoiA;
		StandardFunctions.atoi64 = FarAtoi64A;
		StandardFunctions.itoa = FarItoaA;
		StandardFunctions.itoa64 = FarItoa64A;
		StandardFunctions.bsearch = FarBsearch;
		StandardFunctions.Unquote = UnquoteA;
		StandardFunctions.LTrim = RemoveLeadingSpacesA;
		StandardFunctions.RTrim = RemoveTrailingSpacesA;
		StandardFunctions.Trim = RemoveExternalSpacesA;
		StandardFunctions.TruncStr = TruncStrA;
		StandardFunctions.TruncPathStr = TruncPathStrA;
		StandardFunctions.QuoteSpaceOnly = QuoteSpaceOnlyA;
		StandardFunctions.PointToName = PointToNameA;
		StandardFunctions.GetPathRoot = GetPathRootA;
		StandardFunctions.AddEndSlash = AddEndSlashA;
		StandardFunctions.CopyToClipboard = CopyToClipboardA;
		StandardFunctions.PasteFromClipboard = PasteFromClipboardA;
		StandardFunctions.FarKeyToName = FarKeyToNameA;
		StandardFunctions.FarNameToKey = KeyNameToKeyA;
		StandardFunctions.FarInputRecordToKey = InputRecordToKeyA;
		StandardFunctions.XLat = XlatA;
		StandardFunctions.GetFileOwner = GetFileOwnerA;
		StandardFunctions.GetNumberOfLinks = GetNumberOfLinksA;
		StandardFunctions.FarRecursiveSearch = FarRecursiveSearchA;
		StandardFunctions.MkTemp = FarMkTempA;
		StandardFunctions.DeleteBuffer = DeleteBufferA;
		StandardFunctions.ProcessName = ProcessNameA;
		StandardFunctions.MkLink = FarMkLinkA;
		StandardFunctions.ConvertNameToReal = ConvertNameToRealA;
		StandardFunctions.GetReparsePointInfo = FarGetReparsePointInfoA;
		StandardFunctions.ExpandEnvironmentStr = ExpandEnvironmentStrA;
		StandardFunctions.Execute = farExecuteA;
		StandardFunctions.ExecuteLibrary = farExecuteLibraryA;
		StandardFunctions.DisplayNotification = farDisplayNotificationA;
		StandardFunctions.DispatchInterThreadCalls = farDispatchInterThreadCallsA;
		StandardFunctions.BackgroundTask = farBackgroundTaskA;
		StandardFunctions.StrCellsCount = farStrCellsCountA;
		StandardFunctions.StrSizeOfCells = farStrSizeOfCellsA;
	}

	if (!StartupInfo.StructSize) {
		StartupInfo.StructSize = sizeof(StartupInfo);
		StartupInfo.Menu = FarMenuFnA;
		StartupInfo.Dialog = FarDialogFnA;
		StartupInfo.GetMsg = FarGetMsgFnA;
		StartupInfo.Message = FarMessageFnA;
		StartupInfo.Control = FarControlA;
		StartupInfo.SaveScreen = FarSaveScreen;
		StartupInfo.RestoreScreen = FarRestoreScreen;
		StartupInfo.GetDirList = FarGetDirListA;
		StartupInfo.GetPluginDirList = FarGetPluginDirListA;
		StartupInfo.FreeDirList = FarFreeDirListA;
		StartupInfo.Viewer = FarViewerA;
		StartupInfo.Editor = FarEditorA;
		StartupInfo.CmpName = FarCmpNameA;
		StartupInfo.CharTable = FarCharTableA;
		StartupInfo.Text = FarTextA;
		StartupInfo.EditorControl = FarEditorControlA;
		StartupInfo.ViewerControl = FarViewerControlA;
		StartupInfo.ShowHelp = FarShowHelpA;
		StartupInfo.AdvControl = FarAdvControlA;
		StartupInfo.DialogEx = FarDialogExA;
		StartupInfo.SendDlgMessage = FarSendDlgMessageA;
		StartupInfo.DefDlgProc = FarDefDlgProcA;
		StartupInfo.InputBox = FarInputBoxA;
	}

	*PSI = StartupInfo;
	*FSF = StandardFunctions;
	PSI->ModuleNumber = (INT_PTR)pPlugin;
	PSI->FSF = FSF;
	pPlugin->GetModuleName().GetCharString(PSI->ModuleName, sizeof(PSI->ModuleName));
	PSI->RootKey = nullptr;
}

struct ExecuteStruct
{
	int id;		// function id
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

#define EXECUTE_FUNCTION(function, es)                                                                         \
	{                                                                                                          \
		es.nResult = 0;                                                                                        \
		es.nDefaultResult = 0;                                                                                 \
		es.bUnloaded = false;                                                                                  \
		function;                                                                                              \
	}

#define EXECUTE_FUNCTION_EX(function, es)                                                                      \
	{                                                                                                          \
		es.bUnloaded = false;                                                                                  \
		es.nResult = 0;                                                                                        \
		es.nResult = (INT_PTR)function;                                                                        \
	}

bool PluginA::SetStartupInfo(bool &bUnloaded)
{
	if (pSetStartupInfo) {
		oldfar::PluginStartupInfo _info;
		oldfar::FarStandardFunctions _fsf;

		CreatePluginStartupInfoA(this, &_info, &_fsf);
		// скорректируем адреса и плагино-зависимые поля
		if (mbRootKey.empty())
			mbRootKey = strRootKey.GetMB();
		_info.RootKey = mbRootKey.c_str();
		ExecuteStruct es;
		es.id = EXCEPT_SETSTARTUPINFO;
		EXECUTE_FUNCTION(pSetStartupInfo(&_info), es);

		if (es.bUnloaded) {
			bUnloaded = true;
			return false;
		}
	}

	return true;
}

static void ShowMessageAboutIllegalPluginVersion(const wchar_t *plg, int required)
{
	FARString strMsg1, strMsg2;
	strMsg1.Format(Msg::PlgRequired, (WORD)HIWORD(required), (WORD)LOWORD(required));
	strMsg2.Format(Msg::PlgRequired2, (WORD)HIWORD(FAR_VERSION), (WORD)LOWORD(FAR_VERSION));
	Message(MSG_WARNING, 1, Msg::Error, Msg::PlgBadVers, plg, strMsg1, strMsg2, Msg::Ok);
}

bool PluginA::CheckMinFarVersion(bool &bUnloaded)
{
	if (pMinFarVersion) {
		ExecuteStruct es;
		es.id = EXCEPT_MINFARVERSION;
		es.nDefaultResult = 0;
		EXECUTE_FUNCTION_EX(pMinFarVersion(), es);

		if (es.bUnloaded) {
			bUnloaded = true;
			return false;
		}

		DWORD FVer = (DWORD)es.nResult;

		if (LOWORD(FVer) > LOWORD(FAR_VERSION)
				|| (LOWORD(FVer) == LOWORD(FAR_VERSION) && HIWORD(FVer) > HIWORD(FAR_VERSION))) {
			ShowMessageAboutIllegalPluginVersion(m_strModuleName, FVer);
			return false;
		}
	}

	return true;
}

int PluginA::Unload(bool bExitFAR)
{
	int nResult = TRUE;

	if (bExitFAR)
		ExitFAR();

	if (!WorkFlags.Check(PIWF_CACHED))
		ClearExports();

	CloseModule();

	m_Loaded = false;
	FuncFlags.Clear(PICFF_LOADED);	//??
	return nResult;
}

bool PluginA::IsPanelPlugin()
{
	return pSetFindList || pGetFindData || pGetVirtualFindData || pSetDirectory || pGetFiles || pPutFiles
			|| pDeleteFiles || pMakeDirectory || pProcessHostFile || pProcessKey || pProcessEvent || pCompare
			|| pGetOpenPluginInfo || pFreeFindData || pFreeVirtualFindData || pClosePlugin;
}

HANDLE PluginA::OpenPlugin(int OpenFrom, INT_PTR Item)
{
	// ChangePriority *ChPriority = new ChangePriority(THREAD_PRIORITY_NORMAL);

	CheckScreenLock();	//??

	{
		//		FARString strCurDir;
		//		CtrlObject->CmdLine->GetCurDir(strCurDir);
		//		FarChDir(strCurDir);
		g_strDirToSet.Clear();
	}

	HANDLE hResult = INVALID_HANDLE_VALUE;

	if (Load() && pOpenPlugin) {
		// CurPluginItem=this; //BUGBUG
		ExecuteStruct es;
		es.id = EXCEPT_OPENPLUGIN;
		es.hDefaultResult = INVALID_HANDLE_VALUE;
		es.hResult = INVALID_HANDLE_VALUE;
		char *ItemA = nullptr;

		if (Item && (OpenFrom == OPEN_COMMANDLINE || OpenFrom == OPEN_SHORTCUT)) {
			ItemA = UnicodeToAnsi((const wchar_t *)Item);
			Item = (INT_PTR)ItemA;
		}

		EXECUTE_FUNCTION_EX(pOpenPlugin(OpenFrom, Item), es);

		if (ItemA)
			free(ItemA);

		hResult = es.hResult;
		// CurPluginItem=nullptr; //BUGBUG
		/*
		CtrlObject->Macro.SetRedrawEditor(TRUE); //BUGBUG

		if ( !es.bUnloaded )
		{

			if (
				OpenFrom == OPEN_EDITOR &&
				!CtrlObject->Macro.IsExecuting() &&
				CtrlObject->Plugins.CurEditor &&
				CtrlObject->Plugins.CurEditor->IsVisible()
			)
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
		}
		*/
	}

	//	delete ChPriority;

	return hResult;
}

//////////////////////////////////

HANDLE PluginA::OpenFilePlugin(const wchar_t *Name, const unsigned char *Data, int DataSize, int OpMode)
{
	HANDLE hResult = INVALID_HANDLE_VALUE;

	if (Load() && pOpenFilePlugin) {
		ExecuteStruct es;
		es.id = EXCEPT_OPENFILEPLUGIN;
		es.hDefaultResult = INVALID_HANDLE_VALUE;
		char *NameA = nullptr;

		if (Name)
			NameA = UnicodeToAnsi(Name);

		EXECUTE_FUNCTION_EX(pOpenFilePlugin(NameA, Data, DataSize, OpMode), es);

		if (NameA)
			free(NameA);

		hResult = es.hResult;
	}

	return hResult;
}

int PluginA::SetFindList(HANDLE hPlugin, const PluginPanelItem *PanelItem, int ItemsNumber)
{
	BOOL bResult = FALSE;

	if (pSetFindList) {
		ExecuteStruct es;
		es.id = EXCEPT_SETFINDLIST;
		es.bDefaultResult = FALSE;
		oldfar::PluginPanelItem *PanelItemA = nullptr;
		ConvertPanelItemsArrayToAnsi(PanelItem, PanelItemA, ItemsNumber);
		EXECUTE_FUNCTION_EX(pSetFindList(hPlugin, PanelItemA, ItemsNumber), es);
		FreePanelItemA(PanelItemA, ItemsNumber);
		bResult = es.bResult;
	}

	return bResult;
}

int PluginA::ProcessEditorInput(const INPUT_RECORD *D)
{
	BOOL bResult = FALSE;

	if (Load() && pProcessEditorInput) {
		ExecuteStruct es;
		es.id = EXCEPT_PROCESSEDITORINPUT;
		es.bDefaultResult = TRUE;	//(TRUE) treat the result as a completed request on exception!
		const INPUT_RECORD *Ptr = D;
		INPUT_RECORD OemRecord;

		if (Ptr->EventType == KEY_EVENT) {
			OemRecord = *D;
			int r = WINPORT(WideCharToMultiByte)(CP_UTF8, 0, &D->Event.KeyEvent.uChar.UnicodeChar, 1,
					&OemRecord.Event.KeyEvent.uChar.AsciiChar, 1, nullptr, nullptr);
			if (r < 0)
				fprintf(stderr, "PluginA::ProcessEditorInput: convert failed\n");
			// CharToOemBuff(&D->Event.KeyEvent.uChar.UnicodeChar,&OemRecord.Event.KeyEvent.uChar.AsciiChar,1);
			Ptr = &OemRecord;
		}

		EXECUTE_FUNCTION_EX(pProcessEditorInput(Ptr), es);
		bResult = es.bResult;
	}

	return bResult;
}

int PluginA::ProcessEditorEvent(int Event, PVOID Param)
{
	if (Load() && pProcessEditorEvent) {
		ExecuteStruct es;
		es.id = EXCEPT_PROCESSEDITOREVENT;
		es.nDefaultResult = 0;
		EXECUTE_FUNCTION_EX(pProcessEditorEvent(Event, Param), es);
		(void)es;	// suppress 'set but not used' warning
	}

	return 0;	// oops!
}

int PluginA::ProcessViewerEvent(int Event, void *Param)
{
	if (Load() && pProcessViewerEvent) {
		ExecuteStruct es;
		es.id = EXCEPT_PROCESSVIEWEREVENT;
		es.nDefaultResult = 0;
		EXECUTE_FUNCTION_EX(pProcessViewerEvent(Event, Param), es);
		(void)es;	// suppress 'set but not used' warning
	}

	return 0;	// oops, again!
}

int PluginA::ProcessDialogEvent(int Event, void *Param)
{
	BOOL bResult = FALSE;

	if (Load() && pProcessDialogEvent) {
		ExecuteStruct es;
		es.id = EXCEPT_PROCESSDIALOGEVENT;
		es.bDefaultResult = FALSE;
		EXECUTE_FUNCTION_EX(pProcessDialogEvent(Event, Param), es);
		bResult = es.bResult;
	}

	return bResult;
}

int PluginA::GetVirtualFindData(HANDLE hPlugin, PluginPanelItem **pPanelItem, int *pItemsNumber,
		const wchar_t *Path)
{
	BOOL bResult = FALSE;

	if (pGetVirtualFindData) {
		ExecuteStruct es;
		es.id = EXCEPT_GETVIRTUALFINDDATA;
		es.bDefaultResult = FALSE;
		pVFDPanelItemA = nullptr;
		size_t Size = StrLength(Path) + 1;
		LPSTR PathA = new char[Size * 4];
		PWZ_to_PZ(Path, PathA, Size * 4);
		EXECUTE_FUNCTION_EX(pGetVirtualFindData(hPlugin, &pVFDPanelItemA, pItemsNumber, PathA), es);
		bResult = es.bResult;
		delete[] PathA;

		if (bResult && *pItemsNumber) {
			ConvertPanelItemA(pVFDPanelItemA, pPanelItem, *pItemsNumber);
		}
	}

	return bResult;
}

void PluginA::FreeVirtualFindData(HANDLE hPlugin, PluginPanelItem *PanelItem, int ItemsNumber)
{
	FreeUnicodePanelItem(PanelItem, ItemsNumber);

	if (pFreeVirtualFindData && pVFDPanelItemA) {
		ExecuteStruct es;
		es.id = EXCEPT_FREEVIRTUALFINDDATA;
		EXECUTE_FUNCTION(pFreeVirtualFindData(hPlugin, pVFDPanelItemA, ItemsNumber), es);
		pVFDPanelItemA = nullptr;
		(void)es;	// suppress 'set but not used' warning
	}
}

bool PluginA::GetLinkTarget(HANDLE hPlugin, PluginPanelItem *PanelItem, FARString &result, int OpMode)
{
	return false;
}

int PluginA::GetFiles(HANDLE hPlugin, PluginPanelItem *PanelItem, int ItemsNumber, int Move,
		const wchar_t **DestPath, int OpMode)
{
	int nResult = -1;

	if (pGetFiles) {
		ExecuteStruct es;
		es.id = EXCEPT_GETFILES;
		es.nDefaultResult = -1;
		oldfar::PluginPanelItem *PanelItemA = nullptr;
		ConvertPanelItemsArrayToAnsi(PanelItem, PanelItemA, ItemsNumber);
		char DestA[oldfar::NM];
		PWZ_to_PZ(*DestPath, DestA, sizeof(DestA));
		EXECUTE_FUNCTION_EX(pGetFiles(hPlugin, PanelItemA, ItemsNumber, Move, DestA, OpMode), es);
		static wchar_t DestW[oldfar::NM];
		PZ_to_PWZ(DestA, DestW, ARRAYSIZE(DestW));
		*DestPath = DestW;
		FreePanelItemA(PanelItemA, ItemsNumber);
		nResult = (int)es.nResult;
	}

	return nResult;
}

int PluginA::PutFiles(HANDLE hPlugin, PluginPanelItem *PanelItem, int ItemsNumber, int Move, int OpMode)
{
	int nResult = -1;

	if (pPutFiles) {
		ExecuteStruct es;
		es.id = EXCEPT_PUTFILES;
		es.nDefaultResult = -1;
		oldfar::PluginPanelItem *PanelItemA = nullptr;
		ConvertPanelItemsArrayToAnsi(PanelItem, PanelItemA, ItemsNumber);
		EXECUTE_FUNCTION_EX(pPutFiles(hPlugin, PanelItemA, ItemsNumber, Move, OpMode), es);
		FreePanelItemA(PanelItemA, ItemsNumber);
		nResult = (int)es.nResult;
	}

	return nResult;
}

int PluginA::DeleteFiles(HANDLE hPlugin, PluginPanelItem *PanelItem, int ItemsNumber, int OpMode)
{
	BOOL bResult = FALSE;

	if (pDeleteFiles) {
		ExecuteStruct es;
		es.id = EXCEPT_DELETEFILES;
		es.bDefaultResult = FALSE;
		oldfar::PluginPanelItem *PanelItemA = nullptr;
		ConvertPanelItemsArrayToAnsi(PanelItem, PanelItemA, ItemsNumber);
		EXECUTE_FUNCTION_EX(pDeleteFiles(hPlugin, PanelItemA, ItemsNumber, OpMode), es);
		FreePanelItemA(PanelItemA, ItemsNumber);
		bResult = (int)es.bResult;
	}

	return bResult;
}

int PluginA::MakeDirectory(HANDLE hPlugin, const wchar_t **Name, int OpMode)
{
	int nResult = -1;

	if (pMakeDirectory) {
		ExecuteStruct es;
		es.id = EXCEPT_MAKEDIRECTORY;
		es.nDefaultResult = -1;
		char NameA[oldfar::NM];
		PWZ_to_PZ(*Name, NameA, sizeof(NameA));
		EXECUTE_FUNCTION_EX(pMakeDirectory(hPlugin, NameA, OpMode), es);
		static wchar_t NameW[oldfar::NM];
		PZ_to_PWZ(NameA, NameW, ARRAYSIZE(NameW));
		*Name = NameW;
		nResult = (int)es.nResult;
	}

	return nResult;
}

int PluginA::ProcessHostFile(HANDLE hPlugin, PluginPanelItem *PanelItem, int ItemsNumber, int OpMode)
{
	BOOL bResult = FALSE;

	if (pProcessHostFile) {
		ExecuteStruct es;
		es.id = EXCEPT_PROCESSHOSTFILE;
		es.bDefaultResult = FALSE;
		oldfar::PluginPanelItem *PanelItemA = nullptr;
		ConvertPanelItemsArrayToAnsi(PanelItem, PanelItemA, ItemsNumber);
		EXECUTE_FUNCTION_EX(pProcessHostFile(hPlugin, PanelItemA, ItemsNumber, OpMode), es);
		FreePanelItemA(PanelItemA, ItemsNumber);
		bResult = es.bResult;
	}

	return bResult;
}

int PluginA::ProcessEvent(HANDLE hPlugin, int Event, PVOID Param)
{
	BOOL bResult = FALSE;

	if (pProcessEvent) {
		ExecuteStruct es;
		es.id = EXCEPT_PROCESSEVENT;
		es.bDefaultResult = FALSE;
		PVOID ParamA = Param;

		if (Param && (Event == FE_COMMAND || Event == FE_CHANGEVIEWMODE))
			ParamA = (PVOID)UnicodeToAnsi((const wchar_t *)Param);

		EXECUTE_FUNCTION_EX(pProcessEvent(hPlugin, Event, ParamA), es);

		if (ParamA && (Event == FE_COMMAND || Event == FE_CHANGEVIEWMODE))
			free(ParamA);

		bResult = es.bResult;
	}

	return bResult;
}

int PluginA::Compare(HANDLE hPlugin, const PluginPanelItem *Item1, const PluginPanelItem *Item2, DWORD Mode)
{
	int nResult = -2;

	if (pCompare) {
		ExecuteStruct es;
		es.id = EXCEPT_COMPARE;
		es.nDefaultResult = -2;
		oldfar::PluginPanelItem *Item1A = nullptr;
		oldfar::PluginPanelItem *Item2A = nullptr;
		ConvertPanelItemsArrayToAnsi(Item1, Item1A, 1);
		ConvertPanelItemsArrayToAnsi(Item2, Item2A, 1);
		EXECUTE_FUNCTION_EX(pCompare(hPlugin, Item1A, Item2A, Mode), es);
		FreePanelItemA(Item1A, 1);
		FreePanelItemA(Item2A, 1);
		nResult = (int)es.nResult;
	}

	return nResult;
}

int PluginA::GetFindData(HANDLE hPlugin, PluginPanelItem **pPanelItem, int *pItemsNumber, int OpMode)
{
	BOOL bResult = FALSE;

	if (pGetFindData) {
		ExecuteStruct es;
		es.id = EXCEPT_GETFINDDATA;
		es.bDefaultResult = FALSE;
		pFDPanelItemA = nullptr;
		EXECUTE_FUNCTION_EX(pGetFindData(hPlugin, &pFDPanelItemA, pItemsNumber, OpMode), es);
		bResult = es.bResult;

		if (bResult && *pItemsNumber) {
			ConvertPanelItemA(pFDPanelItemA, pPanelItem, *pItemsNumber);
		}
	}

	return bResult;
}

void PluginA::FreeFindData(HANDLE hPlugin, PluginPanelItem *PanelItem, int ItemsNumber)
{
	FreeUnicodePanelItem(PanelItem, ItemsNumber);

	if (pFreeFindData && pFDPanelItemA) {
		ExecuteStruct es;
		es.id = EXCEPT_FREEFINDDATA;
		EXECUTE_FUNCTION(pFreeFindData(hPlugin, pFDPanelItemA, ItemsNumber), es);
		pFDPanelItemA = nullptr;
		(void)es;	// suppress 'set but not used' warning
	}
}

int PluginA::ProcessKey(HANDLE hPlugin, int Key, unsigned int dwControlState)
{
	BOOL bResult = FALSE;

	if (pProcessKey) {
		ExecuteStruct es;
		es.id = EXCEPT_PROCESSKEY;
		es.bDefaultResult = TRUE;	// do not pass this key to far on exception
		EXECUTE_FUNCTION_EX(pProcessKey(hPlugin, Key, dwControlState), es);
		bResult = es.bResult;
	}

	return bResult;
}

void PluginA::ClosePlugin(HANDLE hPlugin)
{
	if (pClosePlugin) {
		ExecuteStruct es;
		es.id = EXCEPT_CLOSEPLUGIN;
		EXECUTE_FUNCTION(pClosePlugin(hPlugin), es);
		(void)es;	// suppress 'set but not used' warning
	}

	FreeOpenPluginInfo();
	//	m_pManager->m_pCurrentPlugin = (Plugin*)-1;
}

int PluginA::SetDirectory(HANDLE hPlugin, const wchar_t *Dir, int OpMode)
{
	BOOL bResult = FALSE;

	if (pSetDirectory) {
		ExecuteStruct es;
		es.id = EXCEPT_SETDIRECTORY;
		es.bDefaultResult = FALSE;
		char *DirA = UnicodeToAnsi(Dir);
		EXECUTE_FUNCTION_EX(pSetDirectory(hPlugin, DirA, OpMode), es);

		if (DirA)
			free(DirA);

		bResult = es.bResult;
	}

	return bResult;
}

void PluginA::FreeOpenPluginInfo()
{
	if (OPI.CurDir)
		free((void *)OPI.CurDir);

	if (OPI.HostFile)
		free((void *)OPI.HostFile);

	if (OPI.Format)
		free((void *)OPI.Format);

	if (OPI.PanelTitle)
		free((void *)OPI.PanelTitle);

	if (OPI.InfoLines && OPI.InfoLinesNumber) {
		FreeUnicodeInfoPanelLines((InfoPanelLine *)OPI.InfoLines, OPI.InfoLinesNumber);
	}

	if (OPI.DescrFiles) {
		FreeArrayUnicode((wchar_t **)OPI.DescrFiles);
	}

	if (OPI.PanelModesArray) {
		FreeUnicodePanelModes((PanelMode *)OPI.PanelModesArray, OPI.PanelModesNumber);
	}

	if (OPI.KeyBar) {
		FreeUnicodeKeyBarTitles((KeyBarTitles *)OPI.KeyBar);
		free((void *)OPI.KeyBar);
	}

	if (OPI.ShortcutData)
		free((void *)OPI.ShortcutData);

	memset(&OPI, 0, sizeof(OPI));
}

void PluginA::ConvertOpenPluginInfo(oldfar::OpenPluginInfo &Src, OpenPluginInfo *Dest)
{
	FreeOpenPluginInfo();
	OPI.StructSize = sizeof(OPI);
	OPI.Flags = Src.Flags;

	if (Src.CurDir)
		OPI.CurDir = AnsiToUnicode(Src.CurDir);

	if (Src.HostFile)
		OPI.HostFile = AnsiToUnicode(Src.HostFile);

	if (Src.Format)
		OPI.Format = AnsiToUnicode(Src.Format);

	if (Src.PanelTitle)
		OPI.PanelTitle = AnsiToUnicode(Src.PanelTitle);

	if (Src.InfoLines && Src.InfoLinesNumber) {
		ConvertInfoPanelLinesA(Src.InfoLines, (InfoPanelLine **)&OPI.InfoLines, Src.InfoLinesNumber);
		OPI.InfoLinesNumber = Src.InfoLinesNumber;
	}

	if (Src.DescrFiles && Src.DescrFilesNumber) {
		OPI.DescrFiles = ArrayAnsiToUnicode((char **)Src.DescrFiles, Src.DescrFilesNumber);
		OPI.DescrFilesNumber = Src.DescrFilesNumber;
	}

	if (Src.PanelModesArray && Src.PanelModesNumber) {
		ConvertPanelModesA(Src.PanelModesArray, (PanelMode **)&OPI.PanelModesArray, Src.PanelModesNumber);
		OPI.PanelModesNumber = Src.PanelModesNumber;
		OPI.StartPanelMode = Src.StartPanelMode;
		OPI.StartSortMode = Src.StartSortMode;
		OPI.StartSortOrder = Src.StartSortOrder;
	}

	if (Src.KeyBar) {
		OPI.KeyBar = (KeyBarTitles *)malloc(sizeof(KeyBarTitles));
		ConvertKeyBarTitlesA(Src.KeyBar, (KeyBarTitles *)OPI.KeyBar,
				Src.StructSize >= (int)sizeof(oldfar::OpenPluginInfo));
	}

	if (Src.ShortcutData)
		OPI.ShortcutData = AnsiToUnicode(Src.ShortcutData);

	*Dest = OPI;
}

void PluginA::GetOpenPluginInfo(HANDLE hPlugin, OpenPluginInfo *pInfo)
{
	//	m_pManager->m_pCurrentPlugin = this;
	pInfo->StructSize = sizeof(OpenPluginInfo);

	if (pGetOpenPluginInfo) {
		ExecuteStruct es;
		es.id = EXCEPT_GETOPENPLUGININFO;
		oldfar::OpenPluginInfo InfoA{};
		EXECUTE_FUNCTION(pGetOpenPluginInfo(hPlugin, &InfoA), es);
		ConvertOpenPluginInfo(InfoA, pInfo);
		(void)es;	// suppress 'set but not used' warning
	}
}

int PluginA::Configure(int MenuItem)
{
	BOOL bResult = FALSE;

	if (Load() && pConfigure) {
		ExecuteStruct es;
		es.id = EXCEPT_CONFIGURE;
		es.bDefaultResult = FALSE;
		EXECUTE_FUNCTION_EX(pConfigure(MenuItem), es);
		bResult = es.bResult;
	}

	return bResult;
}

void PluginA::FreePluginInfo()
{
	if (PI.DiskMenuStringsNumber) {
		for (int i = 0; i < PI.DiskMenuStringsNumber; i++)
			free((void *)PI.DiskMenuStrings[i]);

		free((void *)PI.DiskMenuStrings);
	}

	if (PI.PluginMenuStringsNumber) {
		for (int i = 0; i < PI.PluginMenuStringsNumber; i++)
			free((void *)PI.PluginMenuStrings[i]);

		free((void *)PI.PluginMenuStrings);
	}

	if (PI.PluginConfigStringsNumber) {
		for (int i = 0; i < PI.PluginConfigStringsNumber; i++)
			free((void *)PI.PluginConfigStrings[i]);

		free((void *)PI.PluginConfigStrings);
	}

	if (PI.CommandPrefix)
		free((void *)PI.CommandPrefix);

	memset(&PI, 0, sizeof(PI));
}

void PluginA::ConvertPluginInfo(oldfar::PluginInfo &Src, PluginInfo *Dest)
{
	FreePluginInfo();
	PI.StructSize = sizeof(PI);
	PI.Flags = Src.Flags;

	if (Src.DiskMenuStringsNumber) {
		wchar_t **p = (wchar_t **)malloc(Src.DiskMenuStringsNumber * sizeof(wchar_t *));

		for (int i = 0; i < Src.DiskMenuStringsNumber; i++)
			p[i] = AnsiToUnicode(Src.DiskMenuStrings[i]);

		PI.DiskMenuStrings = p;
		PI.DiskMenuStringsNumber = Src.DiskMenuStringsNumber;
	}

	if (Src.PluginMenuStringsNumber) {
		wchar_t **p = (wchar_t **)malloc(Src.PluginMenuStringsNumber * sizeof(wchar_t *));

		for (int i = 0; i < Src.PluginMenuStringsNumber; i++)
			p[i] = AnsiToUnicode(Src.PluginMenuStrings[i]);

		PI.PluginMenuStrings = p;
		PI.PluginMenuStringsNumber = Src.PluginMenuStringsNumber;
	}

	if (Src.PluginConfigStringsNumber) {
		wchar_t **p = (wchar_t **)malloc(Src.PluginConfigStringsNumber * sizeof(wchar_t *));

		for (int i = 0; i < Src.PluginConfigStringsNumber; i++)
			p[i] = AnsiToUnicode(Src.PluginConfigStrings[i]);

		PI.PluginConfigStrings = p;
		PI.PluginConfigStringsNumber = Src.PluginConfigStringsNumber;
	}

	if (Src.CommandPrefix)
		PI.CommandPrefix = AnsiToUnicode(Src.CommandPrefix);

	*Dest = PI;
}

bool PluginA::GetPluginInfo(PluginInfo *pi)
{
	memset(pi, 0, sizeof(PluginInfo));

	if (pGetPluginInfo) {
		ExecuteStruct es;
		es.id = EXCEPT_GETPLUGININFO;
		oldfar::PluginInfo InfoA{};
		EXECUTE_FUNCTION(pGetPluginInfo(&InfoA), es);

		if (!es.bUnloaded) {
			ConvertPluginInfo(InfoA, pi);
			return true;
		}
	}

	return false;
}

bool PluginA::MayExitFAR()
{
	if (pMayExitFAR) {
		ExecuteStruct es;
		es.id = EXCEPT_MAYEXITFAR;
		es.bDefaultResult = 1;
		EXECUTE_FUNCTION_EX(pMayExitFAR(), es);
		return es.bResult;
	}

	return true;
}

void PluginA::ExitFAR()
{
	if (pExitFAR) {
		ExecuteStruct es;
		es.id = EXCEPT_EXITFAR;
		EXECUTE_FUNCTION(pExitFAR(), es);
		(void)es;	// suppress 'set but not used' warning
	}
}

void PluginA::ClearExports()
{
	pSetStartupInfo = nullptr;
	pOpenPlugin = nullptr;
	pOpenFilePlugin = nullptr;
	pClosePlugin = nullptr;
	pGetPluginInfo = nullptr;
	pGetOpenPluginInfo = nullptr;
	pGetFindData = nullptr;
	pFreeFindData = nullptr;
	pGetVirtualFindData = nullptr;
	pFreeVirtualFindData = nullptr;
	pSetDirectory = nullptr;
	pGetFiles = nullptr;
	pPutFiles = nullptr;
	pDeleteFiles = nullptr;
	pMakeDirectory = nullptr;
	pProcessHostFile = nullptr;
	pSetFindList = nullptr;
	pConfigure = nullptr;
	pExitFAR = nullptr;
	pMayExitFAR = nullptr;
	pProcessKey = nullptr;
	pProcessEvent = nullptr;
	pCompare = nullptr;
	pProcessEditorInput = nullptr;
	pProcessEditorEvent = nullptr;
	pProcessViewerEvent = nullptr;
	pProcessDialogEvent = nullptr;
	pMinFarVersion = nullptr;
}
