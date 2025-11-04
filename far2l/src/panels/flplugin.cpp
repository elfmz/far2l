/*
flplugin.cpp

Файловая панель - работа с плагинами
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

#include "lang.hpp"
#include "filelist.hpp"
#include "filepanels.hpp"
#include "history.hpp"
#include "ctrlobj.hpp"
#include "syslog.hpp"
#include "message.hpp"
#include "config.hpp"
#include "delete.hpp"
#include "datetime.hpp"
#include "dirmix.hpp"
#include "pathmix.hpp"
#include "panelmix.hpp"
#include "mix.hpp"
#include "ChunkedData.hpp"

/*
	В стеке ФАРова панель не хранится - только плагиновые!
*/

void FileList::PushPlugin(HANDLE hPlugin, const wchar_t *HostFile)
{
	PluginsListItem *stItem = new PluginsListItem;
	stItem->hPlugin = hPlugin;
	stItem->strHostFile = HostFile;
	stItem->strPrevOriginalCurDir = strOriginalCurDir;
	strOriginalCurDir = strCurDir;
	stItem->Modified = FALSE;
	stItem->PrevViewMode = ViewMode;
	stItem->PrevSortMode = SortMode;
	stItem->PrevSortOrder = SortOrder;
	stItem->PrevNumericSort = NumericSort;
	stItem->PrevCaseSensitiveSort = CaseSensitiveSort;
	stItem->PrevViewSettings = ViewSettings;
	stItem->PrevDirectoriesFirst = DirectoriesFirst;
	PluginsList.Push(&stItem);
}

int FileList::PopPlugin(int EnableRestoreViewMode)
{
	OpenPluginInfo Info{};

	if (PluginsList.Empty()) {
		PanelMode = NORMAL_PANEL;
		return FALSE;
	}

	// указатель на плагин, с которого уходим
	PluginsListItem *PStack = *PluginsList.Last();

	// закрываем текущий плагин.
	PluginsList.Delete(PluginsList.Last());
	CtrlObject->Plugins.ClosePlugin(hPlugin);

	if (!PluginsList.Empty()) {
		hPlugin = (*PluginsList.Last())->hPlugin;
		strOriginalCurDir = PStack->strPrevOriginalCurDir;

		if (EnableRestoreViewMode) {
			SetViewMode(PStack->PrevViewMode);
			SortMode = PStack->PrevSortMode;
			NumericSort = PStack->PrevNumericSort;
			CaseSensitiveSort = PStack->PrevCaseSensitiveSort;
			SortOrder = PStack->PrevSortOrder;
			DirectoriesFirst = PStack->PrevDirectoriesFirst;
		}

		if (PStack->Modified) {
			PluginPanelItem PanelItem{};
			FARString strSaveDir;
			apiGetCurrentDirectory(strSaveDir);

			if (FileNameToPluginItem(PStack->strHostFile, &PanelItem)) {
				CtrlObject->Plugins.PutFiles(hPlugin, &PanelItem, 1, FALSE, 0);
			} else {
				PanelItem.FindData.lpwszFileName = wcsdup(PointToName(PStack->strHostFile));
				CtrlObject->Plugins.DeleteFiles(hPlugin, &PanelItem, 1, 0);
				free(PanelItem.FindData.lpwszFileName);
			}

			FarChDir(strSaveDir);
		}

		CtrlObject->Plugins.GetOpenPluginInfo(hPlugin, &Info);

		if (!(Info.Flags & OPIF_REALNAMES)) {
			DeleteFileWithFolder(PStack->strHostFile);	// удаление файла от предыдущего плагина
		}
	} else {
		hPlugin = INVALID_HANDLE_VALUE;
		PanelMode = NORMAL_PANEL;

		if (EnableRestoreViewMode) {
			SetViewMode(PStack->PrevViewMode);
			SortMode = PStack->PrevSortMode;
			NumericSort = PStack->PrevNumericSort;
			CaseSensitiveSort = PStack->PrevCaseSensitiveSort;
			SortOrder = PStack->PrevSortOrder;
			DirectoriesFirst = PStack->PrevDirectoriesFirst;
		}
	}

	delete PStack;

	if (EnableRestoreViewMode)
		CtrlObject->Cp()->RedrawKeyBar();

	return TRUE;
}

int FileList::FileNameToPluginItem(const wchar_t *Name, PluginPanelItem *pi)
{
	FARString strTempDir = Name;

	if (!CutToSlash(strTempDir, true))
		return FALSE;

	FarChDir(strTempDir);
	memset(pi, 0, sizeof(*pi));
	FAR_FIND_DATA_EX fdata;

	if (apiGetFindDataEx(Name, fdata)) {
		apiFindDataExToData(&fdata, &pi->FindData);
		return TRUE;
	}

	return FALSE;
}

void FileList::FileListToPluginItem(FileListItem *fi, PluginPanelItem *pi)
{
	pi->FindData.lpwszFileName = wcsdup(fi->strName);
	pi->FindData.nFileSize = fi->FileSize;
	pi->FindData.nPhysicalSize = fi->PhysicalSize;
	pi->FindData.dwFileAttributes = fi->FileAttr;
	pi->FindData.ftLastWriteTime = fi->WriteTime;
	pi->FindData.ftCreationTime = fi->CreationTime;
	pi->FindData.ftLastAccessTime = fi->AccessTime;
	pi->NumberOfLinks = fi->NumberOfLinks;
	pi->Flags = fi->UserFlags;

	if (fi->Selected)
		pi->Flags|= PPIF_SELECTED;

	pi->CustomColumnData = fi->CustomColumnData;
	pi->CustomColumnNumber = fi->CustomColumnNumber;
	pi->Description = fi->DizText;	// BUGBUG???

	if (fi->UserData && (fi->UserFlags & PPIF_USERDATA)) {
		DWORD Size = *(DWORD *)fi->UserData;
		pi->UserData = (DWORD_PTR)malloc(Size);
		memcpy((void *)pi->UserData, (void *)fi->UserData, Size);
	} else
		pi->UserData = fi->UserData;

	pi->CRC32 = fi->CRC32;
	pi->Reserved[0] = pi->Reserved[1] = 0;
	pi->Owner = fi->strOwner.IsEmpty() ? nullptr : (wchar_t *)fi->strOwner.CPtr();
	pi->Group = fi->strGroup.IsEmpty() ? nullptr : (wchar_t *)fi->strGroup.CPtr();
}

void FileList::FreePluginPanelItem(PluginPanelItem *pi)
{
	apiFreeFindData(&pi->FindData);

	if (pi->UserData && (pi->Flags & PPIF_USERDATA))
		free((void *)pi->UserData);
}

size_t FileList::FileListToPluginItem2(FileListItem *fi, PluginPanelItem *pi)
{
	if (pi) {	// first setup trivial stuff
		pi->FindData.nFileSize = fi->FileSize;
		pi->FindData.nPhysicalSize = fi->PhysicalSize;
		pi->FindData.dwFileAttributes = fi->FileAttr;
		pi->FindData.dwUnixMode = fi->FileMode;
		pi->FindData.ftLastWriteTime = fi->WriteTime;
		pi->FindData.ftCreationTime = fi->CreationTime;
		pi->FindData.ftLastAccessTime = fi->AccessTime;
		pi->NumberOfLinks = fi->NumberOfLinks;
		pi->Flags = fi->Selected ? fi->UserFlags | PPIF_SELECTED : fi->UserFlags;
		pi->CustomColumnNumber = fi->CustomColumnNumber;

		// following may be changed later to non-NULL
		pi->CustomColumnData = nullptr;
		pi->Description = nullptr;
		pi->Owner = nullptr;
		pi->Group = nullptr;
	}

	ChunkedData data(pi);
	data.Inflate(sizeof(*pi));

	// append CustomColumnData prior wchar-s as sizeof(wchar_t *) >= sizeof(wchar_t)
	// so its more alignment/space efficient to put it at beginning
	if (fi->CustomColumnNumber) {
		data.Align(sizeof(wchar_t *));
		data.Inflate(fi->CustomColumnNumber * sizeof(wchar_t *));
		if (pi) {
			pi->CustomColumnData = (wchar_t **)data.Recent();
		}
	}

	data.Align(sizeof(wchar_t));
	for (int i = 0; i < fi->CustomColumnNumber; ++i) {
		data.Append(fi->CustomColumnData[i]);
		if (pi) {
			((const wchar_t **)(pi->CustomColumnData))[i] = (const wchar_t *)data.Recent();
		}
	}

	data.Append(fi->strName);
	if (pi) {
		pi->FindData.lpwszFileName = (wchar_t *)data.Recent();
	}

	if (fi->DizText) {
		data.Append(fi->DizText);
		if (pi) {
			pi->Description = (const wchar_t *)data.Recent();
		}
	}

	if (!fi->strOwner.IsEmpty()) {
		data.Append(fi->strOwner);
		if (pi) {
			pi->Owner = (const wchar_t *)data.Recent();
		}
	}

	if (!fi->strGroup.IsEmpty()) {
		data.Append(fi->strGroup);
		if (pi) {
			pi->Group = (const wchar_t *)data.Recent();
		}
	}

	// copy user data at the end to avoid alignment gaps after
	if (fi->UserData && (fi->UserFlags & PPIF_USERDATA) != 0) {
		const DWORD ud_size = *(const DWORD *)fi->UserData;
		data.Align((ud_size >= 8) ? 8 : 4);
		data.Append((const void *)fi->UserData, ud_size);
		if (pi) {
			pi->UserData = (DWORD_PTR)data.Recent();
		}
	} else if (pi) {
		pi->UserData = fi->UserData;
	}

	return data.Length();
}

void FileList::PluginToFileListItem(PluginPanelItem *pi, FileListItem *fi)
{
	fi->strName = pi->FindData.lpwszFileName;
	fi->strOwner = pi->Owner;
	fi->strGroup = pi->Group;

	if (pi->Description) {
		fi->DizText = new wchar_t[StrLength(pi->Description) + 1];
		wcscpy(fi->DizText, pi->Description);
		fi->DeleteDiz = true;
	} else
		fi->DizText = nullptr;

	fi->FileSize = pi->FindData.nFileSize;
	fi->PhysicalSize = pi->FindData.nPhysicalSize;
	fi->FileAttr = pi->FindData.dwFileAttributes;
	fi->FileMode = pi->FindData.dwUnixMode;
	fi->WriteTime = pi->FindData.ftLastWriteTime;
	fi->CreationTime = pi->FindData.ftCreationTime;
	fi->AccessTime = pi->FindData.ftLastAccessTime;
	fi->ChangeTime.dwHighDateTime = 0;
	fi->ChangeTime.dwLowDateTime = 0;
	fi->NumberOfLinks = pi->NumberOfLinks;
	fi->UserFlags = pi->Flags;

	if (pi->UserData && (pi->Flags & PPIF_USERDATA)) {
		DWORD Size = *(DWORD *)pi->UserData;
		fi->UserData = (DWORD_PTR)malloc(Size);
		memcpy((void *)fi->UserData, (void *)pi->UserData, Size);
	} else
		fi->UserData = pi->UserData;

	if (pi->CustomColumnNumber > 0) {
		fi->CustomColumnData = new wchar_t *[pi->CustomColumnNumber];

		for (int I = 0; I < pi->CustomColumnNumber; I++)
			if (pi->CustomColumnData && pi->CustomColumnData[I]) {
				fi->CustomColumnData[I] = new wchar_t[StrLength(pi->CustomColumnData[I]) + 1];
				wcscpy(fi->CustomColumnData[I], pi->CustomColumnData[I]);
			} else {
				fi->CustomColumnData[I] = new wchar_t[1];
				fi->CustomColumnData[I][0] = 0;
			}
	}

	fi->CustomColumnNumber = pi->CustomColumnNumber;
	fi->CRC32 = pi->CRC32;
}

HANDLE FileList::OpenPluginForFile(const wchar_t *FileName, DWORD FileAttr, OPENFILEPLUGINTYPE Type)
{
	HANDLE Result = INVALID_HANDLE_VALUE;
	if (FileName && *FileName && !(FileAttr & FILE_ATTRIBUTE_DIRECTORY)) {
		SetCurPath();
		_ALGO(SysLog(L"close AnotherPanel file"));
		CtrlObject->Cp()->GetAnotherPanel(this)->CloseFile();
		_ALGO(SysLog(L"call Plugins.OpenFilePlugin {"));
		Result = CtrlObject->Plugins.OpenFilePlugin(FileName, 0, Type);
		_ALGO(SysLog(L"}"));
	}
	return Result;
}

void FileList::CreatePluginItemList(PluginPanelItemVec &ItemList)	// Add but not Create
{
	if (ListData.IsEmpty())
		return;

	const auto SaveSelPosition = GetSelPosition;
	const auto OldLastSelPosition = LastSelPosition;

	try {
		ItemList.ReserveExtra(SelFileCount + 1);

		FARString strSelName;
		DWORD FileAttr = 0;
		GetSelNameCompat(nullptr, FileAttr);

		while (GetSelNameCompat(&strSelName, FileAttr)) {
			if ((!(FileAttr & FILE_ATTRIBUTE_DIRECTORY) || !TestParentFolderName(strSelName))
					&& LastSelPosition >= 0 && LastSelPosition < ListData.Count()) {
				ItemList.Add(ListData[LastSelPosition]);
			}
		}

		if (ItemList.IsEmpty() && !ListData.IsEmpty() && TestParentFolderName(ListData[0]->strName))	// это про ".."
		{
			ItemList.Add(ListData[0]);
		}
	} catch (std::exception &e) {
		fprintf(stderr, "%s: %s\n", __FUNCTION__, e.what());
	}

	LastSelPosition = OldLastSelPosition;
	GetSelPosition = SaveSelPosition;
}

void FileList::PluginDelete()
{
	_ALGO(CleverSysLog clv(L"FileList::PluginDelete()"));
	SaveSelection();
	PluginPanelItemVec ItemList;
	CreatePluginItemList(ItemList);

	if (ItemList.IsEmpty())
		return;

	if (CtrlObject->Plugins.DeleteFiles(hPlugin, ItemList.Data(), ItemList.Count(), 0)) {
		SetPluginModified();
		PutDizToPlugin(this, ItemList.Data(), ItemList.Count(), TRUE, FALSE, nullptr, &Diz);
	}
	ItemList.Clear();

	Update(UPDATE_KEEP_SELECTION);
	Redraw();
	Panel *AnotherPanel = CtrlObject->Cp()->GetAnotherPanel(this);
	AnotherPanel->Update(UPDATE_KEEP_SELECTION | UPDATE_SECONDARY);
	AnotherPanel->Redraw();
}

void FileList::PutDizToPlugin(FileList *DestPanel, PluginPanelItem *ItemList, int ItemNumber, int Delete,
		int Move, DizList *SrcDiz, DizList *DestDiz)
{
	_ALGO(CleverSysLog clv(L"FileList::PutDizToPlugin()"));
	OpenPluginInfo Info;
	CtrlObject->Plugins.GetOpenPluginInfo(DestPanel->hPlugin, &Info);

	if (DestPanel->strPluginDizName.IsEmpty() && Info.DescrFilesNumber > 0)
		DestPanel->strPluginDizName = Info.DescrFiles[0];

	if (((Opt.Diz.UpdateMode == DIZ_UPDATE_IF_DISPLAYED && IsDizDisplayed())
				|| Opt.Diz.UpdateMode == DIZ_UPDATE_ALWAYS)
			&& !DestPanel->strPluginDizName.IsEmpty()
			&& (!Info.HostFile || !*Info.HostFile || DestPanel->GetModalMode()
					|| apiGetFileAttributes(Info.HostFile) != INVALID_FILE_ATTRIBUTES)) {
		CtrlObject->Cp()->LeftPanel->ReadDiz();
		CtrlObject->Cp()->RightPanel->ReadDiz();

		if (DestPanel->GetModalMode())
			DestPanel->ReadDiz();

		int DizPresent = FALSE;

		for (int I = 0; I < ItemNumber; I++)
			if (ItemList[I].Flags & PPIF_PROCESSDESCR) {
				FARString strName = ItemList[I].FindData.lpwszFileName;
				int Code;

				if (Delete)
					Code = DestDiz->DeleteDiz(strName);
				else {
					Code = SrcDiz->CopyDiz(strName, strName, DestDiz);

					if (Code && Move)
						SrcDiz->DeleteDiz(strName);
				}

				if (Code)
					DizPresent = TRUE;
			}

		if (DizPresent) {
			FARString strTempDir;

			if (FarMkTempEx(strTempDir) && apiCreateDirectory(strTempDir, nullptr)) {
				FARString strSaveDir;
				apiGetCurrentDirectory(strSaveDir);
				FARString strDizName = strTempDir + WGOOD_SLASH + DestPanel->strPluginDizName;
				DestDiz->Flush(L"", strDizName);

				if (Move)
					SrcDiz->Flush(L"");

				PluginPanelItem PanelItem;

				if (FileNameToPluginItem(strDizName, &PanelItem))
					CtrlObject->Plugins.PutFiles(DestPanel->hPlugin, &PanelItem, 1, FALSE,
							OPM_SILENT | OPM_DESCR);
				else if (Delete) {
					PluginPanelItem pi{};
					pi.FindData.lpwszFileName = wcsdup(DestPanel->strPluginDizName);
					CtrlObject->Plugins.DeleteFiles(DestPanel->hPlugin, &pi, 1, OPM_SILENT);
					free(pi.FindData.lpwszFileName);
				}

				FarChDir(strSaveDir);
				DeleteFileWithFolder(strDizName);
			}
		}
	}
}

void FileList::PluginGetFiles(const wchar_t **DestPath, int Move)
{
	_ALGO(CleverSysLog clv(L"FileList::PluginGetFiles()"));
	SaveSelection();
	PluginPanelItemVec ItemList;
	CreatePluginItemList(ItemList);
	if (ItemList.IsEmpty())
		return;

	const int GetCode =
			CtrlObject->Plugins.GetFiles(hPlugin, ItemList.Data(), ItemList.Count(), Move, DestPath, 0);

	if ((Opt.Diz.UpdateMode == DIZ_UPDATE_IF_DISPLAYED && IsDizDisplayed())
			|| Opt.Diz.UpdateMode == DIZ_UPDATE_ALWAYS) {
		DizList DestDiz;
		bool DizFound = false;
		for (auto &Item : ItemList) {
			if (Item.Flags & PPIF_PROCESSDESCR) {
				if (!DizFound) {
					CtrlObject->Cp()->LeftPanel->ReadDiz();
					CtrlObject->Cp()->RightPanel->ReadDiz();
					DestDiz.Read(*DestPath);
					DizFound = true;
				}

				FARString strName = Item.FindData.lpwszFileName;
				CopyDiz(strName, strName, &DestDiz);
			}
		}

		DestDiz.Flush(*DestPath);
	}

	if (GetCode == 1) {
		if (!ReturnCurrentFile)
			ClearSelection();

		if (Move) {
			SetPluginModified();
			PutDizToPlugin(this, ItemList.Data(), ItemList.Count(), TRUE, FALSE, nullptr, &Diz);
		}
	} else if (!ReturnCurrentFile)
		PluginClearSelection(ItemList.Data(), ItemList.Count());

	ItemList.Clear();
	Update(UPDATE_KEEP_SELECTION);
	Redraw();
	Panel *AnotherPanel = CtrlObject->Cp()->GetAnotherPanel(this);
	AnotherPanel->Update(UPDATE_KEEP_SELECTION | UPDATE_SECONDARY);
	AnotherPanel->Redraw();
}

void FileList::PluginToPluginFiles(int Move)
{
	_ALGO(CleverSysLog clv(L"FileList::PluginToPluginFiles()"));
	Panel *AnotherPanel = CtrlObject->Cp()->GetAnotherPanel(this);
	FARString strTempDir;

	if (AnotherPanel->GetMode() != PLUGIN_PANEL)
		return;

	FileList *AnotherFilePanel = (FileList *)AnotherPanel;

	if (!FarMkTempEx(strTempDir))
		return;

	SaveSelection();
	apiCreateDirectory(strTempDir, nullptr);

	PluginPanelItemVec ItemList;
	CreatePluginItemList(ItemList);
	if (ItemList.IsEmpty())
		return;

	const wchar_t *lpwszTempDir = strTempDir;
	int PutCode = CtrlObject->Plugins.GetFiles(hPlugin, ItemList.Data(), ItemList.Count(), FALSE,
			&lpwszTempDir, OPM_SILENT);
	strTempDir = lpwszTempDir;

	if (PutCode == 1 || PutCode == 2) {
		FARString strSaveDir;
		apiGetCurrentDirectory(strSaveDir);
		FarChDir(strTempDir);
		PutCode = CtrlObject->Plugins.PutFiles(AnotherFilePanel->hPlugin, ItemList.Data(), ItemList.Count(),
				FALSE, 0);

		if (PutCode == 1 || PutCode == 2) {
			if (!ReturnCurrentFile)
				ClearSelection();

			AnotherPanel->SetPluginModified();
			PutDizToPlugin(AnotherFilePanel, ItemList.Data(), ItemList.Count(), FALSE, FALSE, &Diz,
					&AnotherFilePanel->Diz);

			if (Move)
				if (CtrlObject->Plugins.DeleteFiles(hPlugin, ItemList.Data(), ItemList.Count(), OPM_SILENT)) {
					SetPluginModified();
					PutDizToPlugin(this, ItemList.Data(), ItemList.Count(), TRUE, FALSE, nullptr, &Diz);
				}
		} else if (!ReturnCurrentFile)
			PluginClearSelection(ItemList.Data(), ItemList.Count());

		FarChDir(strSaveDir);
	}

	DeleteDirTree(strTempDir);
	ItemList.Clear();
	Update(UPDATE_KEEP_SELECTION);
	Redraw();

	if (PanelMode == PLUGIN_PANEL)
		AnotherPanel->Update(UPDATE_KEEP_SELECTION | UPDATE_SECONDARY);
	else
		AnotherPanel->Update(UPDATE_KEEP_SELECTION);

	AnotherPanel->Redraw();
}

void FileList::PluginHostGetFiles()
{
	Panel *AnotherPanel = CtrlObject->Cp()->GetAnotherPanel(this);
	FARString strDestPath;
	FARString strSelName;
	DWORD FileAttr;
	SaveSelection();
	GetSelNameCompat(nullptr, FileAttr);

	if (!GetSelNameCompat(&strSelName, FileAttr))
		return;

	AnotherPanel->GetCurDir(strDestPath);

	if (((!AnotherPanel->IsVisible() || AnotherPanel->GetType() != FILE_PANEL) && !SelFileCount)
			|| strDestPath.IsEmpty()) {
		strDestPath = PointToName(strSelName);
		// SVS: А зачем здесь велся поиск точки с начала?
		size_t pos;

		if (strDestPath.RPos(pos, L'.'))
			strDestPath.Truncate(pos);
	}

	int OpMode = OPM_TOPLEVEL, ExitLoop = FALSE;
	GetSelNameCompat(nullptr, FileAttr);

	while (!ExitLoop && GetSelNameCompat(&strSelName, FileAttr)) {
		HANDLE hCurPlugin;

		if ((hCurPlugin = OpenPluginForFile(strSelName, FileAttr, OFP_EXTRACT)) != INVALID_HANDLE_VALUE
				&& hCurPlugin != (HANDLE)-2) {
			PluginPanelItem *ItemList;
			int ItemNumber;
			_ALGO(SysLog(L"call Plugins.GetFindData()"));

			if (CtrlObject->Plugins.GetFindData(hCurPlugin, &ItemList, &ItemNumber, 0)) {
				_ALGO(SysLog(L"call Plugins.GetFiles()"));
				const wchar_t *lpwszDestPath = strDestPath;
				ExitLoop = CtrlObject->Plugins.GetFiles(hCurPlugin, ItemList, ItemNumber, FALSE,
								&lpwszDestPath, OpMode)
						!= 1;
				strDestPath = lpwszDestPath;

				if (!ExitLoop) {
					_ALGO(SysLog(L"call ClearLastGetSelection()"));
					ClearLastGetSelection();
				}

				_ALGO(SysLog(L"call Plugins.FreeFindData()"));
				CtrlObject->Plugins.FreeFindData(hCurPlugin, ItemList, ItemNumber);
				OpMode|= OPM_SILENT;
			}

			_ALGO(SysLog(L"call Plugins.ClosePlugin"));
			CtrlObject->Plugins.ClosePlugin(hCurPlugin);
		}
	}

	Update(UPDATE_KEEP_SELECTION);
	Redraw();
	AnotherPanel->Update(UPDATE_KEEP_SELECTION | UPDATE_SECONDARY);
	AnotherPanel->Redraw();
}

void FileList::PluginPutFilesToNew()
{
	_ALGO(CleverSysLog clv(L"FileList::PluginPutFilesToNew()"));
	//_ALGO(SysLog(L"FileName='%ls'",(FileName?FileName:"(nullptr)")));
	_ALGO(SysLog(L"call Plugins.OpenFilePlugin(nullptr, 0)"));
	HANDLE hNewPlugin = CtrlObject->Plugins.OpenFilePlugin(nullptr, 0, OFP_CREATE);

	if (hNewPlugin != INVALID_HANDLE_VALUE && hNewPlugin != (HANDLE)-2) {
		_ALGO(SysLog(L"Create: FileList TmpPanel, FileCount=%d", ListData.Count()));
		FileList TmpPanel;
		TmpPanel.SetPluginMode(hNewPlugin, L"");	// SendOnFocus??? true???
		TmpPanel.SetModalMode(TRUE);
		auto PrevFileCount = ListData.Count();
		/*
			$ 12.04.2002 IS
			Если PluginPutFilesToAnother вернула число, отличное от 2, то нужно
			попробовать установить курсор на созданный файл.
		*/
		int rc = PluginPutFilesToAnother(FALSE, &TmpPanel);

		if (rc != 2 && ListData.Count() == PrevFileCount + 1) {
			int LastPos = 0;
			/*
				Место, где вычисляются координаты вновь созданного файла
				Позиционирование происходит на файл с максимальной датой
				создания файла. Посему, если какой-то злобный буратино поимел
				в текущем каталоге файло с датой создания поболее текущей,
				то корректного позиционирования не произойдет!
			*/
			FileListItem *PtrListData, *PtrLastPos = nullptr;

			for (int i = 0; i < ListData.Count(); ++i) {
				PtrListData = ListData[i];
				if ((PtrListData->FileAttr & FILE_ATTRIBUTE_DIRECTORY) == 0) {
					if (!PtrLastPos
							|| FileTimeDifference(&PtrListData->CreationTime, &PtrLastPos->CreationTime)
									> 0) {
						LastPos = i;
						PtrLastPos = PtrListData;
					}
				}
			}

			if (PtrLastPos) {
				CurFile = LastPos;
				Redraw();
			}
		}
	}
}

/*
	$ 12.04.2002 IS
	PluginPutFilesToAnother теперь int - возвращает то, что возвращает
	PutFiles:
		-1 - прервано пользовтелем
		0  - неудача
		1  - удача
		2  - удача, курсор принудительно установлен на файл и заново его
		устанавливать не нужно (см. PluginPutFilesToNew)
*/
int FileList::PluginPutFilesToAnother(int Move, Panel *AnotherPanel)
{
	if (AnotherPanel->GetMode() != PLUGIN_PANEL)
		return 0;

	FileList *AnotherFilePanel = (FileList *)AnotherPanel;
	SaveSelection();
	PluginPanelItemVec ItemList;
	CreatePluginItemList(ItemList);

	if (ItemList.IsEmpty())
		return 0;

	SetCurPath();
	_ALGO(SysLog(L"call Plugins.PutFiles"));
	int PutCode = CtrlObject->Plugins.PutFiles(AnotherFilePanel->hPlugin, ItemList.Data(), ItemList.Count(),
			Move, 0);

	if (PutCode == 1 || PutCode == 2) {
		if (!ReturnCurrentFile) {
			_ALGO(SysLog(L"call ClearSelection()"));
			ClearSelection();
		}

		_ALGO(SysLog(L"call PutDizToPlugin"));
		PutDizToPlugin(AnotherFilePanel, ItemList.Data(), ItemList.Count(), FALSE, Move, &Diz,
				&AnotherFilePanel->Diz);
		AnotherPanel->SetPluginModified();
	} else if (!ReturnCurrentFile)
		PluginClearSelection(ItemList.Data(), ItemList.Count());

	_ALGO(SysLog(L"call DeletePluginItemList"));
	ItemList.Clear();
	Update(UPDATE_KEEP_SELECTION);
	Redraw();

	if (AnotherPanel == CtrlObject->Cp()->GetAnotherPanel(this)) {
		AnotherPanel->Update(UPDATE_KEEP_SELECTION);
		AnotherPanel->Redraw();
	}

	return PutCode;
}

void FileList::GetOpenPluginInfo(OpenPluginInfo *Info)
{
	_ALGO(CleverSysLog clv(L"FileList::GetOpenPluginInfo()"));
	//_ALGO(SysLog(L"FileName='%ls'",(FileName?FileName:"(nullptr)")));
	memset(Info, 0, sizeof(*Info));

	if (PanelMode == PLUGIN_PANEL)
		CtrlObject->Plugins.GetOpenPluginInfo(hPlugin, Info);
}

/*
	Функция для вызова команды "Архивные команды" (Shift-F3)
*/
void FileList::ProcessHostFile()
{
	_ALGO(CleverSysLog clv(L"FileList::ProcessHostFile()"));

	//_ALGO(SysLog(L"FileName='%ls'",(FileName?FileName:"(nullptr)")));
	if (!ListData.IsEmpty() && SetCurPath()) {
		int Done = FALSE;
		SaveSelection();

		if (PanelMode == PLUGIN_PANEL && !(*PluginsList.Last())->strHostFile.IsEmpty()) {
			_ALGO(SysLog(L"call CreatePluginItemList"));
			PluginPanelItemVec ItemList;
			CreatePluginItemList(ItemList);
			_ALGO(SysLog(L"call Plugins.ProcessHostFile"));
			Done = CtrlObject->Plugins.ProcessHostFile(hPlugin, ItemList.Data(), ItemList.Count(), 0);

			if (Done)
				SetPluginModified();
			else {
				if (!ReturnCurrentFile)
					PluginClearSelection(ItemList.Data(), ItemList.Count());

				Redraw();
			}

			_ALGO(SysLog(L"call DeletePluginItemList"));
			ItemList.Clear();

			if (Done)
				ClearSelection();
		} else {
			int SCount = GetRealSelCount();

			if (SCount > 0) {
				for (int I = 0; I < ListData.Count(); ++I) {
					if (ListData[I]->Selected) {
						Done = ProcessOneHostFile(I);

						if (Done == 1)
							Select(ListData[I], 0);
						else if (Done == -1)
							continue;
						else	// Если ЭТО убрать, то... будем жать ESC до потере пулься
							break;
					}
				}

				if (SelectedFirst)
					SortFileList(TRUE);
			} else {
				if ((Done = ProcessOneHostFile(CurFile)) == 1)
					ClearSelection();
			}
		}

		if (Done) {
			Update(UPDATE_KEEP_SELECTION);
			Redraw();
			Panel *AnotherPanel = CtrlObject->Cp()->GetAnotherPanel(this);
			AnotherPanel->Update(UPDATE_KEEP_SELECTION | UPDATE_SECONDARY);
			AnotherPanel->Redraw();
		}
	}
}

/*
	Обработка одного хост-файла.
	Return:
		-1 - Этот файл никаким плагином не поддержан
		0  - Плагин вернул FALSE
		1  - Плагин вернул TRUE
*/
int FileList::ProcessOneHostFile(int Idx)
{
	_ALGO(CleverSysLog clv(L"FileList::ProcessOneHostFile()"));
	int Done = -1;
	_ALGO(SysLog(L"call OpenPluginForFile([Idx=%d] '%ls')", Idx, ListData[Idx]->strName.CPtr()));
	FARString strName = ListData[Idx]->strName;
	HANDLE hNewPlugin = OpenPluginForFile(strName, ListData[Idx]->FileAttr, OFP_COMMANDS);

	if (hNewPlugin != INVALID_HANDLE_VALUE && hNewPlugin != (HANDLE)-2) {
		_ALGO(SysLog(L"call Plugins.GetFindData"));

		PluginPanelItem *ItemList;
		int ItemNumber;
		if (CtrlObject->Plugins.GetFindData(hNewPlugin, &ItemList, &ItemNumber, OPM_TOPLEVEL)) {
			_ALGO(SysLog(L"call Plugins.ProcessHostFile"));
			Done = CtrlObject->Plugins.ProcessHostFile(hNewPlugin, ItemList, ItemNumber, OPM_TOPLEVEL);
			_ALGO(SysLog(L"call Plugins.FreeFindData"));
			CtrlObject->Plugins.FreeFindData(hNewPlugin, ItemList, ItemNumber);
		}

		_ALGO(SysLog(L"call Plugins.ClosePlugin"));
		CtrlObject->Plugins.ClosePlugin(hNewPlugin);
	}

	return Done;
}

void FileList::SetPluginMode(HANDLE hPlugin, const wchar_t *PluginFile, bool SendOnFocus)
{
	if (PanelMode != PLUGIN_PANEL) {
		CtrlObject->FolderHistory->AddToHistory(strCurDir);
	}

	PushPlugin(hPlugin, PluginFile);
	FileList::hPlugin = hPlugin;
	PanelMode = PLUGIN_PANEL;

	if (SendOnFocus)
		SetFocus();

	OpenPluginInfo Info;
	CtrlObject->Plugins.GetOpenPluginInfo(hPlugin, &Info);

	if (Info.StartPanelMode)
		SetViewMode(VIEW_0 + Info.StartPanelMode - L'0');

	CtrlObject->Cp()->RedrawKeyBar();

	if (Info.StartSortMode) {
		SortMode = Info.StartSortMode - (SM_UNSORTED - UNSORTED);
		SortOrder = Info.StartSortOrder ? -1 : 1;
	}

	Panel *AnotherPanel = CtrlObject->Cp()->GetAnotherPanel(this);

	if (AnotherPanel->GetType() != FILE_PANEL) {
		AnotherPanel->Update(UPDATE_KEEP_SELECTION);
		AnotherPanel->Redraw();
	}
}

void FileList::PluginGetPanelInfo(PanelInfo &Info)
{
	CorrectPosition();
	Info.CurrentItem = CurFile;
	Info.TopPanelItem = CurTopFile;
	Info.ItemsNumber = ListData.Count();
	Info.SelectedItemsNumber = Info.ItemsNumber ? GetSelCount() : 0;
}

size_t FileList::PluginGetPanelItem(int ItemNumber, PluginPanelItem *Item)
{
	size_t result = 0;

	if (ItemNumber >= 0 && ItemNumber < ListData.Count()) {
		result = FileListToPluginItem2(ListData[ItemNumber], Item);
	}

	return result;
}

size_t FileList::PluginGetSelectedPanelItem(int ItemNumber, PluginPanelItem *Item)
{
	size_t result = 0;

	if (ItemNumber >= 0 && ItemNumber < ListData.Count()) {
		if (ItemNumber == CacheSelIndex) {
			result = FileListToPluginItem2(ListData[CacheSelPos], Item);
		} else {
			if (ItemNumber < CacheSelIndex)
				CacheSelIndex = -1;

			int CurSel = CacheSelIndex, StartValue = CacheSelIndex >= 0 ? CacheSelPos + 1 : 0;

			for (int i = StartValue; i < ListData.Count(); i++) {
				if (ListData[i]->Selected)
					CurSel++;

				if (CurSel == ItemNumber) {
					result = FileListToPluginItem2(ListData[i], Item);
					CacheSelIndex = ItemNumber;
					CacheSelPos = i;
					break;
				}
			}

			if (CurSel == -1 && !ItemNumber) {
				result = FileListToPluginItem2(ListData[CurFile], Item);
				CacheSelIndex = -1;
			}
		}
	}

	return result;
}

void FileList::PluginGetColumnTypesAndWidths(FARString &strColumnTypes, FARString &strColumnWidths)
{
	ViewSettingsToText(ViewSettings.ColumnType, ViewSettings.ColumnWidth, ViewSettings.ColumnWidthType,
			ViewSettings.ColumnCount, strColumnTypes, strColumnWidths);
}

void FileList::PluginBeginSelection()
{
	SaveSelection();
}

void FileList::PluginSetSelection(int ItemNumber, bool Selection)
{
	Select(ListData[ItemNumber], Selection);
}

void FileList::PluginClearSelection(int SelectedItemNumber)
{
	if (SelectedItemNumber >= 0 && SelectedItemNumber < ListData.Count()) {
		if (SelectedItemNumber <= CacheSelClearIndex) {
			CacheSelClearIndex = -1;
		}

		int CurSel = CacheSelClearIndex, StartValue = CacheSelClearIndex >= 0 ? CacheSelClearPos + 1 : 0;

		for (int i = StartValue; i < ListData.Count(); i++) {
			if (ListData[i]->Selected) {
				CurSel++;
			}

			if (CurSel == SelectedItemNumber) {
				Select(ListData[i], FALSE);
				CacheSelClearIndex = SelectedItemNumber;
				CacheSelClearPos = i;
				break;
			}
		}
	}
}

void FileList::PluginEndSelection()
{
	if (SelectedFirst) {
		SortFileList(TRUE);
	}
}

void FileList::ProcessPluginCommand()
{
	_ALGO(CleverSysLog clv(L"FileList::ProcessPluginCommand"));
	_ALGO(SysLog(L"PanelMode=%ls", (PanelMode == PLUGIN_PANEL ? "PLUGIN_PANEL" : "NORMAL_PANEL")));
	int Command = PluginCommand;
	PluginCommand = -1;

	if (PanelMode == PLUGIN_PANEL)
		switch (Command) {
			case FCTL_CLOSEPLUGIN:
				_ALGO(SysLog(L"Command=FCTL_CLOSEPLUGIN"));
				SetCurDir(strPluginParam, TRUE);

				if (strPluginParam.IsEmpty())
					Update(UPDATE_KEEP_SELECTION);

				Redraw();
				break;
		}
}

void FileList::SetPluginModified()
{
	if (PluginsList.Last()) {
		(*PluginsList.Last())->Modified = TRUE;
	}
}

HANDLE FileList::GetPluginHandle()
{
	return (hPlugin);
}

int FileList::ProcessPluginEvent(int Event, void *Param)
{
	if (PanelMode == PLUGIN_PANEL)
		return (CtrlObject->Plugins.ProcessEvent(hPlugin, Event, Param));

	return FALSE;
}

void FileList::PluginClearSelection(PluginPanelItem *ItemList, int ItemNumber)
{
	SaveSelection();
	int FileNumber = 0, PluginNumber = 0;

	while (PluginNumber < ItemNumber) {
		PluginPanelItem *CurPluginPtr = ItemList + PluginNumber;

		if (!(CurPluginPtr->Flags & PPIF_SELECTED)) {
			while (StrCmp(CurPluginPtr->FindData.lpwszFileName, ListData[FileNumber]->strName))
				if (++FileNumber >= ListData.Count())
					return;

			Select(ListData[FileNumber++], 0);
		}

		PluginNumber++;
	}
}
