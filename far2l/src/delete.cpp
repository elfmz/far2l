/*
delete.cpp

Удаление файлов
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
#include "panel.hpp"
#include "chgprior.hpp"
#include "filepanels.hpp"
#include "scantree.hpp"
#include "treelist.hpp"
#include "savescr.hpp"
#include "ctrlobj.hpp"
#include "filelist.hpp"
#include "manager.hpp"
#include "constitle.hpp"
#include "TPreRedrawFunc.hpp"
#include "cddrv.hpp"
#include "interf.hpp"
#include "keyboard.hpp"
#include "message.hpp"
#include "config.hpp"
#include "pathmix.hpp"
#include "dirmix.hpp"
#include "strmix.hpp"
#include "flink.hpp"
#include "panelmix.hpp"
#include "mix.hpp"
#include "dirinfo.hpp"
#include "wakeful.hpp"
#include "execute.hpp"

#include <RandomString.h>
#include <errno.h>

enum DeletionResult
{
	DELETE_SUCCESS,
	DELETE_YES,
	DELETE_SKIP,
	DELETE_CANCEL,
	DELETE_NO_RECYCLE_BIN
};

static void ShellDeleteMsg(const wchar_t *Name, bool Wipe, int Percent);
static DeletionResult ShellRemoveFile(const wchar_t *Name, bool Wipe, int Opt_DeleteToRecycleBin);
static DeletionResult ERemoveDirectory(const wchar_t *Name, bool Wipe);
static DeletionResult RemoveToRecycleBin(const wchar_t *Name);
static bool WipeFile(const wchar_t *Name);
static bool WipeDirectory(const wchar_t *Name);
static void PR_ShellDeleteMsg();

static int ReadOnlyDeleteMode, SkipMode, SkipWipeMode, SkipFoldersMode, SkipRecycleMode;
static bool DeleteAllFolders;
ULONG ProcessedItems;

struct AskDeleteReadOnly
{
	AskDeleteReadOnly(const wchar_t *Name, bool Wipe);

	~AskDeleteReadOnly()
	{
		if (_ump)
			_ump->Unmake();
	}

	inline DeletionResult Choice() const { return _r; }

private:
	IUnmakeWritablePtr _ump;
	DeletionResult _r;
};

class ShellDeleteMsgState
{
	clock_t _last_redraw{0};
	ConsoleTitle _delete_title{Msg::DeletingTitle};

public:
	bool Update(const wchar_t *name, bool wipe, ULONG processed = (ULONG)-1, ULONG total = (ULONG)-1)
	{
		const clock_t now = GetProcessUptimeMSec();

		if (now - _last_redraw > RedrawTimeout) {
			_last_redraw = now;

			const int percent = (Opt.DelOpt.DelShowTotal && total != (ULONG)-1)
					? (total ? (processed * 100 / total) : 0)
					: -1;

			if (percent != -1)
				_delete_title.Set(L"{%d%%} %ls", percent,
						(wipe ? Msg::DeleteWipeTitle : Msg::DeleteTitle).CPtr());

			ShellDeleteMsg(name, wipe, percent);

			if (CheckForEscSilent() && ConfirmAbortOp())
				return false;
		}

		return true;
	}
};

static FARString PanelItemFullName(Panel *SrcPanel, const FARString &strSelName)
{
	if (IsAbsolutePath(strSelName))
		return strSelName;

	FARString strSelFullName;
	SrcPanel->GetCurDir(strSelFullName);
	AddEndSlash(strSelFullName);
	strSelFullName+= strSelName;
	return strSelFullName;
}

static DeletionResult ShellConfirmDirectoryDeletion(const FARString &strFullName, bool Wipe)
{
	if (DeleteAllFolders)
		return DELETE_YES;

	if (TestFolder(strFullName) != TSTFLD_NOTEMPTY)
		return DELETE_YES;

	const int MsgCode = Message(MSG_WARNING, 4, (Wipe ? Msg::WipeFolderTitle : Msg::DeleteFolderTitle),
			(Wipe ? Msg::WipeFolderConfirm : Msg::DeleteFolderConfirm), strFullName,
			(Wipe ? Msg::DeleteFileWipe : Msg::DeleteFileDelete), Msg::DeleteFileAll, Msg::DeleteFileSkip,
			Msg::DeleteFileCancel);

	if (MsgCode < 0 || MsgCode == 3)
		return DELETE_CANCEL;

	if (MsgCode == 2)
		return DELETE_SKIP;

	if (MsgCode == 1)
		DeleteAllFolders = true;

	return DELETE_YES;
}

static DeletionResult ShellDeleteDirectory(int ItemsCount, bool UpdateDiz, Panel *SrcPanel,
		FARString strSelName, DWORD FileAttr, bool Wipe, int Opt_DeleteToRecycleBin)
{
	// для symlink`а не нужно подтверждение
	DeletionResult DR;
	if (!(FileAttr & FILE_ATTRIBUTE_REPARSE_POINT)) {
		FARString strFullName;
		ConvertNameToFull(strSelName, strFullName);
		DR = ShellConfirmDirectoryDeletion(strFullName, Wipe);
		if (DR != DELETE_YES)
			return DR;
	}

	bool DirSymLink = (FileAttr & FILE_ATTRIBUTE_DIRECTORY && FileAttr & FILE_ATTRIBUTE_REPARSE_POINT);

	if (!DirSymLink && (!Opt_DeleteToRecycleBin || Wipe)) {
		ShellDeleteMsgState SDMS;
		ScanTree ScTree(TRUE, TRUE, FALSE);
		FARString strSelFullName = PanelItemFullName(SrcPanel, strSelName);

		ScTree.SetFindPath(strSelFullName, L"*", 0);
		FAR_FIND_DATA_EX FindData;
		FARString strFullName;
		while (ScTree.GetNextName(&FindData, strFullName)) {
			if (!SDMS.Update(strFullName, Wipe, ProcessedItems, ItemsCount))
				return DELETE_CANCEL;

			if (FindData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) {
				if (!ScTree.IsDirSearchDone()
						&& (FindData.dwFileAttributes & FILE_ATTRIBUTE_REPARSE_POINT) == 0) {
					DR = ShellConfirmDirectoryDeletion(strFullName, Wipe);
					if (DR == DELETE_SKIP)
						ScTree.SkipDir();
				} else {
					if (FindData.dwFileAttributes & FILE_ATTRIBUTE_READONLY)
						apiMakeWritable(strFullName);

					DR = ERemoveDirectory(strFullName, Wipe);

					if (DR == DELETE_SUCCESS)
						TreeList::DelTreeName(strFullName);
				}
			} else
				DR = ShellRemoveFile(strFullName, Wipe, Opt_DeleteToRecycleBin);

			if (DR == DELETE_CANCEL)
				return DELETE_CANCEL;

			if (DR == DELETE_SUCCESS && UpdateDiz)
				SrcPanel->DeleteDiz(strFullName);
		}
	}

	if (FileAttr & FILE_ATTRIBUTE_READONLY)
		apiMakeWritable(strSelName);

	/*
		нефига здесь выделываться, а надо учесть, что удаление
		симлинка в корзину чревато потерей оригинала.
	*/
	if (DirSymLink || !Opt_DeleteToRecycleBin || Wipe)
		return ERemoveDirectory(strSelName, Wipe);

	return RemoveToRecycleBin(strSelName);
}

static void FormatDeleteMultipleFilesMsg(FARString &strDeleteFilesMsg, const int SelCount)
{
	// в зависимости от числа ставим нужное окончание
	const wchar_t *Ends = Msg::AskDeleteItemsA;
	wchar_t StrItems[16];
	_itow(SelCount, StrItems, 10);
	const int LenItems = StrLength(StrItems);

	if (LenItems > 0) {
		if ((LenItems >= 2 && StrItems[LenItems - 2] == L'1') || StrItems[LenItems - 1] >= L'5'
				|| StrItems[LenItems - 1] == L'0')
			Ends = Msg::AskDeleteItemsS;
		else if (StrItems[LenItems - 1] == L'1')
			Ends = Msg::AskDeleteItems0;
	}

	strDeleteFilesMsg.Format(Msg::AskDeleteItems, SelCount, Ends);
}

static bool ShellConfirmDeletion(Panel *SrcPanel, bool &Wipe)
{
	const int SelCount = SrcPanel->GetSelCount();
	if (SelCount <= 0)
		return false;

	FARString strSelName;
	DWORD FileAttr;
	SrcPanel->GetSelNameCompat(nullptr, FileAttr);
	SrcPanel->GetSelNameCompat(&strSelName, FileAttr);

	const bool IsDir = (FileAttr & FILE_ATTRIBUTE_DIRECTORY) != 0;
	const bool IsLink = (FileAttr & FILE_ATTRIBUTE_REPARSE_POINT) != 0;
	FARString strDeleteFilesMsg;

	if (SelCount == 1) {
		if (strSelName.IsEmpty() || TestParentFolderName(strSelName))
			return false;

		strDeleteFilesMsg = strSelName;
	} else {
		FormatDeleteMultipleFilesMsg(strDeleteFilesMsg, SelCount);
	}

	if (Opt.Confirm.Delete && SelCount == 1 && IsLink)
	{
		FARString str;
		str.Format(L"%ls %ls",  Msg::AskDeleteLink.CPtr(),  (IsDir ? Msg::AskDeleteLinkFolder.CPtr() : Msg::AskDeleteLinkFile.CPtr()));
		if (Wipe & IsDir)
			Wipe = false; // never wipe directory by symlink
		FarLangMsg OkMsg = (Opt.DeleteToRecycleBin ? Msg::DeleteRecycle : Msg::Delete);
		SetMessageHelp(L"DeleteFile");
		if (Wipe) {
			switch (Message(0, 3, Msg::DeleteLinkTitle, Msg::AskDelete, strDeleteFilesMsg, str, Msg::DeleteWipe, OkMsg, Msg::Cancel)) {
				case 2:
					return false;
				case 1:
					Wipe = false;
			}
		}
		else if (Message(0, 2, Msg::DeleteLinkTitle, Msg::AskDelete, strDeleteFilesMsg, str, OkMsg, Msg::Cancel))
			return false;
	}
	else if (Opt.Confirm.Delete || SelCount > 1)		// || (FileAttr & FILE_ATTRIBUTE_DIRECTORY)))
	{
		FarLangMsg TitleMsg = Wipe ? Msg::DeleteWipeTitle : Msg::DeleteTitle;
		/*
			$ 05.01.2001 IS
			! Косметика в сообщениях - разные сообщения в зависимости от того,
			какие и сколько элементов выделено.
		*/
		FarLangMsg DelMsg;
		if (SelCount == 1) {
			if (Wipe && !IsLink)
				DelMsg = IsDir ? Msg::AskWipeFolder : Msg::AskWipeFile;
			else if (Opt.DeleteToRecycleBin && !IsLink)
				DelMsg = IsDir ? Msg::AskDeleteRecycleFolder : Msg::AskDeleteRecycleFile;
			else
				DelMsg = IsDir ? Msg::AskDeleteFolder : Msg::AskDeleteFile;
		} else if (Wipe && !IsLink)
			DelMsg = Msg::AskWipe;
		else if (Opt.DeleteToRecycleBin && !IsLink)
			DelMsg = Msg::AskDeleteRecycle;
		else
			DelMsg = Msg::AskDelete;

		FarLangMsg OkMsg =
				Wipe ? Msg::DeleteWipe : (Opt.DeleteToRecycleBin ? Msg::DeleteRecycle : Msg::Delete);

		SetMessageHelp(L"DeleteFile");
		if (Message(0, 2, TitleMsg, DelMsg, strDeleteFilesMsg, OkMsg, Msg::Cancel))
			return false;
	}

	if (Opt.Confirm.Delete && SelCount > 1) {
		// SaveScreen SaveScr;
		SetCursorType(FALSE, 0);
		SetMessageHelp(L"DeleteFile");

		FarLangMsg TitleMsg = Wipe ? Msg::WipeFilesTitle : Msg::DeleteFilesTitle;
		FarLangMsg DelMsg = Wipe ? Msg::AskWipe : Msg::AskDelete;
		if (Message(MSG_WARNING, 2, TitleMsg, DelMsg, strDeleteFilesMsg, Msg::DeleteFileAll,
					Msg::DeleteFileCancel))
			return false;
	}

	return true;
}

static ULONG ShellCalcCountOfItemsToDelete(Panel *SrcPanel, bool Wipe)
{
	ULONG ItemsCount = 0;

	DWORD FileAttr;
	FARString strSelName;
	ShellDeleteMsgState SDMS;

	SrcPanel->GetSelNameCompat(nullptr, FileAttr);
	while (SrcPanel->GetSelNameCompat(&strSelName, FileAttr)) {
		if (!(FileAttr & FILE_ATTRIBUTE_REPARSE_POINT)) {
			if (FileAttr & FILE_ATTRIBUTE_DIRECTORY) {
				if (!SDMS.Update(strSelName, Wipe))
					return (ULONG)-1;

				uint32_t CurrentFileCount, CurrentDirCount, ClusterSize;
				UINT64 FileSize, PhysicalSize;

				if (GetDirInfo(nullptr, strSelName, CurrentDirCount, CurrentFileCount, FileSize, PhysicalSize,
							ClusterSize, -1, nullptr, 0)
						<= 0)
					return (ULONG)-1;

				ItemsCount+= CurrentFileCount + CurrentDirCount + 1;
			} else {
				ItemsCount++;
			}
		}
	}

	return ItemsCount;
}

void ShellDelete(Panel *SrcPanel, bool Wipe)
{
	SudoClientRegion scr;
	TPreRedrawFuncGuard preRedrawFuncGuard(PR_ShellDeleteMsg);
	DeleteAllFolders = !Opt.Confirm.DeleteFolder;

	const bool UpdateDiz = (Opt.Diz.UpdateMode == DIZ_UPDATE_ALWAYS
			|| (SrcPanel->IsDizDisplayed() && Opt.Diz.UpdateMode == DIZ_UPDATE_IF_DISPLAYED));

	if (!ShellConfirmDeletion(SrcPanel, Wipe))
		return;

	// TODO: Удаление в корзину только для FIXED-дисков
	int Opt_DeleteToRecycleBin = Opt.DeleteToRecycleBin;

	if (UpdateDiz)
		SrcPanel->ReadDiz();

	FARString strDizName;
	SrcPanel->GetDizName(strDizName);

	const bool DizPresent = (!strDizName.IsEmpty() && apiPathExists(strDizName));

	/*& 31.05.2001 OT Запретить перерисовку текущего фрейма*/
	LockCurrentFrame LCF;
	LCF.RefreshOnUnlock();
	wakeful W;
	if (SrcPanel->GetType() == TREE_PANEL)
		FarChDir(L"/");
	// SaveScreen SaveScr;
	SetCursorType(FALSE, 0);
	ReadOnlyDeleteMode = -1;
	SkipMode = -1;
	SkipWipeMode = -1;
	SkipFoldersMode = -1;
	SkipRecycleMode = -1;
	ProcessedItems = 0;

	ULONG ItemsCount = 0;

	if (Opt.DelOpt.DelShowTotal) {
		ItemsCount = ShellCalcCountOfItemsToDelete(SrcPanel, Wipe);
		if (ItemsCount == (ULONG)-1)	// cancelled?
			return;
	}

	DWORD FileAttr;
	SrcPanel->GetSelNameCompat(nullptr, FileAttr);

	bool NeedSetUpADir = false;
	ShellDeleteMsgState SDMS;
	FARString strSelName;
	while (SrcPanel->GetSelNameCompat(&strSelName, FileAttr)) {
		if (strSelName.IsEmpty())
			continue;

		if (!SDMS.Update(strSelName, Wipe, ProcessedItems, ItemsCount))
			break;

		DeletionResult DR;
		if (FileAttr & FILE_ATTRIBUTE_DIRECTORY) {
			if (!NeedSetUpADir)
				NeedSetUpADir = CheckUpdateAnotherPanel(SrcPanel, strSelName);

			DR = ShellDeleteDirectory(ItemsCount, UpdateDiz, SrcPanel, strSelName, FileAttr, Wipe,
					Opt_DeleteToRecycleBin);
			if (DR == DELETE_NO_RECYCLE_BIN)
				DR = ShellDeleteDirectory(ItemsCount, UpdateDiz, SrcPanel, strSelName, FileAttr, Wipe, FALSE);
		} else {
			DR = ShellRemoveFile(strSelName, Wipe, Opt_DeleteToRecycleBin);
		}

		if (DR == DELETE_CANCEL)
			break;

		if (DR == DELETE_SUCCESS && UpdateDiz) {
			SrcPanel->DeleteDiz(strSelName);
		}
	}

	if (UpdateDiz && DizPresent == (!strDizName.IsEmpty() && apiPathExists(strDizName)))
		SrcPanel->FlushDiz();

	ShellUpdatePanels(SrcPanel, NeedSetUpADir);
}

static void PR_ShellDeleteMsg()
{
	PreRedrawItem preRedrawItem = PreRedraw.Peek();
	ShellDeleteMsg(static_cast<const wchar_t *>(preRedrawItem.Param.Param1),
			static_cast<int>(reinterpret_cast<INT_PTR>(preRedrawItem.Param.Param4)),
			static_cast<int>(preRedrawItem.Param.Param5));
}

void ShellDeleteMsg(const wchar_t *Name, bool Wipe, int Percent)
{
	FARString strProgress;
	size_t Width = 52;

	if (Percent != -1) {
		size_t Length = Width - 5;	// -5 под проценты
		size_t CurPos = Min(Percent, 100) * Length / 100;
		strProgress.Reserve(Length);
		strProgress.Append(BoxSymbols[BS_X_DB], CurPos);
		strProgress.Append(BoxSymbols[BS_X_B0], Length - CurPos);
		FormatString strTmp;
		strTmp << L" " << fmt::Expand(3) << Percent << L"%";
		strProgress+= strTmp;
	}

	FARString strOutFileName(Name);
	TruncPathStr(strOutFileName, static_cast<int>(Width));
	CenterStr(strOutFileName, strOutFileName, static_cast<int>(Width));
	Message(0, 0, (Wipe ? Msg::DeleteWipeTitle : Msg::DeleteTitle),
			(Percent >= 0 || !Opt.DelOpt.DelShowTotal)
					? (Wipe ? Msg::DeletingWiping : Msg::Deleting)
					: Msg::ScanningFolder,
			strOutFileName, strProgress.IsEmpty() ? nullptr : strProgress.CPtr());
	PreRedrawItem preRedrawItem = PreRedraw.Peek();
	preRedrawItem.Param.Param1 = static_cast<void *>(const_cast<wchar_t *>(Name));
	preRedrawItem.Param.Param4 = (void *)(INT_PTR)Wipe;
	preRedrawItem.Param.Param5 = (int64_t)Percent;
	PreRedraw.SetParam(preRedrawItem.Param);
}

AskDeleteReadOnly::AskDeleteReadOnly(const wchar_t *Name, bool Wipe)
	:
	_r(DELETE_YES)
{
	int MsgCode;
	_ump = apiMakeWritable(Name);

	if (!_ump)		//(Attr & FILE_ATTRIBUTE_READONLY))
		return;		//(DELETE_YES);

	if (!Opt.Confirm.RO)
		ReadOnlyDeleteMode = 1;

	if (ReadOnlyDeleteMode != -1)
		MsgCode = ReadOnlyDeleteMode;
	else {
		MsgCode = Message(MSG_WARNING, 5, Msg::Warning, Msg::DeleteRO, Name,
				(Wipe ? Msg::AskWipeRO : Msg::AskDeleteRO),
				(Wipe ? Msg::DeleteFileWipe : Msg::DeleteFileDelete), Msg::DeleteFileAll, Msg::DeleteFileSkip,
				Msg::DeleteFileSkipAll, Msg::DeleteFileCancel);
	}

	switch (MsgCode) {
		case 1:
			ReadOnlyDeleteMode = 1;
			break;
		case 2:
			_r = DELETE_SKIP;
			return;
		case 3:
			ReadOnlyDeleteMode = 3;
			_r = DELETE_SKIP;
			return;
		case -1:
		case -2:
		case 4:
			_r = DELETE_CANCEL;
			return;
	}

	// apiSetFileAttributes(Name,FILE_ATTRIBUTE_NORMAL);
	// return(DELETE_YES);
}

static DeletionResult ShellRemoveFile(const wchar_t *Name, bool Wipe, int Opt_DeleteToRecycleBin)
{
	ProcessedItems++;
	int MsgCode = 0;
	std::unique_ptr<AskDeleteReadOnly> AskDeleteRO;
	/*
		have to pretranslate to full path otherwise plain remove()
		below will malfunction if current directory requires sudo
	*/
	FARString strFullName;
	ConvertNameToFull(Name, strFullName);

	if (Wipe || Opt_DeleteToRecycleBin) {
		/*
			in case its a not a simple deletion - check/sanitize RO files prior any actions,
			cuz code that doing such things is not aware about such complications
		*/
		AskDeleteRO.reset(new AskDeleteReadOnly(strFullName, Wipe));
		if (AskDeleteRO->Choice() != DELETE_YES)
			return AskDeleteRO->Choice();
	}

	for (;;) {
		if (Wipe) {
			bool is_symlink = false;
			int n_hardlinks = 1;
			struct stat s{};
			if ( sdc_lstat(Wide2MB(Name).c_str(), &s) == 0 ) {
				n_hardlinks = (s.st_nlink > 0) ? s.st_nlink : 1;
				is_symlink = (s.st_mode & S_IFMT) == S_IFLNK;
			}

			if (SkipWipeMode != -1) {
				MsgCode = SkipWipeMode;
			} else if (is_symlink) {
				//                            Файл
				//                         "имя файла"
				//                     Это symlink на файл.
				//  Уничтожение файла приведет к обнулению всех ссылающихся на него файлов.
				//                        Уничтожать файл?
				MsgCode = Message(MSG_WARNING, 5, Msg::Error, strFullName, Msg::DeleteSymLink1,
						Msg::DeleteHardLink2, Msg::DeleteHardLink3, Msg::DeleteFileWipe, Msg::DeleteFileAll,
						Msg::DeleteFileSkip, Msg::DeleteFileSkipAll, Msg::DeleteCancel);
			} else if (n_hardlinks > 1) {
				//                            Файл
				//                         "имя файла"
				//                Файл имеет несколько жестких ссылок.
				//  Уничтожение файла приведет к обнулению всех ссылающихся на него файлов.
				//                        Уничтожать файл?
				MsgCode = Message(MSG_WARNING, 5, Msg::Error, strFullName, Msg::DeleteHardLink1,
						Msg::DeleteHardLink2, Msg::DeleteHardLink3, Msg::DeleteFileWipe, Msg::DeleteFileAll,
						Msg::DeleteFileSkip, Msg::DeleteFileSkipAll, Msg::DeleteCancel);
			}
			// !!! [All] & [Skip all] now equivalent for wipe symlink & file with several hardlink - may be do separete?

			switch (MsgCode) {
				case -1:
				case -2:
				case 4:
					return DELETE_CANCEL;
				case 3:
					SkipWipeMode = 2;
				case 2:
					return DELETE_SKIP;
				case 1:
					SkipWipeMode = 0;
				case 0:

					if (WipeFile(strFullName))
						return DELETE_SUCCESS;
			}
		} else if (!Opt_DeleteToRecycleBin) {
			/*
				first try simple removal, only if it will fail
				then fallback to AskDeleteRO and sdc_remove
			*/
			const std::string &mbFullName = strFullName.GetMB();
			if (!AskDeleteRO) {
				if (remove(mbFullName.c_str()) == 0 || errno == ENOENT) {
					break;
				}
				AskDeleteRO.reset(new AskDeleteReadOnly(strFullName, Wipe));
				if (AskDeleteRO->Choice() != DELETE_YES)
					return AskDeleteRO->Choice();
			}
			if (sdc_remove(mbFullName.c_str()) == 0 || errno == ENOENT) {
				break;
			}
		} else {
			const auto DR = RemoveToRecycleBin(strFullName);
			if (DR == DELETE_NO_RECYCLE_BIN) {
				Opt_DeleteToRecycleBin = 0;
				continue;
			}
			return DR;
		}

		if (SkipMode != -1)
			MsgCode = SkipMode;
		else {
			MsgCode = Message(MSG_WARNING | MSG_ERRORTYPE, 4, Msg::Error, Msg::CannotDeleteFile, strFullName,
					Msg::DeleteRetry, Msg::DeleteSkip, Msg::DeleteFileSkipAll, Msg::DeleteCancel);
		}

		switch (MsgCode) {
			case -1:
			case -2:
			case 3:
				return DELETE_CANCEL;
			case 2:
				SkipMode = 1;
			case 1:
				return DELETE_SKIP;
		}
	}

	return DELETE_SUCCESS;
}

DeletionResult ERemoveDirectory(const wchar_t *Name, bool Wipe)
{
	bool is_symlink;
	struct stat s{};

	ProcessedItems++;

	for (;;) {
		is_symlink = sdc_lstat(Wide2MB(Name).c_str(), &s) == 0 && (s.st_mode & S_IFMT) == S_IFLNK;
		if (Wipe && !is_symlink) { // !!! silently never wipe symlink, may be need message ???
			if (WipeDirectory(Name))
				break;
		} else {
			if (is_symlink) {
				if (sdc_unlink(Wide2MB(Name).c_str()) == 0)		// if (apiDeleteFile(Name))
					break;
			}
			if (apiRemoveDirectory(Name))
				break;
		}

		int MsgCode;

		if (SkipFoldersMode != -1)
			MsgCode = SkipFoldersMode;
		else {
			FARString strFullName;
			ConvertNameToFull(Name, strFullName);

			MsgCode = Message(MSG_WARNING | MSG_ERRORTYPE, 4, Msg::Error, Msg::CannotDeleteFolder, Name,
					Msg::DeleteRetry, Msg::DeleteSkip, Msg::DeleteFileSkipAll, Msg::DeleteCancel);
		}

		switch (MsgCode) {
			case -1:
			case -2:
			case 3:
				return DELETE_CANCEL;
			case 2:
				SkipFoldersMode = 2;
				return DELETE_SKIP;
			case 1:
				return DELETE_SKIP;
		}
	}

	return DELETE_SUCCESS;
}

static DeletionResult RemoveToRecycleBin(const wchar_t *Name)
{
	FARString err_file;
	FarMkTempEx(err_file, L"trash");

	FARString FullName;
	ConvertNameToFull(Name, FullName);
	std::string name_arg = FullName.GetMB();
	std::string err_file_arg = err_file.GetMB();


	unsigned int flags = EF_HIDEOUT;
	if (sudo_client_is_required_for(ExtractFilePath(name_arg).c_str(), true)
		|| (apiPathIsDir(FullName) && sudo_client_is_required_for(name_arg.c_str(), true)))
		flags|= EF_SUDO;

	QuoteCmdArgIfNeed(name_arg);
	QuoteCmdArgIfNeed(err_file_arg);

	std::string cmd = GetMyScriptQuoted("trash.sh");
	cmd+= ' ';
	cmd+= name_arg;
	cmd+= ' ';
	cmd+= err_file_arg;

	for (;;) {
		int r = farExecuteA(cmd.c_str(), flags);

		std::string err_str;
		if (r != 0 && ReadWholeFile(err_file.GetMB().c_str(), err_str, 0x10000)) {
			// gio: file:///.../xxx/yyy: Unable to trash file .../xxx/yyy: Permission denied
			err_str.erase(0, err_str.rfind(':') + 1);
			StrTrim(err_str, " \t\r\n");
			SetErrorString(err_str);
		}
		unlink(err_file.GetMB().c_str());

		if (r == 0)
			return DELETE_SUCCESS;

		const int MsgCode = (SkipRecycleMode != -1)
				? SkipRecycleMode
				: Message(MSG_WARNING | MSG_ERRORTYPE, 5, Msg::Error, Msg::CannotDeleteFile, Name,
						Msg::DeleteRetry, Msg::DeleteRetryNotRecycleBin, Msg::DeleteSkip,
						Msg::DeleteFileSkipAll, Msg::DeleteCancel);

		switch (MsgCode) {
			case -1:
			case 4:
				return DELETE_CANCEL;
			case 3:
				SkipRecycleMode = 2;
			case 2:
				return DELETE_SKIP;
			case 1:
				return DELETE_NO_RECYCLE_BIN;
		}
	}
}

static FARString WipingRename(const wchar_t *Name)
{
	FARString strTempName = Name;
	char tmpName[33]{};
	for (size_t tmpLen = 4;;) {
		CutToSlash(strTempName, false);
		RandomStringBuffer(tmpName, tmpLen, tmpLen, RNDF_ALNUM);
		strTempName+= tmpName;
		if (!apiPathExists(strTempName)) {
			break;
		}
		if (tmpLen + 1 < sizeof(tmpName) - 1) {
			++tmpLen;
		}
	}

	if (!apiMoveFile(Name, strTempName)) {
		fprintf(stderr, "%s: error %u renaming '%ls' to '%ls'\n", __FUNCTION__, errno, Name,
				strTempName.CPtr());
		return Name;
	}

	fprintf(stderr, "%s: renamed '%ls' to '%ls'\n", __FUNCTION__, Name, strTempName.CPtr());

	return strTempName;
}

static bool WipeFile(const wchar_t *Name)
{
	apiMakeWritable(Name);	// apiSetFileAttributes(Name,FILE_ATTRIBUTE_NORMAL);
	File WipeFile;
	uint64_t FileSize;
	if (!WipeFile.Open(Name, GENERIC_WRITE, 0, nullptr, OPEN_EXISTING,
				FILE_FLAG_WRITE_THROUGH | FILE_FLAG_SEQUENTIAL_SCAN)
			|| !WipeFile.GetSize(FileSize)) {
		return false;
	}

	if (FileSize) {
		std::vector<BYTE> Buf(0x10000, (BYTE)(unsigned int)Opt.WipeSymbol);
		// fill equal to actual size of file to ensure it will overwrite original sectors
		for (uint64_t WrittenSize = 0; WrittenSize < FileSize;) {
			DWORD WriteSize = (DWORD)Min((uint64_t)Buf.size(), FileSize - WrittenSize);
			if (!WipeFile.Write(Buf.data(), WriteSize, &WriteSize) || WriteSize == 0)
				return false;

			WrittenSize+= WriteSize;
			if (WriteSize < Buf.size()) {	// append alignment tail to hide original size
				if (WipeFile.Write(Buf.data(), DWORD(Buf.size() - WriteSize), &WriteSize))
					WrittenSize+= WriteSize;
			}
		}
		WipeFile.SetPointer(0, nullptr, FILE_BEGIN);
		WipeFile.SetEnd();
	}

	WipeFile.Close();

	FARString strRemoveName = WipingRename(Name);
	return apiDeleteFile(strRemoveName) != FALSE;
}

static bool WipeDirectory(const wchar_t *Name)
{
	FARString strTempName, strPath;

	if (FirstSlash(Name)) {
		strPath = Name;
		DeleteEndSlash(strPath);
		CutToSlash(strPath);
	}

	FARString strRemoveName = WipingRename(Name);
	return apiRemoveDirectory(strRemoveName) != FALSE;
}

int DeleteFileWithFolder(const wchar_t *FileName)
{
	SudoClientRegion scr;
	FARString strFileOrFolderName, strParentFolder;
	strFileOrFolderName = FileName;
	Unquote(strFileOrFolderName);

	strParentFolder = strFileOrFolderName;
	CutToSlash(strParentFolder, true);
	if (!strParentFolder.IsEmpty())
		apiMakeWritable(strParentFolder);
	apiMakeWritable(strFileOrFolderName);
	/*BOOL Ret=apiSetFileAttributes(strFileOrFolderName,FILE_ATTRIBUTE_NORMAL);

	if (Ret)*/
	{
		if (apiDeleteFile(strFileOrFolderName))		// BUGBUG
		{
			return apiRemoveDirectory(strParentFolder);
		}
	}

	WINPORT(SetLastError)((_localLastError = WINPORT(GetLastError)()));
	return FALSE;
}

void DeleteDirTree(const wchar_t *Dir)
{
	if (!*Dir || (IsSlash(Dir[0]) && !Dir[1]))
		return;

	SudoClientRegion scr;
	FARString strFullName;
	FAR_FIND_DATA_EX FindData;
	ScanTree ScTree(TRUE, TRUE, FALSE);
	ScTree.SetFindPath(Dir, L"*", 0);

	while (ScTree.GetNextName(&FindData, strFullName)) {
		if (FindData.dwFileAttributes & FILE_ATTRIBUTE_READONLY)
			apiMakeWritable(strFullName);
		//		apiSetFileAttributes(strFullName,FILE_ATTRIBUTE_NORMAL);

		if ((FindData.dwFileAttributes & (FILE_ATTRIBUTE_DIRECTORY | FILE_ATTRIBUTE_REPARSE_POINT))
				== FILE_ATTRIBUTE_DIRECTORY) {
			if (ScTree.IsDirSearchDone())
				apiRemoveDirectory(strFullName);
		} else
			apiDeleteFile(strFullName);
	}
	apiMakeWritable(Dir);
	//	apiSetFileAttributes(Dir,FILE_ATTRIBUTE_NORMAL);
	apiRemoveDirectory(Dir);
}
