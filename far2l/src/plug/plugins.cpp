/*
plugins.cpp

Работа с плагинами (низкий уровень, кое-что повыше в flplugin.cpp)
*/
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
#include "scantree.hpp"
#include "chgprior.hpp"
#include "chgmmode.hpp"
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
#include "plugapi.hpp"
#include "pathmix.hpp"
#include "strmix.hpp"
#include "processname.hpp"
#include "interf.hpp"
#include "filelist.hpp"
#include "message.hpp"
#include "SafeMMap.hpp"
#include "HotkeyLetterDialog.hpp"
#include "InterThreadCall.hpp"
#include <KeyFileHelper.h>
#include <crc64.h>

#include "farversion.h"

const char *FmtDiskMenuStringD = "DiskMenuString%d";
const char *FmtPluginMenuStringD = "PluginMenuString%d";
const char *FmtPluginConfigStringD = "PluginConfigString%d";
const char *SettingsSection = "Settings";
static const wchar_t *PluginsFolderName = L"Plugins";

static int _cdecl PluginsSort(const void *el1, const void *el2);

enum PluginType
{
	NOT_PLUGIN = 0,
	WIDE_PLUGIN,
	MULTIBYTE_PLUGIN
};

////

const char *PluginsIni()
{
	static std::string s_out(InMyConfig("plugins/state.ini"));
	return s_out.c_str();
}

// Return string used as ini file key that represents given
// plugin object file. To reduce overhead encode less meaningful
// components like file path and extension as CRC suffix, leaded
// by actual plugin file name.
// If plugins resided in a path that is nested under g_strFarPath
// then dismiss g_strFarPath from CRC to make result invariant to
// whole package relocation.
static std::string PluginSettingsName(const FARString &strModuleName)
{
	std::string pathname;

	const size_t FarPathLength = g_strFarPath.GetLength();
	if (FarPathLength < strModuleName.GetLength()
			&& !StrCmpNI(strModuleName, g_strFarPath, (int)FarPathLength)) {
		Wide2MB(strModuleName.CPtr() + FarPathLength, pathname);
	} else {
		Wide2MB(strModuleName.CPtr(), pathname);
	}

	FilePathHashSuffix(pathname);

	return pathname;
}

static PluginType PluginTypeByExtension(const wchar_t *lpModuleName)
{
	const wchar_t *ext = wcsrchr(lpModuleName, L'.');
	if (ext) {
		if (wcscmp(ext, L".far-plug-wide") == 0)
			return WIDE_PLUGIN;
		if (wcscmp(ext, L".far-plug-mb") == 0)
			return MULTIBYTE_PLUGIN;
	}

	return NOT_PLUGIN;
}

PluginManager::PluginManager()
	:
	PluginsData(nullptr), PluginsCount(0), CurPluginItem(nullptr), CurEditor(nullptr), CurViewer(nullptr)
{}

PluginManager::~PluginManager()
{
	CurPluginItem = nullptr;
	Plugin *pPlugin;

	for (int i = 0; i < PluginsCount; i++) {
		pPlugin = PluginsData[i];
		pPlugin->Unload(true);
		delete pPlugin;
	}
	if (PluginsData) {
		free(PluginsData);
	}
}

bool PluginManager::AddPlugin(Plugin *pPlugin)
{
	Plugin **NewPluginsData = (Plugin **)realloc(PluginsData, sizeof(*PluginsData) * (PluginsCount + 1));

	if (!NewPluginsData)
		return false;

	PluginsData = NewPluginsData;
	PluginsData[PluginsCount] = pPlugin;
	PluginsCount++;
	return true;
}

bool PluginManager::RemovePlugin(Plugin *pPlugin)
{
	for (int i = 0; i < PluginsCount; i++) {
		if (PluginsData[i] == pPlugin) {
			delete pPlugin;
			memmove(&PluginsData[i], &PluginsData[i + 1], (PluginsCount - i - 1) * sizeof(Plugin *));
			PluginsCount--;
			return true;
		}
	}

	return false;
}

bool PluginManager::LoadPlugin(const FARString &strModuleName, bool UncachedLoad)
{
	const PluginType PlType = PluginTypeByExtension(strModuleName);

	if (PlType == NOT_PLUGIN)
		return false;

	struct stat st{};
	if (stat(strModuleName.GetMB().c_str(), &st) == -1) {
		fprintf(stderr, "%s: stat error %u for '%ls'\n", __FUNCTION__, errno, strModuleName.CPtr());
		return false;
	}

	const std::string &SettingsName = PluginSettingsName(strModuleName);
	const std::string &ModuleID = StrPrintf("%llx.%llx.%llx.%llx", (unsigned long long)st.st_ino,
			(unsigned long long)st.st_size, (unsigned long long)st.st_mtime, (unsigned long long)st.st_ctime);

	Plugin *pPlugin = nullptr;

	switch (PlType) {
		case WIDE_PLUGIN:
			pPlugin = new (std::nothrow) PluginW(this, strModuleName, SettingsName, ModuleID);
			break;
		case MULTIBYTE_PLUGIN:
			pPlugin = new (std::nothrow) PluginA(this, strModuleName, SettingsName, ModuleID);
			break;
		default:
			ABORT();
	}

	if (!pPlugin)
		return false;

	if (!AddPlugin(pPlugin)) {
		delete pPlugin;
		return false;
	}

	bool bResult = false;

	if (!UncachedLoad) {
		bResult = pPlugin->LoadFromCache();
		fprintf(stderr, "%s: cache %s for '%ls'\n", __FUNCTION__, bResult ? "hit" : "miss",
				strModuleName.CPtr());
	}

	if (!bResult && !Opt.LoadPlug.PluginsCacheOnly) {
		bResult = pPlugin->Load();

		if (!bResult)
			RemovePlugin(pPlugin);
	}

	return bResult;
}

bool PluginManager::CacheForget(const wchar_t *lpwszModuleName)
{
	KeyFileHelper kfh(PluginsIni());
	const std::string &SettingsName = PluginSettingsName(lpwszModuleName);
	if (!kfh.RemoveSection(SettingsName)) {
		fprintf(stderr, "%s: nothing to forget for '%ls'\n", __FUNCTION__, lpwszModuleName);
		return false;
	}
	fprintf(stderr, "%s: forgotten - '%ls'\n", __FUNCTION__, lpwszModuleName);
	return true;
}

bool PluginManager::LoadPluginExternal(const wchar_t *lpwszModuleName, bool LoadToMem)
{
	Plugin *pPlugin = GetPlugin(lpwszModuleName);

	if (pPlugin) {
		if (LoadToMem && !pPlugin->Load()) {
			RemovePlugin(pPlugin);
			return false;
		}
	} else {
		if (!LoadPlugin(lpwszModuleName, LoadToMem))
			return false;

		far_qsort(PluginsData, PluginsCount, sizeof(*PluginsData), PluginsSort);
	}
	return true;
}

int PluginManager::UnloadPlugin(Plugin *pPlugin, DWORD dwException, bool bRemove)
{
	int nResult = FALSE;

	if (pPlugin && (dwException != EXCEPT_EXITFAR))		// схитрим, если упали в EXITFAR, не полезем в рекурсию, мы и так в Unload
	{
		// какие-то непонятные действия...
		CurPluginItem = nullptr;
		Frame *frame;

		if ((frame = FrameManager->GetBottomFrame()))
			frame->Unlock();

		if (Flags.Check(PSIF_DIALOG))		// BugZ#52 exception handling for floating point incorrect
		{
			Flags.Clear(PSIF_DIALOG);
			FrameManager->DeleteFrame();
			FrameManager->PluginCommit();
		}

		bool bPanelPlugin = pPlugin->IsPanelPlugin();

		if (dwException != (DWORD)-1)
			nResult = pPlugin->Unload(true);
		else
			nResult = pPlugin->Unload(false);

		if (bPanelPlugin /*&& bUpdatePanels*/) {
			CtrlObject->Cp()->ActivePanel->SetCurDir(L".", TRUE);
			Panel *ActivePanel = CtrlObject->Cp()->ActivePanel;
			ActivePanel->Update(UPDATE_KEEP_SELECTION);
			ActivePanel->Redraw();
			Panel *AnotherPanel = CtrlObject->Cp()->GetAnotherPanel(ActivePanel);
			AnotherPanel->Update(UPDATE_KEEP_SELECTION | UPDATE_SECONDARY);
			AnotherPanel->Redraw();
		}

		if (bRemove)
			RemovePlugin(pPlugin);
	}

	return nResult;
}

int PluginManager::UnloadPluginExternal(const wchar_t *lpwszModuleName)
{
	// BUGBUG нужны проверки на легальность выгрузки
	int nResult = FALSE;
	Plugin *pPlugin = GetPlugin(lpwszModuleName);

	if (pPlugin) {
		nResult = pPlugin->Unload(true);
		RemovePlugin(pPlugin);
	}

	return nResult;
}

Plugin *PluginManager::GetPlugin(const wchar_t *lpwszModuleName)
{
	Plugin *pPlugin;

	for (int i = 0; i < PluginsCount; i++) {
		pPlugin = PluginsData[i];

		if (!StrCmp(lpwszModuleName, pPlugin->GetModuleName()))
			return pPlugin;
	}

	return nullptr;
}

Plugin *PluginManager::GetPlugin(int PluginNumber)
{
	if (PluginNumber < PluginsCount && PluginNumber >= 0)
		return PluginsData[PluginNumber];

	return nullptr;
}

void PluginManager::LoadPlugins()
{
	Flags.Clear(PSIF_PLUGINSLOADDED);

	if (Opt.LoadPlug.PluginsCacheOnly)		// $ 01.09.2000 tran  '/co' switch
	{
		LoadPluginsFromCache();
	} else if (Opt.LoadPlug.MainPluginDir || !Opt.LoadPlug.strCustomPluginsPath.IsEmpty()
			|| (Opt.LoadPlug.PluginsPersonal && !Opt.LoadPlug.strPersonalPluginsPath.IsEmpty())) {
		ScanTree ScTree(FALSE, TRUE, Opt.LoadPlug.ScanSymlinks);
		UserDefinedList PluginPathList;		// хранение списка каталогов
		FARString strPluginsDir;
		FARString strFullName;
		FAR_FIND_DATA_EX FindData;
		PluginPathList.SetParameters(0, 0, ULF_UNIQUE);

		// сначала подготовим список
		if (Opt.LoadPlug.MainPluginDir)		// только основные и персональные?
		{
			strPluginsDir = g_strFarPath + PluginsFolderName;
			PluginPathList.AddItem(strPluginsDir);

			if (TranslateFarString<TranslateInstallPath_Share2Lib>(strPluginsDir)) {
				PluginPathList.AddItem(strPluginsDir);
			}

			// ...а персональные есть?
			if (Opt.LoadPlug.PluginsPersonal && !Opt.LoadPlug.strPersonalPluginsPath.IsEmpty()
					&& !(Opt.Policies.DisabledOptions & FFPOL_PERSONALPATH))
				PluginPathList.AddItem(Opt.LoadPlug.strPersonalPluginsPath);
		} else if (!Opt.LoadPlug.strCustomPluginsPath.IsEmpty())	// только "заказные" пути?
		{
			PluginPathList.AddItem(Opt.LoadPlug.strCustomPluginsPath);
		}

		const wchar_t *NamePtr;

		// теперь пройдемся по всему ранее собранному списку
		for (size_t PPLI = 0; nullptr != (NamePtr = PluginPathList.Get(PPLI)); ++PPLI) {
			// расширяем значение пути
			apiExpandEnvironmentStrings(NamePtr, strFullName);
			Unquote(strFullName);	//??? здесь ХЗ

			if (!IsAbsolutePath(strFullName)) {
				strPluginsDir = g_strFarPath;
				strPluginsDir+= strFullName;
				strFullName = strPluginsDir;
			}

			// Получим реальное значение полного длинного пути
			ConvertNameToFull(strFullName, strFullName);
			strPluginsDir = strFullName;

			if (strPluginsDir.IsEmpty())	// Хмм... а нужно ли ЭТО условие после такой модернизации алгоритма загрузки?
				continue;

			// ставим на поток очередной путь из списка...
			ScTree.SetFindPath(strPluginsDir, L"*.far-plug-*", 0);

			// ...и пройдемся по нему
			while (ScTree.GetNextName(&FindData, strFullName)) {
				if (!(FindData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)) {
					// this will check filename extension
					LoadPlugin(strFullName, false);
				}
			}	// end while
		}
	}

	Flags.Set(PSIF_PLUGINSLOADDED);

	far_qsort(PluginsData, PluginsCount, sizeof(*PluginsData), PluginsSort);
}

/*
	$ 01.09.2000 tran
	Load cache only plugins  - '/co' switch
*/
void PluginManager::LoadPluginsFromCache()
{
	KeyFileReadHelper kfh(PluginsIni());
	const std::vector<std::string> &sections = kfh.EnumSections();
	FARString strModuleName;
	for (const auto &s : sections) {
		if (s != SettingsSection) {
			const std::string &module = kfh.GetString(s, "Module");
			if (!module.empty()) {
				strModuleName = module;
				LoadPlugin(strModuleName, false);
			}
		}
	}
}

int _cdecl PluginsSort(const void *el1, const void *el2)
{
	Plugin *Plugin1 = *((Plugin **)el1);
	Plugin *Plugin2 = *((Plugin **)el2);
	return (StrCmpI(PointToName(Plugin1->GetModuleName()), PointToName(Plugin2->GetModuleName())));
}

HANDLE PluginManager::OpenFilePlugin(const wchar_t *Name, int OpMode, OPENFILEPLUGINTYPE Type,
		Plugin *pDesiredPlugin)
{
	ChangePriority ChPriority(ChangePriority::NORMAL);
	ConsoleTitle ct(Opt.ShowCheckingFile ? Msg::CheckingFileInPlugin.CPtr() : nullptr);
	HANDLE hResult = INVALID_HANDLE_VALUE;
	PluginHandle *pResult = nullptr;
	TPointerArray<PluginHandle> items;
	FARString strFullName;

	if (Name) {
		ConvertNameToFull(Name, strFullName);
		Name = strFullName;
	}

	bool ShowMenu = Opt.PluginConfirm.OpenFilePlugin == BSTATE_3STATE
			? !(Type == OFP_NORMAL || Type == OFP_SEARCH)
			: Opt.PluginConfirm.OpenFilePlugin != 0;
	if (Type == OFP_ALTERNATIVE)
		OpMode|= OPM_PGDN;
	if (Type == OFP_COMMANDS)
		OpMode|= OPM_COMMANDS;

	Plugin *pPlugin = nullptr;
	std::unique_ptr<SafeMMap> smm;

	for (int i = 0; i < PluginsCount; i++) {
		pPlugin = PluginsData[i];
		if (pDesiredPlugin != nullptr && pDesiredPlugin != pPlugin)
			continue;

		if (!pPlugin->HasOpenFilePlugin() && !(pPlugin->HasAnalyse() && pPlugin->HasOpenPlugin()))
			continue;
		if ((Type == OFP_EXTRACT && !pPlugin->HasGetFiles()) || (Type == OFP_COMMANDS && !pPlugin->HasProcessHostFile()))
			continue;

		if (Name && !smm) {
			try {
				smm.reset(new SafeMMap(Wide2MB(Name).c_str(), SafeMMap::M_READ, Opt.PluginMaxReadData));
			} catch (std::exception &e) {
				fprintf(stderr, "PluginManager::OpenFilePlugin: %s\n", e.what());

				if (!OpMode) {
					Message(MSG_WARNING | MSG_ERRORTYPE, 1, MB2Wide(e.what()).c_str(),
							Msg::OpenPluginCannotOpenFile, Name, Msg::Ok);
				}
				break;
			}
		}

		if (pPlugin->HasOpenFilePlugin()) {
			if (Opt.ShowCheckingFile)
				ct.Set(L"%ls - [%ls]...", Msg::CheckingFileInPlugin.CPtr(),
						PointToName(pPlugin->GetModuleName()));

			HANDLE hPlugin = pPlugin->OpenFilePlugin(Name, smm ? (const unsigned char *)smm->View() : nullptr,
					smm ? (DWORD)smm->Length() : 0, OpMode);

			if (hPlugin == (HANDLE)-2)		// сразу на выход, плагин решил нагло обработать все сам (Autorun/PictureView)!!!
			{
				hResult = (HANDLE)-2;
				break;
			}

			if (hPlugin != INVALID_HANDLE_VALUE) {
				PluginHandle *handle = items.addItem();
				handle->hPlugin = hPlugin;
				handle->pPlugin = pPlugin;
			}
		} else {
			AnalyseData AData;
			AData.lpwszFileName = Name;
			AData.pBuffer = smm ? (const unsigned char *)smm->View() : nullptr;
			AData.dwBufferSize = smm ? (DWORD)smm->Length() : 0;
			AData.OpMode = OpMode;

			if (pPlugin->Analyse(&AData)) {
				PluginHandle *handle = items.addItem();
				handle->pPlugin = pPlugin;
				handle->hPlugin = INVALID_HANDLE_VALUE;
			}
		}

		if (items.getCount() && !ShowMenu)
			break;
	}

	if (items.getCount() && (hResult != (HANDLE)-2)) {
		bool OnlyOne = (items.getCount() == 1)
				&& !(Name && Opt.PluginConfirm.OpenFilePlugin && Opt.PluginConfirm.StandardAssociation
						&& Opt.PluginConfirm.EvenIfOnlyOnePlugin);

		if (!OnlyOne && ShowMenu) {
			VMenu menu(Msg::PluginConfirmationTitle, nullptr, 0, ScrY - 4);
			menu.SetPosition(-1, -1, 0, 0);
			menu.SetHelp(L"ChoosePluginMenu");
			menu.SetFlags(VMENU_SHOWAMPERSAND | VMENU_WRAPMODE);
			MenuItemEx mitem;

			for (size_t i = 0; i < items.getCount(); i++) {
				PluginHandle *handle = items.getItem(i);
				mitem.Clear();
				mitem.strName = PointToName(handle->pPlugin->GetModuleName());
				// NB: here is really should be used sizeof(handle), not sizeof(*handle)
				// cuz sizeof(void *) has special meaning in SetUserData!
				menu.SetUserData(handle, sizeof(handle), menu.AddItem(&mitem));
			}

			if (Opt.PluginConfirm.StandardAssociation && Type == OFP_NORMAL) {
				mitem.Clear();
				mitem.Flags|= MIF_SEPARATOR;
				menu.AddItem(&mitem);
				mitem.Clear();
				mitem.strName = Msg::MenuPluginStdAssociation;
				menu.AddItem(&mitem);
			}

			menu.Show();

			while (!menu.Done()) {
				menu.ReadInput();
				menu.ProcessInput();
			}

			if (menu.GetExitCode() == -1)
				hResult = (HANDLE)-2;
			else
				pResult = (PluginHandle *)menu.GetUserData(nullptr, 0);
		} else {
			pResult = items.getItem(0);
		}

		if (pResult && pResult->hPlugin == INVALID_HANDLE_VALUE) {
			HANDLE h = pResult->pPlugin->OpenPlugin(OPEN_ANALYSE, 0);

			if (h != INVALID_HANDLE_VALUE)
				pResult->hPlugin = h;
			else
				pResult = nullptr;
		}
	}

	for (size_t i = 0; i < items.getCount(); i++) {
		PluginHandle *handle = items.getItem(i);

		if (handle != pResult) {
			if (handle->hPlugin != INVALID_HANDLE_VALUE)
				handle->pPlugin->ClosePlugin(handle->hPlugin);
		}
	}

	if (pResult) {
		PluginHandle *pDup = new PluginHandle;
		pDup->hPlugin = pResult->hPlugin;
		pDup->pPlugin = pResult->pPlugin;
		hResult = reinterpret_cast<HANDLE>(pDup);
	}

	return hResult;
}

HANDLE PluginManager::OpenFindListPlugin(const PluginPanelItem *PanelItem, int ItemsNumber)
{
	ChangePriority ChPriority(ChangePriority::NORMAL);
	PluginHandle *pResult = nullptr;
	TPointerArray<PluginHandle> items;
	Plugin *pPlugin = nullptr;

	for (int i = 0; i < PluginsCount; i++) {
		pPlugin = PluginsData[i];

		if (!pPlugin->HasSetFindList())
			continue;

		HANDLE hPlugin = pPlugin->OpenPlugin(OPEN_FINDLIST, 0);

		if (hPlugin != INVALID_HANDLE_VALUE) {
			PluginHandle *handle = items.addItem();
			handle->hPlugin = hPlugin;
			handle->pPlugin = pPlugin;
		}

		if (items.getCount() && !Opt.PluginConfirm.SetFindList)
			break;
	}

	if (items.getCount()) {
		if (items.getCount() > 1) {
			VMenu menu(Msg::PluginConfirmationTitle, nullptr, 0, ScrY - 4);
			menu.SetPosition(-1, -1, 0, 0);
			menu.SetHelp(L"ChoosePluginMenu");
			menu.SetFlags(VMENU_SHOWAMPERSAND | VMENU_WRAPMODE);
			MenuItemEx mitem;

			for (size_t i = 0; i < items.getCount(); i++) {
				PluginHandle *handle = items.getItem(i);
				mitem.Clear();
				mitem.strName = PointToName(handle->pPlugin->GetModuleName());
				menu.AddItem(&mitem);
			}

			menu.Show();

			while (!menu.Done()) {
				menu.ReadInput();
				menu.ProcessInput();
			}

			int ExitCode = menu.GetExitCode();

			if (ExitCode >= 0) {
				pResult = items.getItem(ExitCode);
			}
		} else {
			pResult = items.getItem(0);
		}
	}

	if (pResult) {
		if (!pResult->pPlugin->SetFindList(pResult->hPlugin, PanelItem, ItemsNumber)) {
			pResult = nullptr;
		}
	}

	for (size_t i = 0; i < items.getCount(); i++) {
		PluginHandle *handle = items.getItem(i);

		if (handle != pResult) {
			if (handle->hPlugin != INVALID_HANDLE_VALUE)
				handle->pPlugin->ClosePlugin(handle->hPlugin);
		}
	}

	if (pResult) {
		PluginHandle *pDup = new PluginHandle;
		pDup->hPlugin = pResult->hPlugin;
		pDup->pPlugin = pResult->pPlugin;
		pResult = pDup;
	}

	return pResult ? reinterpret_cast<HANDLE>(pResult) : INVALID_HANDLE_VALUE;
}

void PluginManager::ClosePlugin(HANDLE hPlugin)
{
	ChangePriority ChPriority(ChangePriority::NORMAL);
	PluginHandle *ph = (PluginHandle *)hPlugin;
	const auto RefCnt = ph->RefCnt;
	ASSERT(RefCnt > 0);
	ph->RefCnt = RefCnt - 1;
	if (RefCnt == 1) {
		ph->pPlugin->ClosePlugin(ph->hPlugin);
		delete ph;
	}
}

void PluginManager::RetainPlugin(HANDLE hPlugin)
{
	PluginHandle *ph = (PluginHandle *)hPlugin;
	const auto RefCnt = ph->RefCnt;
	ASSERT(RefCnt > 0);
	ph->RefCnt = RefCnt + 1;
}

HANDLE PluginManager::GetRealPluginHandle(HANDLE hPlugin)
{
	PluginHandle *ph = (PluginHandle *)hPlugin;
	return ph->hPlugin;
}

FARString PluginManager::GetPluginModuleName(HANDLE hPlugin)
{
	PluginHandle *ph = (PluginHandle *)hPlugin;
	return ph->pPlugin->GetModuleName();
}

int PluginManager::ProcessEditorInput(INPUT_RECORD *Rec)
{
	for (int i = 0; i < PluginsCount; i++) {
		Plugin *pPlugin = PluginsData[i];

		if (pPlugin->HasProcessEditorInput() && pPlugin->ProcessEditorInput(Rec))
			return TRUE;
	}

	return FALSE;
}

int PluginManager::ProcessEditorEvent(int Event, void *Param)
{
	int nResult = 0;

	if (CtrlObject->Plugins.CurEditor) {
		Plugin *pPlugin = nullptr;

		for (int i = 0; i < PluginsCount; i++) {
			pPlugin = PluginsData[i];

			if (pPlugin->HasProcessEditorEvent())
				nResult = pPlugin->ProcessEditorEvent(Event, Param);
		}
	}

	return nResult;
}

int PluginManager::ProcessViewerEvent(int Event, void *Param)
{
	int nResult = 0;

	for (int i = 0; i < PluginsCount; i++) {
		Plugin *pPlugin = PluginsData[i];

		if (pPlugin->HasProcessViewerEvent())
			nResult = pPlugin->ProcessViewerEvent(Event, Param);
	}

	return nResult;
}

int PluginManager::ProcessDialogEvent(int Event, void *Param)
{
	for (int i = 0; i < PluginsCount; i++) {
		Plugin *pPlugin = PluginsData[i];

		if (pPlugin->HasProcessDialogEvent() && pPlugin->ProcessDialogEvent(Event, Param))
			return TRUE;
	}

	return FALSE;
}

#if defined(PROCPLUGINMACROFUNC)
int PluginManager::ProcessMacroFunc(const wchar_t *Name, const FarMacroValue *Params, int nParams,
		FarMacroValue **Results, int *nResults)
{
	int nResult = 0;

	for (int i = 0; i < PluginsCount; i++) {
		Plugin *pPlugin = PluginsData[i];

		if (pPlugin->HasProcessMacroFunc())
			if ((nResult = pPlugin->ProcessMacroFunc(Name, Params, nParams, Results, nResults)) != 0)
				break;
	}

	return nResult;
}
#endif

int PluginManager::GetFindData(HANDLE hPlugin, PluginPanelItem **pPanelData, int *pItemsNumber, int OpMode)
{
	ChangePriority ChPriority(ChangePriority::NORMAL);
	PluginHandle *ph = (PluginHandle *)hPlugin;
	*pItemsNumber = 0;
	return ph->pPlugin->GetFindData(ph->hPlugin, pPanelData, pItemsNumber, OpMode);
}

void PluginManager::FreeFindData(HANDLE hPlugin, PluginPanelItem *PanelItem, int ItemsNumber)
{
	PluginHandle *ph = (PluginHandle *)hPlugin;
	ph->pPlugin->FreeFindData(ph->hPlugin, PanelItem, ItemsNumber);
}

int PluginManager::GetVirtualFindData(HANDLE hPlugin, PluginPanelItem **pPanelData, int *pItemsNumber,
		const wchar_t *Path)
{
	ChangePriority ChPriority(ChangePriority::NORMAL);
	PluginHandle *ph = (PluginHandle *)hPlugin;
	*pItemsNumber = 0;
	return ph->pPlugin->GetVirtualFindData(ph->hPlugin, pPanelData, pItemsNumber, Path);
}

void PluginManager::FreeVirtualFindData(HANDLE hPlugin, PluginPanelItem *PanelItem, int ItemsNumber)
{
	PluginHandle *ph = (PluginHandle *)hPlugin;
	return ph->pPlugin->FreeVirtualFindData(ph->hPlugin, PanelItem, ItemsNumber);
}

int PluginManager::SetDirectory(HANDLE hPlugin, const wchar_t *Dir, int OpMode)
{
	ChangePriority ChPriority(ChangePriority::NORMAL);
	PluginHandle *ph = (PluginHandle *)hPlugin;
	return ph->pPlugin->SetDirectory(ph->hPlugin, Dir, OpMode);
}

int PluginManager::GetFile(HANDLE hPlugin, PluginPanelItem *PanelItem, const wchar_t *DestPath,
		FARString &strResultName, int OpMode)
{
	ChangePriority ChPriority(ChangePriority::NORMAL);
	PluginHandle *ph = (PluginHandle *)hPlugin;
	SaveScreen *SaveScr = nullptr;
	int Found = FALSE;
	KeepUserScreen = FALSE;

	if (!(OpMode & OPM_FIND))
		SaveScr = new SaveScreen;	//???

	UndoGlobalSaveScrPtr UndSaveScr(SaveScr);
	int GetCode = ph->pPlugin->GetFiles(ph->hPlugin, PanelItem, 1, 0, &DestPath, OpMode);
	FARString strFindPath;
	strFindPath = DestPath;
	AddEndSlash(strFindPath);
	strFindPath+= L"*";
	FAR_FIND_DATA_EX fdata;
	FindFile Find(strFindPath);
	bool Done = true;
	while (Find.Get(fdata)) {
		if (!(fdata.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)) {
			Done = false;
			break;
		}
	}

	if (!Done) {
		strResultName = DestPath;
		AddEndSlash(strResultName);
		strResultName+= fdata.strFileName;

		if (GetCode != 1) {
			apiSetFileAttributes(strResultName, FILE_ATTRIBUTE_NORMAL);
			apiDeleteFile(strResultName);	// BUGBUG
		} else
			Found = TRUE;
	}

	ReadUserBackground(SaveScr);
	delete SaveScr;
	return Found;
}

int PluginManager::DeleteFiles(HANDLE hPlugin, PluginPanelItem *PanelItem, int ItemsNumber, int OpMode)
{
	ChangePriority ChPriority(ChangePriority::NORMAL);
	PluginHandle *ph = (PluginHandle *)hPlugin;
	SaveScreen SaveScr;
	KeepUserScreen = FALSE;
	int Code = ph->pPlugin->DeleteFiles(ph->hPlugin, PanelItem, ItemsNumber, OpMode);

	if (Code)
		ReadUserBackground(&SaveScr);	//???

	return Code;
}

int PluginManager::MakeDirectory(HANDLE hPlugin, const wchar_t **Name, int OpMode)
{
	ChangePriority ChPriority(ChangePriority::NORMAL);
	PluginHandle *ph = (PluginHandle *)hPlugin;
	SaveScreen SaveScr;
	KeepUserScreen = FALSE;
	int Code = ph->pPlugin->MakeDirectory(ph->hPlugin, Name, OpMode);

	if (Code != -1)		//???BUGBUG
		ReadUserBackground(&SaveScr);

	return Code;
}

int PluginManager::ProcessHostFile(HANDLE hPlugin, PluginPanelItem *PanelItem, int ItemsNumber, int OpMode)
{
	PluginHandle *ph = (PluginHandle *)hPlugin;
	ChangePriority ChPriority(ChangePriority::NORMAL);
	SaveScreen SaveScr;
	KeepUserScreen = FALSE;
	int Code = ph->pPlugin->ProcessHostFile(ph->hPlugin, PanelItem, ItemsNumber, OpMode);

	if (Code)	// BUGBUG
		ReadUserBackground(&SaveScr);

	return Code;
}

bool PluginManager::GetLinkTarget(HANDLE hPlugin, PluginPanelItem *PanelItem, FARString &result, int OpMode)
{
	ChangePriority ChPriority(ChangePriority::NORMAL);
	PluginHandle *ph = (PluginHandle *)hPlugin;
	return ph->pPlugin->GetLinkTarget(ph->hPlugin, PanelItem, result, OpMode);
}

int PluginManager::GetFiles(HANDLE hPlugin, PluginPanelItem *PanelItem, int ItemsNumber, int Move,
		const wchar_t **DestPath, int OpMode)
{
	ChangePriority ChPriority(ChangePriority::NORMAL);
	PluginHandle *ph = (PluginHandle *)hPlugin;
	return ph->pPlugin->GetFiles(ph->hPlugin, PanelItem, ItemsNumber, Move, DestPath, OpMode);
}

int PluginManager::PutFiles(HANDLE hPlugin, PluginPanelItem *PanelItem, int ItemsNumber, int Move, int OpMode)
{
	PluginHandle *ph = (PluginHandle *)hPlugin;
	ChangePriority ChPriority(ChangePriority::NORMAL);
	SaveScreen SaveScr;
	KeepUserScreen = FALSE;
	int Code = ph->pPlugin->PutFiles(ph->hPlugin, PanelItem, ItemsNumber, Move, OpMode);

	if (Code)	// BUGBUG
		ReadUserBackground(&SaveScr);

	return Code;
}

void PluginManager::GetOpenPluginInfo(HANDLE hPlugin, OpenPluginInfo *Info)
{
	if (!Info)
		return;

	memset(Info, 0, sizeof(*Info));
	PluginHandle *ph = (PluginHandle *)hPlugin;
	ph->pPlugin->GetOpenPluginInfo(ph->hPlugin, Info);

	if (!Info->CurDir)	// хмм...
		Info->CurDir = L"";

	if ((Info->Flags & OPIF_REALNAMES) && (CtrlObject->Cp()->ActivePanel->GetPluginHandle() == hPlugin)
			&& *Info->CurDir && !IsNetworkServerPath(Info->CurDir))
		apiSetCurrentDirectory(Info->CurDir, false);
}

int PluginManager::ProcessKey(HANDLE hPlugin, int Key, unsigned int ControlState)
{
	PluginHandle *ph = (PluginHandle *)hPlugin;
	return ph->pPlugin->ProcessKey(ph->hPlugin, Key, ControlState);
}

int PluginManager::ProcessEvent(HANDLE hPlugin, int Event, void *Param)
{
	PluginHandle *ph = (PluginHandle *)hPlugin;
	return ph->pPlugin->ProcessEvent(ph->hPlugin, Event, Param);
}

int PluginManager::Compare(HANDLE hPlugin, const PluginPanelItem *Item1, const PluginPanelItem *Item2,
		unsigned int Mode)
{
	PluginHandle *ph = (PluginHandle *)hPlugin;
	return ph->pPlugin->Compare(ph->hPlugin, Item1, Item2, Mode);
}

void PluginManager::ConfigureCurrent(Plugin *pPlugin, int INum)
{
	if (pPlugin->Configure(INum)) {
		int PMode[2];
		PMode[0] = CtrlObject->Cp()->LeftPanel->GetMode();
		PMode[1] = CtrlObject->Cp()->RightPanel->GetMode();

		for (size_t I = 0; I < ARRAYSIZE(PMode); ++I) {
			if (PMode[I] == PLUGIN_PANEL) {
				Panel *pPanel = (I ? CtrlObject->Cp()->RightPanel : CtrlObject->Cp()->LeftPanel);
				pPanel->Update(UPDATE_KEEP_SELECTION);
				pPanel->SetViewMode(pPanel->GetViewMode());
				pPanel->Redraw();
			}
		}
		pPlugin->SaveToCache();
	}
}

struct PluginMenuItemData
{
	Plugin *pPlugin;
	int nItem;
};

/*
	$ 29.05.2001 IS
	! При настройке "параметров внешних модулей" закрывать окно с их
	списком только при нажатии на ESC
*/

bool PluginManager::CheckIfHotkeyPresent(HotKeyKind Kind)
{
	for (int I = 0; I < PluginsCount; I++) {
		Plugin *pPlugin = PluginsData[I];
		PluginInfo Info{};
		bool bCached = pPlugin->CheckWorkFlags(PIWF_CACHED) ? true : false;
		if (!bCached && !pPlugin->GetPluginInfo(&Info)) {
			continue;
		}

		for (int J = 0;; ++J) {
			if (bCached) {
				const char *MenuNameFmt =
						(Kind == HKK_CONFIG) ? FmtPluginConfigStringD : FmtPluginMenuStringD;
				KeyFileReadSection kfh(PluginsIni(), pPlugin->GetSettingsName());
				const auto &key_name = StrPrintf(MenuNameFmt, J);
				if (!kfh.HasKey(key_name))
					break;
			} else if (J >= ((Kind == HKK_CONFIG) ? Info.PluginConfigStringsNumber
												: Info.PluginMenuStringsNumber)) {
				break;
			}

			FARString strHotKey;
			GetPluginHotKey(pPlugin, J, Kind, strHotKey);
			if (!strHotKey.IsEmpty()) {
				return true;
			}
		}
	}
	return false;
}

void PluginManager::Configure(int StartPos)
{
	// Полиция 4 - Параметры внешних модулей
	if (Opt.Policies.DisabledOptions & FFPOL_MAINMENUPLUGINS)
		return;

	{
		VMenu PluginList(Msg::PluginConfigTitle, nullptr, 0, ScrY - 4);
		PluginList.SetFlags(VMENU_WRAPMODE);
		PluginList.SetHelp(L"PluginsConfig");

		for (;;) {
			BOOL NeedUpdateItems = TRUE;
			int MenuItemNumber = 0;
			bool HotKeysPresent = CheckIfHotkeyPresent(HKK_CONFIG);

			if (NeedUpdateItems) {
				PluginList.ClearDone();
				PluginList.DeleteItems();
				PluginList.SetPosition(-1, -1, 0, 0);
				MenuItemNumber = 0;
				LoadIfCacheAbsent();
				FARString strHotKey, strValue, strName;
				PluginInfo Info{};

				for (int I = 0; I < PluginsCount; I++) {
					Plugin *pPlugin = PluginsData[I];
					bool bCached = pPlugin->CheckWorkFlags(PIWF_CACHED) ? true : false;

					if (!bCached && !pPlugin->GetPluginInfo(&Info)) {
						continue;
					}

					for (int J = 0;; J++) {
						if (bCached) {
							KeyFileReadSection kfh(PluginsIni(), pPlugin->GetSettingsName());
							const std::string &key = StrPrintf(FmtPluginConfigStringD, J);
							if (!kfh.HasKey(key))
								break;

							strName = kfh.GetString(key, "");
						} else {
							if (J >= Info.PluginConfigStringsNumber)
								break;

							strName = Info.PluginConfigStrings[J];
						}

						GetPluginHotKey(pPlugin, J, HKK_CONFIG, strHotKey);
						MenuItemEx ListItem;
						ListItem.Clear();

						if (!HotKeysPresent)
							ListItem.strName = strName;
						else if (!strHotKey.IsEmpty())
							ListItem.strName.Format(L"&%lc%ls  %ls", strHotKey.At(0),
									(strHotKey.At(0) == L'&' ? L"&" : L""), strName.CPtr());
						else
							ListItem.strName.Format(L"   %ls", strName.CPtr());

						// ListItem.SetSelect(MenuItemNumber++ == StartPos);
						MenuItemNumber++;
						PluginMenuItemData item;
						item.pPlugin = pPlugin;
						item.nItem = J;
						PluginList.SetUserData(&item, sizeof(PluginMenuItemData),
								PluginList.AddItem(&ListItem));
					}
				}

				PluginList.AssignHighlights(FALSE);
				PluginList.SetBottomTitle(Msg::PluginHotKeyBottom);
				PluginList.ClearDone();
				PluginList.SortItems(0, HotKeysPresent ? 3 : 0);
				PluginList.SetSelectPos(StartPos, 1);
				NeedUpdateItems = FALSE;
			}

			FARString strPluginModuleName;
			PluginList.Show();

			while (!PluginList.Done()) {
				const auto Key = PluginList.ReadInput();
				int SelPos = PluginList.GetSelectPos();
				PluginMenuItemData *item = (PluginMenuItemData *)PluginList.GetUserData(nullptr, 0, SelPos);

				switch (Key) {
					case KEY_SHIFTF1:
						if (item)
						{
							strPluginModuleName = item->pPlugin->GetModuleName();

							if (!FarShowHelp(strPluginModuleName, L"Config", FHELP_SELFHELP | FHELP_NOSHOWERROR)
									&& !FarShowHelp(strPluginModuleName, L"Configure",
											FHELP_SELFHELP | FHELP_NOSHOWERROR)) {
								FarShowHelp(strPluginModuleName, nullptr, FHELP_SELFHELP | FHELP_NOSHOWERROR);
							}
						}

						break;
					case KEY_F4:
						if (item && PluginList.GetItemCount() > 0 && SelPos < MenuItemNumber) {
							FARString strName00;
							int nOffset = HotKeysPresent ? 3 : 0;
							strName00 = PluginList.GetItemPtr()->strName.CPtr() + nOffset;
							RemoveExternalSpaces(strName00);

							if (SetHotKeyDialog(strName00,
										GetHotKeySettingName(item->pPlugin, item->nItem, HKK_CONFIG))) {
								PluginList.Hide();
								NeedUpdateItems = TRUE;
								StartPos = SelPos;
								PluginList.SetExitCode(SelPos);
								PluginList.Show();
								break;
							}
						}

						break;
					default:
						PluginList.ProcessInput();
						break;
				}
			}

			if (!NeedUpdateItems) {
				StartPos = PluginList.Modal::GetExitCode();
				PluginList.Hide();

				if (StartPos < 0)
					break;

				PluginMenuItemData *item = (PluginMenuItemData *)PluginList.GetUserData(nullptr, 0, StartPos);
				ConfigureCurrent(item->pPlugin, item->nItem);
			}
		}
	}
}

int PluginManager::CommandsMenu(int ModalType, int StartPos, const wchar_t *HistoryName)
{
	if (ModalType == MODALTYPE_DIALOG) {
		if (reinterpret_cast<Dialog *>(FrameManager->GetCurrentFrame())->CheckDialogMode(DMODE_NOPLUGINS)) {
			return 0;
		}
	}

	int MenuItemNumber = 0;
	int Editor = ModalType == MODALTYPE_EDITOR, Viewer = ModalType == MODALTYPE_VIEWER,
		Dialog = ModalType == MODALTYPE_DIALOG;
	PluginMenuItemData item;
	{
		ChangeMacroMode CMM(MACRO_MENU);
		VMenu PluginList(Msg::PluginCommandsMenuTitle, nullptr, 0, ScrY - 4);
		PluginList.SetFlags(VMENU_WRAPMODE);
		PluginList.SetHelp(L"PluginCommands");
		BOOL NeedUpdateItems = TRUE;
		BOOL Done = FALSE;

		while (!Done) {
			bool HotKeysPresent = CheckIfHotkeyPresent(HKK_MENU);

			if (NeedUpdateItems) {
				PluginList.ClearDone();
				PluginList.DeleteItems();
				PluginList.SetPosition(-1, -1, 0, 0);
				LoadIfCacheAbsent();
				FARString strHotKey, strValue, strName;
				PluginInfo Info{};
				KeyFileReadHelper kfh(PluginsIni());

				for (int I = 0; I < PluginsCount; I++) {
					Plugin *pPlugin = PluginsData[I];
					bool bCached = pPlugin->CheckWorkFlags(PIWF_CACHED) ? true : false;
					int IFlags;

					if (bCached) {
						IFlags = kfh.GetUInt(pPlugin->GetSettingsName(), "Flags", 0);
					} else {
						if (!pPlugin->GetPluginInfo(&Info))
							continue;

						IFlags = Info.Flags;
					}

					if ((Editor && !(IFlags & PF_EDITOR)) || (Viewer && !(IFlags & PF_VIEWER))
							|| (Dialog && !(IFlags & PF_DIALOG))
							|| (!Editor && !Viewer && !Dialog && (IFlags & PF_DISABLEPANELS)))
						continue;

					for (int J = 0;; J++) {
						if (bCached) {
							const std::string &key = StrPrintf(FmtPluginMenuStringD, J);
							if (!kfh.HasKey(pPlugin->GetSettingsName(), key))
								break;
							strName = kfh.GetString(pPlugin->GetSettingsName(), key, "");
						} else {
							if (J >= Info.PluginMenuStringsNumber)
								break;

							strName = Info.PluginMenuStrings[J];
						}

						GetPluginHotKey(pPlugin, J, HKK_MENU, strHotKey);
						MenuItemEx ListItem;
						ListItem.Clear();

						if (!HotKeysPresent)
							ListItem.strName = strName;
						else if (!strHotKey.IsEmpty())
							ListItem.strName.Format(L"&%lc%ls  %ls", strHotKey.At(0),
									(strHotKey.At(0) == L'&' ? L"&" : L""), strName.CPtr());
						else
							ListItem.strName.Format(L"   %ls", strName.CPtr());

						// ListItem.SetSelect(MenuItemNumber++ == StartPos);
						MenuItemNumber++;
						PluginMenuItemData item;
						item.pPlugin = pPlugin;
						item.nItem = J;
						PluginList.SetUserData(&item, sizeof(PluginMenuItemData),
								PluginList.AddItem(&ListItem));
					}
				}

				PluginList.AssignHighlights(FALSE);
				PluginList.SetBottomTitle(Msg::PluginHotKeyBottom);
				PluginList.SortItems(0, HotKeysPresent ? 3 : 0);
				PluginList.SetSelectPos(StartPos, 1);
				NeedUpdateItems = FALSE;
			}

			PluginList.Show();

			while (!PluginList.Done()) {
				const auto Key = PluginList.ReadInput();
				int SelPos = PluginList.GetSelectPos();
				PluginMenuItemData *item = (PluginMenuItemData *)PluginList.GetUserData(nullptr, 0, SelPos);

				switch (Key) {
					case KEY_SHIFTF1:
						// Вызываем нужный топик, который передали в CommandsMenu()
						if (item)
							FarShowHelp(item->pPlugin->GetModuleName(), HistoryName,
									FHELP_SELFHELP | FHELP_NOSHOWERROR | FHELP_USECONTENTS);
						break;
					case KEY_ALTF11:
						// todo WriteEvent(FLOG_PLUGINSINFO);
						break;
					case KEY_F4:
						if (item && PluginList.GetItemCount() > 0 && SelPos < MenuItemNumber) {
							FARString strName00;
							int nOffset = HotKeysPresent ? 3 : 0;
							strName00 = PluginList.GetItemPtr()->strName.CPtr() + nOffset;
							RemoveExternalSpaces(strName00);

							if (SetHotKeyDialog(strName00,
										GetHotKeySettingName(item->pPlugin, item->nItem, HKK_MENU))) {
								PluginList.Hide();
								NeedUpdateItems = TRUE;
								StartPos = SelPos;
								PluginList.SetExitCode(SelPos);
								PluginList.Show();
							}
						}
						break;
					case KEY_ALTSHIFTF9:
						PluginList.Hide();
						NeedUpdateItems = TRUE;
						StartPos = SelPos;
						PluginList.SetExitCode(SelPos);
						Configure();
						PluginList.Show();
						break;
					case KEY_SHIFTF9:
						if (item && PluginList.GetItemCount() > 0 && SelPos < MenuItemNumber) {
							NeedUpdateItems = TRUE;
							StartPos = SelPos;

							if (item->pPlugin->HasConfigure())
								ConfigureCurrent(item->pPlugin, item->nItem);

							PluginList.SetExitCode(SelPos);
							PluginList.Show();
						}
						break;
					default:
						PluginList.ProcessInput();
						break;
				}
			}

			if (!NeedUpdateItems && PluginList.Done())
				break;
		}

		int ExitCode = PluginList.Modal::GetExitCode();
		PluginList.Hide();

		if (ExitCode < 0) {
			return FALSE;
		}

		ScrBuf.Flush();
		item = *(PluginMenuItemData *)PluginList.GetUserData(nullptr, 0, ExitCode);
	}

	Panel *ActivePanel = CtrlObject->Cp()->ActivePanel;
	int OpenCode = OPEN_PLUGINSMENU;
	INT_PTR Item = item.nItem;
	OpenDlgPluginData pd;

	if (Editor) {
		OpenCode = OPEN_EDITOR;
	} else if (Viewer) {
		OpenCode = OPEN_VIEWER;
	} else if (Dialog) {
		OpenCode = OPEN_DIALOG;
		pd.hDlg = (HANDLE)FrameManager->GetCurrentFrame();
		pd.ItemNumber = item.nItem;
		Item = (INT_PTR)&pd;
	}

	HANDLE hPlugin = OpenPlugin(item.pPlugin, OpenCode, Item);

	if (hPlugin != INVALID_HANDLE_VALUE && !Editor && !Viewer && !Dialog) {
		if (ActivePanel->ProcessPluginEvent(FE_CLOSE, nullptr)) {
			ClosePlugin(hPlugin);
			return FALSE;
		}

		Panel *NewPanel = CtrlObject->Cp()->ChangePanel(ActivePanel, FILE_PANEL, TRUE, TRUE);
		NewPanel->SetPluginMode(hPlugin, L"", true);
		NewPanel->Update(0);
		NewPanel->Show();
	}

	// restore title for old plugins only.
	if (item.pPlugin->IsPluginMB() && Editor && CurEditor) {
		CurEditor->SetPluginTitle(nullptr);
	}

	return TRUE;
}

std::string PluginManager::GetHotKeySettingName(Plugin *pPlugin, int ItemNumber, HotKeyKind Kind)
{
	const char *NamePart;
	switch (Kind) {
		case HKK_CONFIG:
			NamePart = "ConfHotkey";
			break;
		case HKK_MENU:
			NamePart = "Hotkey";
			break;
		case HKK_DRIVEMENU:
			NamePart = "DriveMenuHotkey";
			break;
		default:
			fprintf(stderr, "%s: wrong kind %u\n", __FUNCTION__, Kind);
			abort();
	}
	std::string out = pPlugin->GetSettingsName();
	out+= StrPrintf(":%s#%d", NamePart, ItemNumber);
	return out;
}

void PluginManager::GetPluginHotKey(Plugin *pPlugin, int ItemNumber, HotKeyKind Kind, FARString &strHotKey)
{
	strHotKey = KeyFileReadSection(PluginsIni(), SettingsSection)
						.GetString(GetHotKeySettingName(pPlugin, ItemNumber, Kind));
}

bool PluginManager::SetHotKeyDialog(const wchar_t *DlgPluginTitle,		// имя плагина
		const std::string &SettingName									// ключ, откуда берем значение в state.ini/Settings
)
{
	KeyFileHelper kfh(PluginsIni());
	const auto &Setting = kfh.GetString(SettingsSection, SettingName, L"");
	WCHAR Letter[2] = {Setting.empty() ? 0 : Setting[0], 0};
	if (!HotkeyLetterDialog(Msg::PluginHotKeyTitle, DlgPluginTitle, Letter[0]))
		return false;

	if (Letter[0])
		kfh.SetString(SettingsSection, SettingName, Letter);
	else
		kfh.RemoveKey(SettingsSection, SettingName);
	return true;
}

bool PluginManager::GetDiskMenuItem(Plugin *pPlugin, int PluginItem, bool &ItemPresent, wchar_t &PluginHotkey,
		FARString &strPluginText)
{
	LoadIfCacheAbsent();

	FARString strHotKey;
	GetPluginHotKey(pPlugin, PluginItem, HKK_DRIVEMENU, strHotKey);
	PluginHotkey = strHotKey.At(0);

	if (pPlugin->CheckWorkFlags(PIWF_CACHED)) {
		KeyFileReadSection kfh(PluginsIni(), pPlugin->GetSettingsName());
		strPluginText = kfh.GetString(StrPrintf(FmtDiskMenuStringD, PluginItem), "");
		ItemPresent = !strPluginText.IsEmpty();
		return true;
	}

	PluginInfo Info;

	if (!pPlugin->GetPluginInfo(&Info) || Info.DiskMenuStringsNumber <= PluginItem) {
		ItemPresent = false;
	} else {
		strPluginText = Info.DiskMenuStrings[PluginItem];
		ItemPresent = true;
	}

	return true;
}

int PluginManager::UseFarCommand(HANDLE hPlugin, int CommandType)
{
	OpenPluginInfo Info;
	GetOpenPluginInfo(hPlugin, &Info);

	if (!(Info.Flags & OPIF_REALNAMES))
		return FALSE;

	PluginHandle *ph = (PluginHandle *)hPlugin;

	switch (CommandType) {
		case PLUGIN_FARGETFILE:
		case PLUGIN_FARGETFILES:
			return (!ph->pPlugin->HasGetFiles() || (Info.Flags & OPIF_EXTERNALGET));
		case PLUGIN_FARPUTFILES:
			return (!ph->pPlugin->HasPutFiles() || (Info.Flags & OPIF_EXTERNALPUT));
		case PLUGIN_FARDELETEFILES:
			return (!ph->pPlugin->HasDeleteFiles() || (Info.Flags & OPIF_EXTERNALDELETE));
		case PLUGIN_FARMAKEDIRECTORY:
			return (!ph->pPlugin->HasMakeDirectory() || (Info.Flags & OPIF_EXTERNALMKDIR));
	}

	return TRUE;
}

void PluginManager::ReloadLanguage()
{
	Plugin *PData;

	for (int I = 0; I < PluginsCount; I++) {
		PData = PluginsData[I];
		PData->CloseLang();
	}

	DiscardCache();
}

void PluginManager::DiscardCache()
{
	for (int I = 0; I < PluginsCount; I++) {
		Plugin *pPlugin = PluginsData[I];
		pPlugin->Load();
	}

	KeyFileHelper kfh(PluginsIni());
	const std::vector<std::string> &sections = kfh.EnumSections();
	for (const auto &s : sections) {
		if (s != SettingsSection)
			kfh.RemoveSection(s);
	}
}

void PluginManager::LoadIfCacheAbsent()
{
	struct stat st;
	if (stat(PluginsIni(), &st) == -1) {
		for (int I = 0; I < PluginsCount; I++) {
			Plugin *pPlugin = PluginsData[I];
			pPlugin->Load();
		}
	}
}

// template parameters must have external linkage
struct PluginData
{
	Plugin *pPlugin;
	DWORD PluginFlags;
};

int PluginManager::ProcessCommandLine(const wchar_t *CommandParam, Panel *Target)
{
	size_t PrefixLength = 0;
	FARString strCommand = CommandParam;
	UnquoteExternal(strCommand);
	RemoveLeadingSpaces(strCommand);

	for (;;) {
		wchar_t Ch = strCommand.At(PrefixLength);

		if (!Ch || IsSpace(Ch) || Ch == L'/' || PrefixLength > 64)
			return FALSE;

		if (Ch == L':' && PrefixLength > 0)
			break;

		PrefixLength++;
	}

	LoadIfCacheAbsent();
	FARString strPrefix(strCommand, PrefixLength);
	FARString strPluginPrefix;
	TPointerArray<PluginData> items;

	for (int I = 0; I < PluginsCount; I++) {
		int PluginFlags = 0;

		if (PluginsData[I]->CheckWorkFlags(PIWF_CACHED)) {
			KeyFileReadSection kfh(PluginsIni(), PluginsData[I]->GetSettingsName());
			strPluginPrefix = kfh.GetString("CommandPrefix", "");
			PluginFlags = kfh.GetUInt("Flags", 0);
		} else {
			PluginInfo Info;

			if (PluginsData[I]->GetPluginInfo(&Info)) {
				strPluginPrefix = Info.CommandPrefix;
				PluginFlags = Info.Flags;
			} else
				continue;
		}

		if (strPluginPrefix.IsEmpty())
			continue;

		const wchar_t *PrStart = strPluginPrefix;
		PrefixLength = strPrefix.GetLength();

		for (;;) {
			const wchar_t *PrEnd = wcschr(PrStart, L':');
			size_t Len = PrEnd ? (PrEnd - PrStart) : StrLength(PrStart);

			if (Len < PrefixLength)
				Len = PrefixLength;

			if (!StrCmpNI(strPrefix, PrStart, (int)Len)) {
				if (PluginsData[I]->Load() && PluginsData[I]->HasOpenPlugin()) {
					PluginData *pD = items.addItem();
					pD->pPlugin = PluginsData[I];
					pD->PluginFlags = PluginFlags;
					break;
				}
			}

			if (!PrEnd)
				break;

			PrStart = ++PrEnd;
		}

		if (items.getCount() && !Opt.PluginConfirm.Prefix)
			break;
	}

	if (!items.getCount())
		return FALSE;

	Panel *ActivePanel = CtrlObject->Cp()->ActivePanel;
	Panel *CurPanel = (Target) ? Target : ActivePanel;

	if (CurPanel->ProcessPluginEvent(FE_CLOSE, nullptr))
		return FALSE;

	PluginData *PData = nullptr;

	if (items.getCount() > 1) {
		VMenu menu(Msg::PluginConfirmationTitle, nullptr, 0, ScrY - 4);
		menu.SetPosition(-1, -1, 0, 0);
		menu.SetHelp(L"ChoosePluginMenu");
		menu.SetFlags(VMENU_SHOWAMPERSAND | VMENU_WRAPMODE);
		MenuItemEx mitem;

		for (size_t i = 0; i < items.getCount(); i++) {
			mitem.Clear();
			mitem.strName = PointToName(items.getItem(i)->pPlugin->GetModuleName());
			menu.AddItem(&mitem);
		}

		menu.Show();

		while (!menu.Done()) {
			menu.ReadInput();
			menu.ProcessInput();
		}

		int ExitCode = menu.GetExitCode();

		if (ExitCode >= 0) {
			PData = items.getItem(ExitCode);
		}
	} else {
		PData = items.getItem(0);
	}

	if (PData) {
		CtrlObject->CmdLine->SetString(L"");
		FARString strPluginCommand =
				strCommand.CPtr() + (PData->PluginFlags & PF_FULLCMDLINE ? 0 : PrefixLength + 1);
		RemoveTrailingSpaces(strPluginCommand);
		HANDLE hPlugin = OpenPlugin(PData->pPlugin, OPEN_COMMANDLINE, (INT_PTR)strPluginCommand.CPtr());	// BUGBUG

		if (hPlugin != INVALID_HANDLE_VALUE) {
			Panel *NewPanel = CtrlObject->Cp()->ChangePanel(CurPanel, FILE_PANEL, TRUE, TRUE);
			NewPanel->SetPluginMode(hPlugin, L"", !Target || Target == ActivePanel);
			NewPanel->Update(0);
			NewPanel->Show();
		}
	}

	return TRUE;
}

void PluginManager::ReadUserBackground(SaveScreen *SaveScr)
{
	FilePanels *FPanel = CtrlObject->Cp();
	FPanel->LeftPanel->ProcessingPluginCommand++;
	FPanel->RightPanel->ProcessingPluginCommand++;

	if (KeepUserScreen) {
		if (SaveScr)
			SaveScr->Discard();

		RedrawDesktop Redraw;
	}

	FPanel->LeftPanel->ProcessingPluginCommand--;
	FPanel->RightPanel->ProcessingPluginCommand--;
}

/*
	$ 27.09.2000 SVS
	Функция CallPlugin - найти плагин по ID и запустить
	в зачаточном состоянии!
*/
int PluginManager::CallPlugin(DWORD SysID, int OpenFrom, void *Data, int *Ret)
{
	Plugin *pPlugin = FindPlugin(SysID);

	if (pPlugin) {
		if (pPlugin->HasOpenPlugin()) {
			HANDLE hNewPlugin = OpenPlugin(pPlugin, OpenFrom, (INT_PTR)Data);
			bool process = false;

			if (OpenFrom & OPEN_FROMMACRO) {
				// <????>
				;
				// </????>
			} else {
				process = OpenFrom == OPEN_PLUGINSMENU || OpenFrom == OPEN_FILEPANEL;
			}

			if (hNewPlugin != INVALID_HANDLE_VALUE && process) {
				int CurFocus = CtrlObject->Cp()->ActivePanel->GetFocus();
				Panel *NewPanel =
						CtrlObject->Cp()->ChangePanel(CtrlObject->Cp()->ActivePanel, FILE_PANEL, TRUE, TRUE);
				NewPanel->SetPluginMode(hNewPlugin, L"",
						CurFocus || !CtrlObject->Cp()->GetAnotherPanel(NewPanel)->IsVisible());

				if (Data && *(const wchar_t *)Data)
					SetDirectory(hNewPlugin, (const wchar_t *)Data, 0);

				/*
					$ 04.04.2001 SVS
					Код закомментирован! Попытка исключить ненужные вызовы в CallPlugin()
					Если что-то не так - раскомментировать!!!
				*/
				// NewPanel->Update(0);
				// NewPanel->Show();
			}

			if (Ret) {
				PluginHandle *handle = (PluginHandle *)hNewPlugin;
				*Ret = hNewPlugin == INVALID_HANDLE_VALUE || handle->hPlugin ? 1 : 0;
			}

			return TRUE;
		}
	}

	return FALSE;
}

Plugin *PluginManager::FindPlugin(DWORD SysID)
{
	if (SysID && SysID != 0xFFFFFFFFUl)		// не допускается 0 и -1
	{
		Plugin *PData;

		for (int I = 0; I < PluginsCount; I++) {
			PData = PluginsData[I];

			if (PData->GetSysID() == SysID)
				return PData;
		}
	}

	return nullptr;
}

HANDLE PluginManager::OpenPlugin(Plugin *pPlugin, int OpenFrom, INT_PTR Item)
{
	Flags.Set(PSIF_ENTERTOOPENPLUGIN);
	HANDLE hPlugin = pPlugin->OpenPlugin(OpenFrom, Item);
	Flags.Clear(PSIF_ENTERTOOPENPLUGIN);

	if (hPlugin != INVALID_HANDLE_VALUE) {
		PluginHandle *handle = new PluginHandle;
		handle->hPlugin = hPlugin;
		handle->pPlugin = pPlugin;
		return (HANDLE)handle;
	}

	return hPlugin;
}

void PluginManager::GetCustomData(FileListItem *ListItem)
{
	FARString FilePath(NTPath(ListItem->strName).Get());

	for (int i = 0; i < PluginsCount; i++) {
		Plugin *pPlugin = PluginsData[i];

		wchar_t *CustomData = nullptr;

		if (pPlugin->HasGetCustomData() && pPlugin->GetCustomData(FilePath.CPtr(), &CustomData)) {
			if (!ListItem->strCustomData.IsEmpty())
				ListItem->strCustomData+= L" ";
			ListItem->strCustomData+= CustomData;

			if (pPlugin->HasFreeCustomData())
				pPlugin->FreeCustomData(CustomData);
		}
	}
}

bool PluginManager::MayExitFar()
{
	bool out = true;
	for (int i = 0; i < PluginsCount; i++) {
		Plugin *pPlugin = PluginsData[i];

		if (pPlugin->HasMayExitFAR() && !pPlugin->MayExitFAR()) {
			out = false;
		}
	}

	return out;
}

static void OnBackgroundTasksChangedSynched()
{
	if (FrameManager)
		FrameManager->RefreshFrame();
}

void PluginManager::BackgroundTaskStarted(const wchar_t *Info)
{
	{
		std::lock_guard<std::mutex> lock(BgTasks);
		auto ir = BgTasks.emplace(Info, 0);
		ir.first->second++;
		fprintf(stderr, "PluginManager::BackgroundTaskStarted('%ls') - count=%d\n", Info, ir.first->second);
	}

	InterThreadCallAsync(std::bind(OnBackgroundTasksChangedSynched));
}

void PluginManager::BackgroundTaskFinished(const wchar_t *Info)
{
	{
		std::lock_guard<std::mutex> lock(BgTasks);
		auto it = BgTasks.find(Info);
		if (it == BgTasks.end()) {
			fprintf(stderr, "PluginManager::BackgroundTaskFinished('%ls') - no such task!\n", Info);
			return;
		}

		it->second--;
		fprintf(stderr, "PluginManager::BackgroundTaskFinished('%ls') - count=%d\n", Info, it->second);
		if (it->second == 0)
			BgTasks.erase(it);
	}

	InterThreadCallAsync(std::bind(OnBackgroundTasksChangedSynched));
}

bool PluginManager::HasBackgroundTasks()
{
	std::lock_guard<std::mutex> lock(BgTasks);
	return !BgTasks.empty();
}

std::map<std::wstring, unsigned int> PluginManager::BackgroundTasks()
{
	std::lock_guard<std::mutex> lock(BgTasks);
	return BgTasks;
}
