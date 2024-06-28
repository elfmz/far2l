/*
treelist.cpp

Tree panel
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

#include <sys/stat.h>
#include <sys/statvfs.h>
#if defined(__APPLE__) || defined(__FreeBSD__) || defined(__DragonFly__) || defined(__CYGWIN__)
#include <sys/mount.h>
#elif !defined(__HAIKU__)
#include <sys/statfs.h>
#endif

#include "treelist.hpp"
#include "keyboard.hpp"
#include "colors.hpp"
#include "lang.hpp"
#include "keys.hpp"
#include "filepanels.hpp"
#include "filelist.hpp"
#include "cmdline.hpp"
#include "chgprior.hpp"
#include "scantree.hpp"
#include "copy.hpp"
#include "qview.hpp"
#include "savescr.hpp"
#include "ctrlobj.hpp"
#include "help.hpp"
#include "lockscrn.hpp"
#include "macroopcode.hpp"
#include "RefreshFrameManager.hpp"
#include "scrbuf.hpp"
#include "TPreRedrawFunc.hpp"
#include "cddrv.hpp"
#include "interf.hpp"
#include "message.hpp"
#include "clipboard.hpp"
#include "config.hpp"
#include "delete.hpp"
#include "mkdir.hpp"
#include "setattr.hpp"
#include "execute.hpp"
#include "Bookmarks.hpp"
#include "dirmix.hpp"
#include "pathmix.hpp"
#include "processname.hpp"
#include "constitle.hpp"
#include "syslog.hpp"
#include "cache.hpp"
#include "filestr.hpp"
#include "wakeful.hpp"

static int _cdecl SortList(const void *el1, const void *el2);
static int _cdecl SortCacheList(const void *el1, const void *el2);
static int StaticSortNumeric;
static int StaticSortCaseSensitive;
static int TreeCmp(const wchar_t *Str1, const wchar_t *Str2, int Numeric, int CaseSensitive);
static clock_t TreeStartTime;
static int LastScrX = -1;
static int LastScrY = -1;

static struct TreeListCache
{
	FARString strTreeName;
	wchar_t **ListName;
	int TreeCount;
	int TreeSize;

	TreeListCache()
	{
		ListName = nullptr;
		TreeCount = 0;
		TreeSize = 0;
	}

	void Resize()
	{
		if (TreeCount == TreeSize) {
			TreeSize+= TreeSize ? TreeSize >> 2 : 32;
			wchar_t **NewPtr = (wchar_t **)realloc(ListName, sizeof(wchar_t *) * TreeSize);

			if (!NewPtr)
				return;

			ListName = NewPtr;
		}
	}

	void Add(const wchar_t *name)
	{
		Resize();
		ListName[TreeCount++] = wcsdup(name);
	}

	void Insert(int idx, const wchar_t *name)
	{
		Resize();
		memmove(ListName + idx + 1, ListName + idx, sizeof(wchar_t *) * (TreeCount - idx));
		ListName[idx] = wcsdup(name);
		TreeCount++;
	}

	void Delete(int idx)
	{
		if (ListName[idx])
			free(ListName[idx]);

		memmove(ListName + idx, ListName + idx + 1, sizeof(wchar_t *) * (TreeCount - idx - 1));
		TreeCount--;
	}

	void Clean()
	{
		if (!TreeSize)
			return;

		for (int i = 0; i < TreeCount; i++) {
			if (ListName[i])
				free(ListName[i]);
		}

		if (ListName)
			free(ListName);

		ListName = nullptr;
		TreeCount = 0;
		TreeSize = 0;
		strTreeName.Clear();
	}

	// TODO: необходимо оптимизировать!
	void Copy(TreeListCache *Dest)
	{
		Dest->Clean();

		for (int I = 0; I < TreeCount; I++)
			Dest->Add(ListName[I]);
	}

} TreeCache, tempTreeCache;

TreeList::TreeList(int IsPanel)
	:
	PrevMacroMode(-1),
	ListData(nullptr),
	TreeCount(0),
	WorkDir(0),
	GetSelPosition(0),
	NumericSort(FALSE),
	CaseSensitiveSort(FALSE),
	ExitCode(1),
	SaveListData(nullptr)
{
	Type = TREE_PANEL;
	CurFile = CurTopFile = 0;
	Flags.Set(FTREELIST_UPDATEREQUIRED);
	Flags.Clear(FTREELIST_TREEISPREPARED);
	Flags.Change(FTREELIST_ISPANEL, IsPanel);
}

TreeList::~TreeList()
{
	if (ListData) {
		for (long i = 0; i < TreeCount; i++)
			delete ListData[i];

		free(ListData);
	}

	if (SaveListData)
		delete[] SaveListData;

	tempTreeCache.Clean();
	FlushCache();
	SetMacroMode(TRUE);
}

void TreeList::SetRootDir(const wchar_t *NewRootDir)
{
	strRoot = NewRootDir;
	strCurDir = NewRootDir;
}

void TreeList::DisplayObject()
{
	if (Flags.Check(FSCROBJ_ISREDRAWING))
		return;

	Flags.Set(FSCROBJ_ISREDRAWING);

	if (Flags.Check(FTREELIST_UPDATEREQUIRED))
		Update(0);

	if (ExitCode) {
		Panel *RootPanel = GetRootPanel();

		if (RootPanel->GetType() == FILE_PANEL) {
			int RootCaseSensitiveSort = RootPanel->GetCaseSensitiveSort();
			int RootNumeric = RootPanel->GetNumericSort();

			if (RootNumeric != NumericSort || RootCaseSensitiveSort != CaseSensitiveSort) {
				NumericSort = RootNumeric;
				CaseSensitiveSort = RootCaseSensitiveSort;
				StaticSortNumeric = NumericSort;
				StaticSortCaseSensitive = CaseSensitiveSort;
				far_qsort(ListData, TreeCount, sizeof(*ListData), SortList);
				FillLastData();
				SyncDir();
			}
		}

		DisplayTree(FALSE);
	}

	Flags.Clear(FSCROBJ_ISREDRAWING);
}

FARString &TreeList::GetTitle(FARString &strTitle, int SubLen, int TruncSize)
{
	strTitle.Format(L" %ls ", (ModalMode ? Msg::FindFolderTitle : Msg::TreeTitle).CPtr());
	TruncStr(strTitle, X2 - X1 - 3);
	return strTitle;
}

void TreeList::DisplayTree(int Fast)
{
	wchar_t TreeLineSymbol[4][3] = {
			{L' ',                   L' ',              0},
			{BoxSymbols[BS_V1],      L' ',              0},
			{BoxSymbols[BS_LB_H1V1], BoxSymbols[BS_H1], 0},
			{BoxSymbols[BS_L_H1V1],  BoxSymbols[BS_H1], 0},
	};
	TreeItem *CurPtr;
	FARString strTitle;
	LockScreen *LckScreen = nullptr;

	if (CtrlObject->Cp()->GetAnotherPanel(this)->GetType() == QVIEW_PANEL)
		LckScreen = new LockScreen;

	CorrectPosition();

	if (TreeCount > 0)
		strCurDir = ListData[CurFile]->strName;		// BUGBUG

	if (!Fast) {
		Box(X1, Y1, X2, Y2, FarColorToReal(COL_PANELBOX), DOUBLE_BOX);
		DrawSeparator(Y2 - 2 - (ModalMode));
		GetTitle(strTitle);

		if (!strTitle.IsEmpty()) {
			SetFarColor((Focus || ModalMode) ? COL_PANELSELECTEDTITLE : COL_PANELTITLE);
			GotoXY(X1 + (X2 - X1 + 1 - (int)strTitle.GetLength()) / 2, Y1);
			Text(strTitle);
		}
	}

	for (int I = Y1 + 1, J = CurTopFile; I < Y2 - 2 - (ModalMode); I++, J++) {
		GotoXY(X1 + 1, I);
		SetFarColor(COL_PANELTEXT);
		Text(L" ");

		if (J < TreeCount && Flags.Check(FTREELIST_TREEISPREPARED)) {
			CurPtr = ListData[J];

			if (!J) {
				DisplayTreeName(strRoot.CPtr(), J);
			} else {
				FARString strOutStr;

				for (int i = 0; i < CurPtr->Depth - 1 && WhereX() + 3 * i < X2 - 6; i++) {
					strOutStr+= TreeLineSymbol[CurPtr->Last[i] ? 0 : 1];
				}

				strOutStr+= TreeLineSymbol[CurPtr->Last[CurPtr->Depth - 1] ? 2 : 3];
				BoxText(strOutStr);
				const wchar_t *ChPtr = LastSlash(CurPtr->strName);

				if (ChPtr)
					DisplayTreeName(ChPtr + 1, J);
			}
		}

		SetFarColor(COL_PANELTEXT);

		if (WhereX() < X2) {
			FS << fmt::Cells() << fmt::Expand(X2 - WhereX()) << L"";
		}
	}

	if (Opt.ShowPanelScrollbar) {
		SetFarColor(COL_PANELSCROLLBAR);
		ScrollBarEx(X2, Y1 + 1, Y2 - Y1 - 3, CurTopFile, TreeCount);
	}

	SetFarColor(COL_PANELTEXT);
	SetScreen(X1 + 1, Y2 - (ModalMode ? 2 : 1), X2 - 1, Y2 - 1, L' ', FarColorToReal(COL_PANELTEXT));

	if (TreeCount > 0) {
		GotoXY(X1 + 1, Y2 - 1);
		FS << fmt::LeftAlign() << fmt::Cells() << fmt::Size(X2 - X1 - 1) << ListData[CurFile]->strName;
	}

	UpdateViewPanel();
	SetTitle();		// не забудим прорисовать заголовок

	if (LckScreen)
		delete LckScreen;
}

void TreeList::DisplayTreeName(const wchar_t *Name, int Pos)
{
	if (WhereX() > X2 - 4)
		GotoXY(X2 - 4, WhereY());

	if (Pos == CurFile) {
		GotoXY(WhereX() - 1, WhereY());

		if (Focus || ModalMode) {
			SetFarColor((Pos == WorkDir) ? COL_PANELSELECTEDCURSOR : COL_PANELCURSOR);
			FS << L" " << fmt::Cells() << fmt::Truncate(X2 - WhereX() - 3) << Name << L" ";
		} else {
			SetFarColor((Pos == WorkDir) ? COL_PANELSELECTEDTEXT : COL_PANELTEXT);
			FS << L"[" << fmt::Cells() << fmt::Truncate(X2 - WhereX() - 3) << Name << L"]";
		}
	} else {
		SetFarColor((Pos == WorkDir) ? COL_PANELSELECTEDTEXT : COL_PANELTEXT);
		FS << fmt::Cells() << fmt::Truncate(X2 - WhereX() - 1) << Name;
	}
}

void TreeList::Update(int Mode)
{
	if (!EnableUpdate)
		return;

	if (!IsVisible()) {
		Flags.Set(FTREELIST_UPDATEREQUIRED);
		return;
	}

	Flags.Clear(FTREELIST_UPDATEREQUIRED);
	GetRoot();
	int LastTreeCount = TreeCount;
	int RetFromReadTree = TRUE;
	Flags.Clear(FTREELIST_TREEISPREPARED);
	int TreeFilePresent = ReadTreeFile();

	if (!TreeFilePresent)
		RetFromReadTree = ReadTree();

	Flags.Set(FTREELIST_TREEISPREPARED);

	if (!RetFromReadTree && !Flags.Check(FTREELIST_ISPANEL)) {
		ExitCode = 0;
		return;
	}

	if (RetFromReadTree && TreeCount > 0 && (!(Mode & UPDATE_KEEP_SELECTION) || LastTreeCount != TreeCount)) {
		SyncDir();
		TreeItem *CurPtr = ListData[CurFile];

		if (apiGetFileAttributes(CurPtr->strName) == INVALID_FILE_ATTRIBUTES) {
			DelTreeName(CurPtr->strName);
			Update(UPDATE_KEEP_SELECTION);
			Show();
		}
	} else if (!RetFromReadTree) {
		Show();

		if (!Flags.Check(FTREELIST_ISPANEL)) {
			Panel *AnotherPanel = CtrlObject->Cp()->GetAnotherPanel(this);
			AnotherPanel->Update(UPDATE_KEEP_SELECTION | UPDATE_SECONDARY);
			AnotherPanel->Redraw();
		}
	}
}

int TreeList::ReadTree()
{
	ChangePriority ChPriority(ChangePriority::NORMAL);
	// SaveScreen SaveScr;
	TPreRedrawFuncGuard preRedrawFuncGuard(TreeList::PR_MsgReadTree);
	ScanTree ScTree(FALSE);
	FAR_FIND_DATA_EX fdata;
	FARString strFullName;
	SaveState();
	FlushCache();
	GetRoot();

	if (ListData) {
		for (long i = 0; i < TreeCount; i++)
			delete ListData[i];

		free(ListData);
	}

	TreeCount = 0;

	if (!(ListData = (TreeItem **)malloc((TreeCount + 256 + 1) * sizeof(TreeItem *)))) {
		RestoreState();
		return FALSE;
	}

	ListData[0] = new TreeItem;
	ListData[0]->Clear();
	ListData[0]->strName = strRoot;
	SaveScreen SaveScrTree;
	UndoGlobalSaveScrPtr UndSaveScr(&SaveScrTree);
	/*
		Т.к. мы можем вызвать диалог подтверждения (который не перерисовывает панельки,
		а восстанавливает сохраненный образ экрана, то нарисуем чистую панель
	*/
	// Redraw();
	TreeCount = 1;
	int FirstCall = TRUE, AscAbort = FALSE;
	TreeStartTime = GetProcessUptimeMSec();
	RefreshFrameManager frref(ScrX, ScrY, TreeStartTime, FALSE);	// DontRedrawFrame);
	ScTree.SetFindPath(strRoot, L"*", FSCANTREE_NOFILES | FSCANTREE_NODEVICES);
	LastScrX = ScrX;
	LastScrY = ScrY;
	wakeful W;
	while (ScTree.GetNextName(&fdata, strFullName)) {
		//		if(TreeCount > 3)
		TreeList::MsgReadTree(TreeCount, FirstCall);

		if (CheckForEscSilent()) {
			AscAbort = ConfirmAbortOp();
			FirstCall = TRUE;
		}

		if (AscAbort)
			break;

		if (!(TreeCount & 255)) {
			TreeItem **TmpListData =
					(TreeItem **)realloc(ListData, (TreeCount + 256 + 1) * sizeof(TreeItem *));

			if (!TmpListData) {
				AscAbort = TRUE;
				break;
			}

			ListData = TmpListData;
		}

		if (!(fdata.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY))
			continue;

		ListData[TreeCount] = new TreeItem;
		ListData[TreeCount]->Clear();
		ListData[TreeCount]->strName = strFullName;
		TreeCount++;
	}

	if (AscAbort && !Flags.Check(FTREELIST_ISPANEL)) {
		if (ListData) {
			for (long i = 0; i < TreeCount; i++)
				delete ListData[i];

			free(ListData);
		}

		ListData = nullptr;
		TreeCount = 0;
		RestoreState();
		return FALSE;
	}

	StaticSortNumeric = NumericSort = StaticSortCaseSensitive = CaseSensitiveSort = FALSE;
	far_qsort(ListData, TreeCount, sizeof(*ListData), SortList);

	if (!FillLastData())
		return FALSE;

	if (!AscAbort)
		SaveTreeFile();

	if (!FirstCall && !Flags.Check(FTREELIST_ISPANEL)) {	// Перерисуем другую панель - удалим следы сообщений :)
		Panel *AnotherPanel = CtrlObject->Cp()->GetAnotherPanel(this);
		AnotherPanel->Redraw();
	}

	return TRUE;
}

void TreeList::SaveTreeFile()
{
	if (TreeCount < 4)
		return;

	FARString strName;
	long I;
	size_t RootLength = strRoot.IsEmpty() ? 0 : strRoot.GetLength() - 1;
	MkTreeFileName(strRoot, strName);
	// получим и сразу сбросим атрибуты (если получится)
	DWORD FileAttributes = apiGetFileAttributes(strName);

	if (FileAttributes != INVALID_FILE_ATTRIBUTES)
		apiSetFileAttributes(strName, FILE_ATTRIBUTE_NORMAL);

	File TreeFile;
	if (!TreeFile.Open(strName, GENERIC_WRITE, FILE_SHARE_READ, nullptr, OPEN_ALWAYS,
				FILE_ATTRIBUTE_NORMAL)) {
		/*
			$ 16.10.2000 tran
			если диск должен кешироваться, то и пытаться не стоит
		*/
		if (MustBeCached(strRoot))
			if (!GetCacheTreeName(strRoot, strName, TRUE)
					|| !TreeFile.Open(strName, GENERIC_WRITE, FILE_SHARE_READ, nullptr, OPEN_ALWAYS,
							FILE_ATTRIBUTE_NORMAL))
				return;

		/* tran $ */
	}

	bool Success = true;
	CachedWrite Cache(TreeFile);
	for (I = 0; I < TreeCount && Success; I++) {
		if (RootLength >= ListData[I]->strName.GetLength()) {
			DWORD Size = 2 * sizeof(WCHAR);
			Success = Cache.Write(L"/\n", Size);
		} else {
			DWORD Size = static_cast<DWORD>((ListData[I]->strName.GetLength() - RootLength) * sizeof(WCHAR));
			Success = Cache.Write(ListData[I]->strName.SubStr(RootLength), Size);
			Size = 1 * sizeof(WCHAR);
			Success = Cache.Write(L"\n", Size);
		}
	}
	Cache.Flush();
	TreeFile.Close();

	if (!Success) {
		apiDeleteFile(strName);
		Message(MSG_WARNING | MSG_ERRORTYPE, 1, Msg::Error, Msg::CannotSaveTree, strName, Msg::Ok);
	} else if (FileAttributes != INVALID_FILE_ATTRIBUTES)	// вернем атрибуты (если получится :-)
		apiSetFileAttributes(strName, FileAttributes);
}

int TreeList::GetCacheTreeName(const wchar_t *Root, FARString &strName, int CreateDir)
{
	unsigned long long fsid = 0;
	struct statfs sfs{};
	memcpy(&fsid, &sfs.f_fsid, std::min(sizeof(fsid), sizeof(sfs.f_fsid)));

	if (sdc_statfs(Wide2MB(Root).c_str(), &sfs) != 0) {
		return FALSE;
	}

	FARString strFolderName;
	FARString strFarPath;
	MkTreeCacheFolderName(strFarPath, strFolderName);

	if (CreateDir) {
		apiCreateDirectory(strFolderName, nullptr);
		apiSetFileAttributes(strFolderName, Opt.Tree.TreeFileAttr);
	}

	strName.Format(L"%ls/%llx", strFolderName.CPtr(), fsid);
	return TRUE;
}

void TreeList::GetRoot()
{
	FARString strPanelDir;
	Panel *RootPanel = GetRootPanel();
	RootPanel->GetCurDir(strPanelDir);
	strRoot = strPanelDir;
}

Panel *TreeList::GetRootPanel()
{
	Panel *RootPanel;

	if (ModalMode) {
		if (ModalMode == MODALTREE_ACTIVE)
			RootPanel = CtrlObject->Cp()->ActivePanel;
		else if (ModalMode == MODALTREE_FREE)
			RootPanel = this;
		else {
			RootPanel = CtrlObject->Cp()->GetAnotherPanel(CtrlObject->Cp()->ActivePanel);

			if (!RootPanel->IsVisible())
				RootPanel = CtrlObject->Cp()->ActivePanel;
		}
	} else
		RootPanel = CtrlObject->Cp()->GetAnotherPanel(this);

	return (RootPanel);
}

void TreeList::SyncDir()
{
	FARString strPanelDir;
	Panel *AnotherPanel = GetRootPanel();
	AnotherPanel->GetCurDir(strPanelDir);

	if (!strPanelDir.IsEmpty()) {
		if (AnotherPanel->GetType() == FILE_PANEL) {
			if (!SetDirPosition(strPanelDir)) {
				ReadSubTree(strPanelDir);
				ReadTreeFile();
				SetDirPosition(strPanelDir);
			}
		} else
			SetDirPosition(strPanelDir);
	}
}

void TreeList::PR_MsgReadTree()
{
	int FirstCall = 1;
	PreRedrawItem preRedrawItem = PreRedraw.Peek();
	TreeList::MsgReadTree(preRedrawItem.Param.Flags, FirstCall);
}

int TreeList::MsgReadTree(int TreeCount, int &FirstCall)
{
	/*
		$ 24.09.2001 VVM
		! Писать сообщение о чтении дерева только, если это заняло более 500 мсек.
	*/
	BOOL IsChangeConsole = LastScrX != ScrX || LastScrY != ScrY;

	if (IsChangeConsole) {
		LastScrX = ScrX;
		LastScrY = ScrY;
	}

	if (IsChangeConsole || (GetProcessUptimeMSec() - TreeStartTime) > 1000) {
		wchar_t NumStr[32];
		_itow(TreeCount, NumStr, 10);	// BUGBUG
		Message((FirstCall ? 0 : MSG_KEEPBACKGROUND), 0, Msg::TreeTitle, Msg::ReadingTree, NumStr);
		PreRedrawItem preRedrawItem = PreRedraw.Peek();
		preRedrawItem.Param.Flags = TreeCount;
		PreRedraw.SetParam(preRedrawItem.Param);
		TreeStartTime = GetProcessUptimeMSec();
	}

	return 1;
}

bool TreeList::FillLastData()
{
	const size_t RootLength = strRoot.IsEmpty() ? 0 : strRoot.GetLength() - 1;

	for (int I = 1; I < TreeCount; I++) {
		int PathLength;
		size_t Pos, Depth;

		if (ListData[I]->strName.RPos(Pos, GOOD_SLASH))
			PathLength = (int)Pos + 1;
		else
			PathLength = 0;

		ListData[I]->Depth = Depth = CountSlash(ListData[I]->strName.CPtr() + RootLength);

		if (!Depth)
			return false;

		bool Last;
		int J, SubDirPos;
		for (J = I + 1, SubDirPos = I, Last = true; J < TreeCount; J++) {
			if (CountSlash(ListData[J]->strName.CPtr() + RootLength) > Depth) {
				SubDirPos = J;
				continue;
			} else {
				if (!StrCmpNI(ListData[I]->strName, ListData[J]->strName, PathLength))
					Last = false;

				break;
			}
		}

		for (int J = I; J <= SubDirPos; J++) {
			TreeItem::LastT &JLast = ListData[J]->Last;
			if (Depth == JLast.size()) {
				JLast.push_back(Depth);
			} else {
				if (Depth > JLast.size())
					JLast.resize(Depth);

				JLast[Depth - 1] = Last;
			}
		}
	}

	return true;
}

UINT TreeList::CountSlash(const wchar_t *Str)
{
	UINT Count = 0;

	for (; *Str; Str++)
		if (IsSlash(*Str))
			Count++;

	return (Count);
}

int64_t TreeList::VMProcess(MacroOpcode OpCode, void *vParam, int64_t iParam)
{
	switch (OpCode) {
		case MCODE_C_EMPTY:
			return TreeCount <= 0;
		case MCODE_C_EOF:
			return CurFile == TreeCount - 1;
		case MCODE_C_BOF:
			return !CurFile;
		case MCODE_C_SELECTED:
			return 0;
		case MCODE_V_ITEMCOUNT:
			return TreeCount;
		case MCODE_V_CURPOS:
			return CurFile + 1;
	}

	return 0;
}

int TreeList::ProcessKey(FarKey Key)
{
	if (!IsVisible())
		return FALSE;

	if (!TreeCount && Key != KEY_CTRLR)
		return FALSE;

	FARString strTemp;

	if (Key >= KEY_CTRLSHIFT0 && Key <= KEY_CTRLSHIFT9) {
		SaveShortcutFolder(Key - KEY_CTRLSHIFT0);
		return TRUE;
	}

	if (Key >= KEY_RCTRL0 && Key <= KEY_RCTRL9) {
		ExecShortcutFolder(Key - KEY_RCTRL0);
		return TRUE;
	}

	switch (Key) {
		case KEY_F1: {
			Help::Present(L"TreePanel");
			return TRUE;
		}
		case KEY_SHIFTNUMENTER:
		case KEY_CTRLNUMENTER:
		case KEY_SHIFTENTER:
		case KEY_CTRLENTER:
		case KEY_CTRLF:
		case KEY_CTRLALTINS:
		case KEY_CTRLALTNUMPAD0: {
			FARString strQuotedName = ListData[CurFile]->strName;
			EscapeSpace(strQuotedName);

			if (Key == KEY_CTRLALTINS || Key == KEY_CTRLALTNUMPAD0) {
				CopyToClipboard(strQuotedName);
			} else {
				if (Key == KEY_SHIFTENTER || Key == KEY_SHIFTNUMENTER) {
					Execute(strQuotedName, TRUE, TRUE);
				} else {
					strQuotedName+= L" ";
					CtrlObject->CmdLine->InsertString(strQuotedName);
				}
			}

			return TRUE;
		}
		case KEY_CTRLBACKSLASH: {
			CurFile = 0;
			ProcessEnter();
			return TRUE;
		}
		case KEY_NUMENTER:
		case KEY_ENTER: {
			if (!ModalMode && CtrlObject->CmdLine->GetLength() > 0)
				break;

			ProcessEnter();
			return TRUE;
		}
		case KEY_F4:
		case KEY_CTRLA: {
			if (SetCurPath())
				ShellSetFileAttributes(this);

			return TRUE;
		}
		case KEY_CTRLR: {
			ReadTree();

			if (TreeCount > 0)
				SyncDir();

			Redraw();
			break;
		}
		case KEY_SHIFTF5:
		case KEY_SHIFTF6: {
			if (SetCurPath()) {
				int ToPlugin = 0;
				ShellCopy ShCopy(this, Key == KEY_SHIFTF6, FALSE, TRUE, TRUE, ToPlugin, nullptr);
			}

			return TRUE;
		}
		case KEY_F5:
		case KEY_DRAGCOPY:
		case KEY_F6:
		case KEY_ALTF6:
		case KEY_DRAGMOVE: {
			if (SetCurPath() && TreeCount > 0) {
				Panel *AnotherPanel = CtrlObject->Cp()->GetAnotherPanel(this);
				int Ask = ((Key != KEY_DRAGCOPY && Key != KEY_DRAGMOVE) || Opt.Confirm.Drag);
				int Move = (Key == KEY_F6 || Key == KEY_DRAGMOVE);
				int ToPlugin = AnotherPanel->GetMode() == PLUGIN_PANEL && AnotherPanel->IsVisible()
						&& !CtrlObject->Plugins.UseFarCommand(AnotherPanel->GetPluginHandle(),
								PLUGIN_FARPUTFILES);
				int Link = (Key == KEY_ALTF6 && !ToPlugin);

				if (Key == KEY_ALTF6 && !Link)	// молча отвалим :-)
					return TRUE;

				{
					ShellCopy ShCopy(this, Move, Link, FALSE, Ask, ToPlugin, nullptr);
				}

				if (ToPlugin == 1) {
					PluginPanelItem *ItemList = new PluginPanelItem[1];
					int ItemNumber = 1;
					HANDLE hAnotherPlugin = AnotherPanel->GetPluginHandle();
					FileList::FileNameToPluginItem(ListData[CurFile]->strName, ItemList);
					int PutCode = CtrlObject->Plugins.PutFiles(hAnotherPlugin, ItemList, ItemNumber, Move, 0);

					if (PutCode == 1 || PutCode == 2)
						AnotherPanel->SetPluginModified();

					if (ItemList)
						delete[] ItemList;

					if (Move)
						ReadSubTree(ListData[CurFile]->strName);

					Update(0);
					Redraw();
					AnotherPanel->Update(UPDATE_KEEP_SELECTION);
					AnotherPanel->Redraw();
				}
			}

			return TRUE;
		}
		case KEY_F7: {
			if (SetCurPath())
				ShellMakeDir(this);

			return TRUE;
		}
		/*
				Удаление                               Shift-Del, Shift-F8, F8

				Удаление файлов и папок. F8 и Shift-Del удаляют все выбранные
			файлы, Shift-F8 - только файл под курсором. Shift-Del всегда
			удаляет файлы, не используя Корзину (Recycle Bin).
			Использование Корзины командами F8 и Shift-F8
			зависит от конфигурации.

			Уничтожение файлов и папок                                 Alt-Del
		*/
		case KEY_F8:
		case KEY_SHIFTDEL:
		case KEY_SHIFTNUMDEL:
		case KEY_SHIFTDECIMAL:
		case KEY_ALTNUMDEL:
		case KEY_ALTDECIMAL:
		case KEY_ALTDEL: {
			if (SetCurPath()) {
				int SaveOpt = Opt.DeleteToRecycleBin;

				if (Key == KEY_SHIFTDEL || Key == KEY_SHIFTNUMDEL || Key == KEY_SHIFTDECIMAL)
					Opt.DeleteToRecycleBin = 0;

				ShellDelete(this, Key == KEY_ALTDEL || Key == KEY_ALTNUMDEL || Key == KEY_ALTDECIMAL);
				// Надобно не забыть обновить противоположную панель...
				Panel *AnotherPanel = CtrlObject->Cp()->GetAnotherPanel(this);
				AnotherPanel->Update(UPDATE_KEEP_SELECTION);
				AnotherPanel->Redraw();
				Opt.DeleteToRecycleBin = SaveOpt;

				if (Opt.Tree.AutoChangeFolder && !ModalMode)
					ProcessKey(KEY_ENTER);
			}

			return TRUE;
		}
		case KEY_MSWHEEL_UP:
		case (KEY_MSWHEEL_UP | KEY_ALT): {
			Scroll(Key & KEY_ALT ? -1 : -Opt.MsWheelDelta);
			return TRUE;
		}
		case KEY_MSWHEEL_DOWN:
		case (KEY_MSWHEEL_DOWN | KEY_ALT): {
			Scroll(Key & KEY_ALT ? 1 : Opt.MsWheelDelta);
			return TRUE;
		}
		case KEY_MSWHEEL_LEFT:
		case (KEY_MSWHEEL_LEFT | KEY_ALT): {
			int Roll = Key & KEY_ALT ? 1 : Opt.MsHWheelDelta;

			for (int i = 0; i < Roll; i++)
				ProcessKey(KEY_LEFT);

			return TRUE;
		}
		case KEY_MSWHEEL_RIGHT:
		case (KEY_MSWHEEL_RIGHT | KEY_ALT): {
			int Roll = Key & KEY_ALT ? 1 : Opt.MsHWheelDelta;

			for (int i = 0; i < Roll; i++)
				ProcessKey(KEY_RIGHT);

			return TRUE;
		}
		case KEY_HOME:
		case KEY_NUMPAD7: {
			Up(0x7fffff);

			if (Opt.Tree.AutoChangeFolder && !ModalMode)
				ProcessKey(KEY_ENTER);

			return TRUE;
		}
		case KEY_ADD:		// OFM: Gray+/Gray- navigation
		{
			CurFile = GetNextNavPos();

			if (Opt.Tree.AutoChangeFolder && !ModalMode)
				ProcessKey(KEY_ENTER);
			else
				DisplayTree(TRUE);

			return TRUE;
		}
		case KEY_SUBTRACT:		// OFM: Gray+/Gray- navigation
		{
			CurFile = GetPrevNavPos();

			if (Opt.Tree.AutoChangeFolder && !ModalMode)
				ProcessKey(KEY_ENTER);
			else
				DisplayTree(TRUE);

			return TRUE;
		}
		case KEY_END:
		case KEY_NUMPAD1: {
			Down(0x7fffff);

			if (Opt.Tree.AutoChangeFolder && !ModalMode)
				ProcessKey(KEY_ENTER);

			return TRUE;
		}
		case KEY_UP:
		case KEY_NUMPAD8: {
			Up(1);

			if (Opt.Tree.AutoChangeFolder && !ModalMode)
				ProcessKey(KEY_ENTER);

			return TRUE;
		}
		case KEY_DOWN:
		case KEY_NUMPAD2: {
			Down(1);

			if (Opt.Tree.AutoChangeFolder && !ModalMode)
				ProcessKey(KEY_ENTER);

			return TRUE;
		}
		case KEY_PGUP:
		case KEY_NUMPAD9: {
			CurTopFile-= Y2 - Y1 - 3 - ModalMode;
			CurFile-= Y2 - Y1 - 3 - ModalMode;
			DisplayTree(TRUE);

			if (Opt.Tree.AutoChangeFolder && !ModalMode)
				ProcessKey(KEY_ENTER);

			return TRUE;
		}
		case KEY_PGDN:
		case KEY_NUMPAD3: {
			CurTopFile+= Y2 - Y1 - 3 - ModalMode;
			CurFile+= Y2 - Y1 - 3 - ModalMode;
			DisplayTree(TRUE);

			if (Opt.Tree.AutoChangeFolder && !ModalMode)
				ProcessKey(KEY_ENTER);

			return TRUE;
		}
		default:

			if ((Key >= KEY_ALT_BASE + 0x01 && Key <= KEY_ALT_BASE + 255)
					|| (Key >= KEY_ALTSHIFT_BASE + 0x01 && Key <= KEY_ALTSHIFT_BASE + 255)) {
				FastFind(Key);

				if (Opt.Tree.AutoChangeFolder && !ModalMode)
					ProcessKey(KEY_ENTER);
			} else
				break;

			return TRUE;
	}

	return FALSE;
}

int TreeList::GetNextNavPos()
{
	int NextPos = CurFile;

	if (CurFile + 1 < TreeCount) {
		int CurDepth = ListData[CurFile]->Depth;

		for (int I = CurFile + 1; I < TreeCount; ++I)
			if (ListData[I]->Depth == CurDepth) {
				NextPos = I;
				break;
			}
	}

	return NextPos;
}

int TreeList::GetPrevNavPos()
{
	int PrevPos = CurFile;

	if (CurFile - 1 > 0) {
		int CurDepth = ListData[CurFile]->Depth;

		for (int I = CurFile - 1; I > 0; --I)
			if (ListData[I]->Depth == CurDepth) {
				PrevPos = I;
				break;
			}
	}

	return PrevPos;
}

void TreeList::Up(int Count)
{
	CurFile-= Count;
	DisplayTree(TRUE);
}

void TreeList::Down(int Count)
{
	CurFile+= Count;
	DisplayTree(TRUE);
}

void TreeList::Scroll(int Count)
{
	CurFile+= Count;
	CurTopFile+= Count;
	DisplayTree(TRUE);
}

void TreeList::CorrectPosition()
{
	if (!TreeCount) {
		CurFile = CurTopFile = 0;
		return;
	}

	int Height = Y2 - Y1 - 3 - (ModalMode);

	if (CurTopFile + Height > TreeCount)
		CurTopFile = TreeCount - Height;

	if (CurFile < 0)
		CurFile = 0;

	if (CurFile > TreeCount - 1)
		CurFile = TreeCount - 1;

	if (CurTopFile < 0)
		CurTopFile = 0;

	if (CurTopFile > TreeCount - 1)
		CurTopFile = TreeCount - 1;

	if (CurFile < CurTopFile)
		CurTopFile = CurFile;

	if (CurFile > CurTopFile + Height - 1)
		CurTopFile = CurFile - (Height - 1);
}

BOOL TreeList::SetCurDir(const wchar_t *NewDir, int ClosePlugin)
{
	if (!TreeCount)
		Update(0);

	if (TreeCount > 0 && !SetDirPosition(NewDir)) {
		Update(0);
		SetDirPosition(NewDir);
	}

	if (GetFocus()) {
		CtrlObject->CmdLine->SetCurDir(NewDir);
		CtrlObject->CmdLine->Show();
	}

	return TRUE;	//???
}

int TreeList::SetDirPosition(const wchar_t *NewDir)
{
	long I;

	for (I = 0; I < TreeCount; I++) {
		if (!StrCmpI(NewDir, ListData[I]->strName)) {
			WorkDir = CurFile = I;
			CurTopFile = CurFile - (Y2 - Y1 - 1) / 2;
			CorrectPosition();
			return TRUE;
		}
	}

	return FALSE;
}

int TreeList::GetCurDir(FARString &strCurDir)
{
	if (!TreeCount) {
		if (ModalMode == MODALTREE_FREE)
			strCurDir = strRoot;
		else
			strCurDir.Clear();
	} else
		strCurDir = ListData[CurFile]->strName;		// BUGBUG

	return (int)strCurDir.GetLength();
}

int TreeList::ProcessMouse(MOUSE_EVENT_RECORD *MouseEvent)
{
	int OldFile = CurFile;
	int RetCode;

	if (Opt.ShowPanelScrollbar && MouseX == X2 && (MouseEvent->dwButtonState & 1) && !IsDragging()) {
		int ScrollY = Y1 + 1;
		int Height = Y2 - Y1 - 3;

		if (MouseY == ScrollY) {
			while (IsMouseButtonPressed())
				ProcessKey(KEY_UP);

			if (!ModalMode)
				SetFocus();

			return TRUE;
		}

		if (MouseY == ScrollY + Height - 1) {
			while (IsMouseButtonPressed())
				ProcessKey(KEY_DOWN);

			if (!ModalMode)
				SetFocus();

			return TRUE;
		}

		if (MouseY > ScrollY && MouseY < ScrollY + Height - 1 && Height > 2) {
			CurFile = (TreeCount - 1) * (MouseY - ScrollY) / (Height - 2);
			DisplayTree(TRUE);

			if (!ModalMode)
				SetFocus();

			return TRUE;
		}
	}

	if (Panel::PanelProcessMouse(MouseEvent, RetCode))
		return (RetCode);

	if (MouseEvent->dwMousePosition.Y > Y1 && MouseEvent->dwMousePosition.Y < Y2 - 2) {
		if (!ModalMode)
			SetFocus();

		MoveToMouse(MouseEvent);
		DisplayTree(TRUE);

		if (!TreeCount)
			return TRUE;

		if (((MouseEvent->dwButtonState & FROM_LEFT_1ST_BUTTON_PRESSED)
					&& MouseEvent->dwEventFlags == DOUBLE_CLICK)
				|| ((MouseEvent->dwButtonState & RIGHTMOST_BUTTON_PRESSED) && !MouseEvent->dwEventFlags)
				|| (OldFile != CurFile && Opt.Tree.AutoChangeFolder && !ModalMode)) {
			ProcessEnter();
			return TRUE;
		}

		return TRUE;
	}

	if (MouseEvent->dwMousePosition.Y <= Y1 + 1) {
		if (!ModalMode)
			SetFocus();

		if (!TreeCount)
			return TRUE;

		while (IsMouseButtonPressed() && MouseY <= Y1 + 1)
			Up(1);

		if (Opt.Tree.AutoChangeFolder && !ModalMode)
			ProcessKey(KEY_ENTER);

		return TRUE;
	}

	if (MouseEvent->dwMousePosition.Y >= Y2 - 2) {
		if (!ModalMode)
			SetFocus();

		if (!TreeCount)
			return TRUE;

		while (IsMouseButtonPressed() && MouseY >= Y2 - 2)
			Down(1);

		if (Opt.Tree.AutoChangeFolder && !ModalMode)
			ProcessKey(KEY_ENTER);

		return TRUE;
	}

	return FALSE;
}

void TreeList::MoveToMouse(MOUSE_EVENT_RECORD *MouseEvent)
{
	CurFile = CurTopFile + MouseEvent->dwMousePosition.Y - Y1 - 1;
	CorrectPosition();
}

void TreeList::ProcessEnter()
{
	TreeItem *CurPtr;
	DWORD Attr;
	CurPtr = ListData[CurFile];

	if ((Attr = apiGetFileAttributes(CurPtr->strName)) != INVALID_FILE_ATTRIBUTES
			&& (Attr & FILE_ATTRIBUTE_DIRECTORY)) {
		if (!ModalMode && FarChDir(CurPtr->strName)) {
			Panel *AnotherPanel = GetRootPanel();
			SetCurDir(CurPtr->strName, TRUE);
			Show();
			AnotherPanel->SetCurDir(CurPtr->strName, TRUE);
			AnotherPanel->Redraw();
		}
	} else {
		DelTreeName(CurPtr->strName);
		Update(UPDATE_KEEP_SELECTION);
		Show();
	}
}

int TreeList::ReadTreeFile()
{
	size_t RootLength = strRoot.IsEmpty() ? 0 : strRoot.GetLength() - 1;
	FARString strName;
	// SaveState();
	FlushCache();
	MkTreeFileName(strRoot, strName);

	File TreeFile;
	if (MustBeCached(strRoot)
			|| (!TreeFile.Open(strName, FILE_READ_DATA, FILE_SHARE_READ, nullptr, OPEN_EXISTING))) {
		if (!GetCacheTreeName(strRoot, strName, FALSE)
				|| (!TreeFile.Open(strName, FILE_READ_DATA, FILE_SHARE_READ, nullptr, OPEN_EXISTING))) {
			// RestoreState();
			return FALSE;
		}
	}

	if (ListData) {
		for (long i = 0; i < TreeCount; i++)
			delete ListData[i];

		free(ListData);
	}

	ListData = nullptr;
	TreeCount = 0;
	{
		FARString strLastDirName;
		GetFileString GetStr(TreeFile);
		LPWSTR Record = nullptr;
		int RecordLength = 0;
		while (GetStr.GetString(&Record, CP_WIDE_LE, RecordLength) > 0) {
			FARString strDirName(strRoot, RootLength);
			strDirName.Append(Record, RecordLength);
			if (!IsSlash(*Record) || !StrCmpI(strDirName, strLastDirName)) {
				continue;
			}

			strLastDirName = strDirName;
			size_t Pos = 0;
			if (strDirName.Pos(Pos, L'\n')) {
				strDirName.Truncate(Pos);
			}

			if (RootLength > 0 && strDirName.At(RootLength - 1) != L':' && IsSlash(strDirName.At(RootLength))
					&& !strDirName.At(RootLength + 1)) {
				strDirName.Truncate(RootLength);
			}

			if (!(TreeCount & 255)) {
				TreeItem **TmpListData =
						(TreeItem **)realloc(ListData, (TreeCount + 256 + 1) * sizeof(TreeItem *));

				if (!TmpListData) {
					if (ListData) {
						for (long i = 0; i < TreeCount; i++)
							delete ListData[i];

						free(ListData);
					}

					ListData = nullptr;
					TreeCount = 0;
					TreeFile.Close();
					// RestoreState();
					return FALSE;
				}

				ListData = TmpListData;
			}

			ListData[TreeCount] = new TreeItem;
			ListData[TreeCount]->Clear();
			ListData[TreeCount]->strName = strDirName;
			TreeCount++;
		}
	}

	TreeFile.Close();

	if (!TreeCount)
		return FALSE;

	NumericSort = FALSE;
	CaseSensitiveSort = FALSE;
	far_qsort(TreeCache.ListName, TreeCache.TreeCount, sizeof(wchar_t *), SortCacheList);
	return FillLastData();
}

bool TreeList::FindPartName(const wchar_t *Name, int Next, int Direct, int ExcludeSets)
{
	FARString strMask;
	strMask = Name;
	strMask+= L"*";

	if (ExcludeSets) {
		ReplaceStrings(strMask, L"[", L"<[%>", -1, 1);
		ReplaceStrings(strMask, L"]", L"[]]", -1, 1);
		ReplaceStrings(strMask, L"<[%>", L"[[]", -1, 1);
	}

	for (int i = CurFile + (Next ? Direct : 0); i >= 0 && i < TreeCount; i+= Direct) {
		if (CmpName(strMask, ListData[i]->strName, true)) {
			CurFile = i;
			CurTopFile = CurFile - (Y2 - Y1 - 1) / 2;
			DisplayTree(TRUE);
			return true;
		}
	}

	for (int i = (Direct > 0) ? 0 : TreeCount - 1; (Direct > 0) ? i < CurFile : i > CurFile; i+= Direct) {
		if (CmpName(strMask, ListData[i]->strName, true)) {
			CurFile = i;
			CurTopFile = CurFile - (Y2 - Y1 - 1) / 2;
			DisplayTree(TRUE);
			return true;
		}
	}

	return false;
}

int TreeList::GetSelCount()
{
	return 1;
}

int TreeList::GetSelName(FARString *strName, DWORD &FileAttr, DWORD &FileMode, FAR_FIND_DATA_EX *fd)
{
	FileMode = 0640;

	if (!strName) {
		GetSelPosition = 0;
		return TRUE;
	}

	if (!GetSelPosition) {
		GetCurDir(*strName);

		FileAttr = FILE_ATTRIBUTE_DIRECTORY;
		FileMode|= S_IFDIR | S_IXUSR | S_IXGRP | S_IXOTH;
		GetSelPosition++;
		return TRUE;
	}

	GetSelPosition = 0;
	return FALSE;
}

int TreeList::GetCurName(FARString &strName)
{
	if (!TreeCount) {
		strName.Clear();
		return FALSE;
	}

	strName = ListData[CurFile]->strName;
	return TRUE;
}

void TreeList::AddTreeName(const wchar_t *Name)
{
	if (!*Name)
		return;

	FARString strFullName;
	ConvertNameToFull(Name, strFullName);
	Name = strFullName;

	if (!LastSlash(Name))
		return;

	ReadCache(strFullName);

	for (long CachePos = 0; CachePos < TreeCache.TreeCount; CachePos++) {
		int Result = StrCmpI(TreeCache.ListName[CachePos], Name);

		if (!Result)
			break;

		if (Result > 0) {
			TreeCache.Insert(CachePos, Name);
			break;
		}
	}
}

void TreeList::DelTreeName(const wchar_t *Name)
{
	if (!*Name)
		return;

	FARString strFullName;
	ConvertNameToFull(Name, strFullName);
	Name = strFullName;
	ReadCache(strFullName);

	for (long CachePos = 0; CachePos < TreeCache.TreeCount; CachePos++) {
		const wchar_t *wszDirName = TreeCache.ListName[CachePos];
		int Length = StrLength(Name);
		int DirLength = StrLength(wszDirName);

		if (DirLength < Length)
			continue;

		if (!StrCmpNI(Name, wszDirName, Length) && (!wszDirName[Length] || IsSlash(wszDirName[Length]))) {
			TreeCache.Delete(CachePos);
			CachePos--;
		}
	}
}

void TreeList::RenTreeName(const wchar_t *SrcName, const wchar_t *DestName)
{
	if (!*SrcName || !*DestName)
		return;

	FARString SrcNameFull, DestNameFull;
	ConvertNameToFull(SrcName, SrcNameFull);
	ConvertNameToFull(DestName, DestNameFull);
	ReadCache(SrcNameFull);
	int SrcLength = StrLength(SrcName);

	for (int CachePos = 0; CachePos < TreeCache.TreeCount; CachePos++) {
		const wchar_t *DirName = TreeCache.ListName[CachePos];

		if (!StrCmpNI(SrcName, DirName, SrcLength) && (!DirName[SrcLength] || IsSlash(DirName[SrcLength]))) {
			FARString strNewName = DestName;
			strNewName+= DirName + SrcLength;

			if (TreeCache.ListName[CachePos])
				free(TreeCache.ListName[CachePos]);

			TreeCache.ListName[CachePos] = wcsdup(strNewName);
		}
	}
}

void TreeList::ReadSubTree(const wchar_t *Path)
{
	ChangePriority ChPriority(ChangePriority::NORMAL);
	// SaveScreen SaveScr;
	TPreRedrawFuncGuard preRedrawFuncGuard(TreeList::PR_MsgReadTree);
	ScanTree ScTree(FALSE);
	FAR_FIND_DATA_EX fdata;
	FARString strDirName;
	FARString strFullName;
	int Count = 0;
	DWORD FileAttr;

	if ((FileAttr = apiGetFileAttributes(Path)) == INVALID_FILE_ATTRIBUTES
			|| !(FileAttr & FILE_ATTRIBUTE_DIRECTORY))
		return;

	ConvertNameToFull(Path, strDirName);
	AddTreeName(strDirName);
	int FirstCall = TRUE, AscAbort = FALSE;
	ScTree.SetFindPath(strDirName, L"*", 0);
	LastScrX = ScrX;
	LastScrY = ScrY;

	while (ScTree.GetNextName(&fdata, strFullName)) {
		if (fdata.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) {
			TreeList::MsgReadTree(Count + 1, FirstCall);

			if (CheckForEscSilent()) {
				AscAbort = ConfirmAbortOp();
				FirstCall = TRUE;
			}

			if (AscAbort)
				break;

			AddTreeName(strFullName);
			++Count;
		}
	}
}

void TreeList::ClearCache(int EnableFreeMem)
{
	TreeCache.Clean();
}

void TreeList::ReadCache(const wchar_t *TreeRoot)
{
	FARString strTreeName;
	FILE *TreeFile = nullptr;

	if (!StrCmp(MkTreeFileName(TreeRoot, strTreeName), TreeCache.strTreeName))
		return;

	if (TreeCache.TreeCount)
		FlushCache();

	if (MustBeCached(TreeRoot) || !(TreeFile = fopen(Wide2MB(strTreeName).c_str(), FOPEN_READ)))
		if (!GetCacheTreeName(TreeRoot, strTreeName, FALSE)
				|| !(TreeFile = fopen(Wide2MB(strTreeName).c_str(), FOPEN_READ))) {
			ClearCache(1);
			return;
		}

	TreeCache.strTreeName = strTreeName;
	wchar_t *DirName = new (std::nothrow) wchar_t[NT_MAX_PATH];

	if (DirName) {
		while (fgetws(DirName, NT_MAX_PATH, TreeFile)) {
			if (!IsSlash(*DirName))
				continue;

			wchar_t *ChPtr = wcschr(DirName, L'\n');

			if (ChPtr)
				*ChPtr = 0;

			TreeCache.Add(DirName);
		}

		delete[] DirName;
	}

	fclose(TreeFile);
}

void TreeList::FlushCache()
{
	FILE *TreeFile;

	if (!TreeCache.strTreeName.IsEmpty()) {
		DWORD FileAttributes = apiGetFileAttributes(TreeCache.strTreeName);

		if (FileAttributes != INVALID_FILE_ATTRIBUTES)
			apiSetFileAttributes(TreeCache.strTreeName, FILE_ATTRIBUTE_NORMAL);

		if (!(TreeFile = fopen(Wide2MB(TreeCache.strTreeName).c_str(), FOPEN_WRITE))) {
			ClearCache(1);
			return;
		}

		far_qsort(TreeCache.ListName, TreeCache.TreeCount, sizeof(wchar_t *), SortCacheList);

		bool SaveFailed = false;
		for (int i = 0; i < TreeCache.TreeCount; i++) {
			if (fwprintf(TreeFile, L"%ls\n", TreeCache.ListName[i]) < 0)
				SaveFailed = true;
		}

		if (fflush(TreeFile) == EOF)
			SaveFailed = true;

		fclose(TreeFile);

		if (SaveFailed) {
			apiDeleteFile(TreeCache.strTreeName);
			Message(MSG_WARNING | MSG_ERRORTYPE, 1, Msg::Error, Msg::CannotSaveTree, TreeCache.strTreeName,
					Msg::Ok);
		} else if (FileAttributes != INVALID_FILE_ATTRIBUTES)	// вернем атрибуты (если получится :-)
			apiSetFileAttributes(TreeCache.strTreeName, FileAttributes);
	}

	ClearCache(1);
}

void TreeList::UpdateViewPanel()
{
	if (!ModalMode) {
		Panel *AnotherPanel = GetRootPanel();
		FARString strCurName;
		GetCurDir(strCurName);

		if (AnotherPanel->GetType() == QVIEW_PANEL && SetCurPath())
			((QuickView *)AnotherPanel)->ShowFile(strCurName, FALSE, nullptr);
	}
}

bool TreeList::GoToFile(long idxItem)
{
	if ((DWORD)idxItem < (DWORD)TreeCount) {
		CurFile = idxItem;
		CorrectPosition();
		return true;
	}

	return false;
}

bool TreeList::GoToFile(const wchar_t *Name, BOOL OnlyPartName)
{
	return GoToFile(FindFile(Name, OnlyPartName));
}

long TreeList::FindFile(const wchar_t *Name, BOOL OnlyPartName)
{
	for (long I = 0; I < TreeCount; I++) {
		const wchar_t *CurPtrName =
				OnlyPartName ? PointToName(ListData[I]->strName) : ListData[I]->strName.CPtr();

		if (!StrCmp(Name, CurPtrName))
			return I;

		if (!StrCmpI(Name, CurPtrName))
			return I;
	}

	return -1;
}

long TreeList::FindFirst(const wchar_t *Name)
{
	return FindNext(0, Name);
}

long TreeList::FindNext(int StartPos, const wchar_t *Name)
{
	if ((DWORD)StartPos < (DWORD)TreeCount) {
		for (int I = StartPos; I < TreeCount; I++) {
			if (CmpName(Name, ListData[I]->strName, true))
				if (!TestParentFolderName(ListData[I]->strName))
					return I;
		}
	}

	return -1;
}

int TreeList::GetFileName(FARString &strName, int Pos, DWORD &FileAttr)
{
	if (Pos < 0 || Pos >= TreeCount)
		return FALSE;

	strName = ListData[Pos]->strName;
	FileAttr = FILE_ATTRIBUTE_DIRECTORY | apiGetFileAttributes(ListData[Pos]->strName);
	return TRUE;
}

int _cdecl SortList(const void *el1, const void *el2)
{
	return TreeCmp(((TreeItem **)el1)[0]->strName, ((TreeItem **)el2)[0]->strName, StaticSortNumeric,
			StaticSortCaseSensitive);
}

int _cdecl SortCacheList(const void *el1, const void *el2)
{
	return TreeCmp(*(wchar_t **)el1, *(wchar_t **)el2, StaticSortNumeric, 0);
}

int TreeCmp(const wchar_t *Str1, const wchar_t *Str2, int Numeric, int CaseSensitive)
{
	typedef int(__cdecl * CMPFUNC)(const wchar_t *, int, const wchar_t *, int);
	static CMPFUNC funcs[2][2] = {
		{StrCmpNN,   StrCmpNNI  },
		{NumStrCmpN, NumStrCmpNI}
	};
	CMPFUNC cmpfunc = funcs[Numeric ? 1 : 0][CaseSensitive ? 0 : 1];

	if (*Str1 == GOOD_SLASH && *Str2 == GOOD_SLASH) {
		Str1++;
		Str2++;
	}

	const wchar_t *s1 = wcschr(Str1, GOOD_SLASH);
	const wchar_t *s2 = wcschr(Str2, GOOD_SLASH);

	while (s1 && s2) {
		int r = cmpfunc(Str1, static_cast<int>(s1 - Str1), Str2, static_cast<int>(s2 - Str2));

		if (r)
			return r;

		Str1 = s1 + 1;
		Str2 = s2 + 1;
		s1 = wcschr(Str1, GOOD_SLASH);
		s2 = wcschr(Str2, GOOD_SLASH);
	}

	if (s1 || s2) {
		int r = cmpfunc(Str1, s1 ? static_cast<int>(s1 - Str1) : -1, Str2,
				s2 ? static_cast<int>(s2 - Str2) : -1);

		if (r)
			return r;

		return s1 ? 1 : -1;
	}

	return cmpfunc(Str1, -1, Str2, -1);
}

/*
	$ 16.10.2000 tran
	функция, определяющаяя необходимость кеширования
	файла
*/
int TreeList::MustBeCached(const wchar_t *Root)
{
	UINT type;
	type = FAR_GetDriveType(Root);

	if (type == DRIVE_UNKNOWN || type == DRIVE_NO_ROOT_DIR || type == DRIVE_REMOVABLE
			|| IsDriveTypeCDROM(type)) {
		if (type == DRIVE_REMOVABLE) {
			if (Upper(Root[0]) == L'A' || Upper(Root[0]) == L'B')
				return FALSE;	// это дискеты
		}

		return TRUE;
		// кешируются CD, removable и неизвестно что :)
	}

	/*
		остались
		DRIVE_REMOTE
		DRIVE_RAMDISK
		DRIVE_FIXED
	*/
	return FALSE;
}

void TreeList::SetFocus()
{
	Panel::SetFocus();
	SetTitle();
	SetMacroMode(FALSE);
}

void TreeList::KillFocus()
{
	if (CurFile < TreeCount) {
		if (apiGetFileAttributes(ListData[CurFile]->strName) == INVALID_FILE_ATTRIBUTES) {
			DelTreeName(ListData[CurFile]->strName);
			Update(UPDATE_KEEP_SELECTION);
		}
	}

	Panel::KillFocus();
	SetMacroMode(TRUE);
}

void TreeList::SetMacroMode(int Restore)
{
	if (!CtrlObject)
		return;

	if (PrevMacroMode == -1)
		PrevMacroMode = CtrlObject->Macro.GetMode();

	CtrlObject->Macro.SetMode(Restore ? PrevMacroMode : MACRO_TREEPANEL);
}

BOOL TreeList::UpdateKeyBar()
{
	KeyBar *KB = CtrlObject->MainKeyBar;
	KB->SetAllGroup(KBL_MAIN, Msg::KBTreeF1, 12);
	KB->SetAllGroup(KBL_SHIFT, Msg::KBTreeShiftF1, 12);
	KB->SetAllGroup(KBL_ALT, Msg::KBTreeAltF1, 12);
	KB->SetAllGroup(KBL_CTRL, Msg::KBTreeCtrlF1, 12);
	KB->SetAllGroup(KBL_CTRLSHIFT, Msg::KBTreeCtrlShiftF1, 12);
	KB->SetAllGroup(KBL_CTRLALT, Msg::KBTreeCtrlAltF1, 12);
	KB->SetAllGroup(KBL_ALTSHIFT, Msg::KBTreeAltShiftF1, 12);
	KB->SetAllGroup(KBL_CTRLALTSHIFT, Msg::KBTreeCtrlAltShiftF1, 12);
	DynamicUpdateKeyBar();
	return TRUE;
}

void TreeList::DynamicUpdateKeyBar()
{
	KeyBar *KB = CtrlObject->MainKeyBar;
	KB->ReadRegGroup(L"Tree", Opt.strLanguage);
	KB->SetAllRegGroup();
}

void TreeList::SetTitle()
{
	if (GetFocus()) {
		FARString strTitleDir(L"{");

		const wchar_t *Ptr = ListData ? ListData[CurFile]->strName.CPtr() : L"";

		if (*Ptr) {
			strTitleDir+= Ptr;
			strTitleDir+= L" - ";
		}

		strTitleDir+= L"Tree}";

		ConsoleTitle::SetFarTitle(strTitleDir);
	}
}

/*
   "Local AppData" = [HKCU\Software\Microsoft\Windows\CurrentVersion\Explorer\Shell Folders]/Local AppData
   "AppData"       = [HKCU\Software\Microsoft\Windows\CurrentVersion\Explorer\Shell Folders]/AppData

*/

static void MkTreeName(FARString &out, const wchar_t *RootDir, const char *ext)
{
	struct stat s{};
	int r = sdc_stat(Wide2MB(RootDir).c_str(), &s);
	if (r == 0) {
		out = InMyTempFmt("tree/%llx-%llx.%s", (unsigned long long)s.st_rdev, (unsigned long long)s.st_ino, ext);
	} else {
		std::string tmp = InMyTemp("tree/wtf-");
		const std::string &RootMB = Wide2MB(RootDir);
		for (char c : RootMB) {
			tmp+= (c == GOOD_SLASH) ? '@' : c;
		}
		tmp+= '.';
		tmp+= ext;
		out = tmp;
	}
}

FARString &TreeList::MkTreeFileName(const wchar_t *RootDir, FARString &strDest)
{
	MkTreeName(strDest, RootDir, "far");
	return strDest;
}

FARString &TreeList::MkTreeCacheFolderName(const wchar_t *RootDir, FARString &strDest)
{
	MkTreeName(strDest, RootDir, "cache");
	return strDest;
}

/*
	Opt.Tree.LocalDisk
	Opt.Tree.NetDisk
	Opt.Tree.NetPath
	Opt.Tree.RemovableDisk
	Opt.Tree.CDROM
	Opt.Tree.SavedTreePath

		локальных дисков - "X.nnnnnnnn.tree"
		сетевых дисков - "X.nnnnnnnn.tree"
		сетевых путей - "Server.share.tree"
		сменных дисков(DRIVE_REMOVABLE) - "Far.nnnnnnnn.tree"
		сменных дисков(CD) - "Label.nnnnnnnn.tree"

*/
FARString &TreeList::CreateTreeFileName(const wchar_t *Path, FARString &strDest)
{
#if 0
	char RootPath[NM];
	RootPath = ExtractPathRoot(Path);
	UINT DriveType = FAR_GetDriveType(RootPath,nullptr,FALSE);
	// получение инфы о томе
	char VolumeName[NM],FileSystemName[NM];
	DWORD MaxNameLength,FileSystemFlags,VolumeNumber;

	if (!GetVolumeInformation(
			RootDir,VolumeName,sizeof(VolumeName),&VolumeNumber,
			&MaxNameLength,&FileSystemFlags,
			FileSystemName,sizeof(FileSystemName)
		)
	)
		Opt.Tree.SavedTreePath
#endif
	return strDest;
}

const void *TreeList::GetItem(int Index)
{
	if (Index == -1 || Index == -2)
		Index = GetCurrentPos();

	if (Index >= (int)TreeCount)
		return nullptr;

	return ListData[Index];
}

int TreeList::GetCurrentPos()
{
	return CurFile;
}

bool TreeList::SaveState()
{
	if (SaveListData)
		delete[] SaveListData;

	SaveListData = nullptr;
	SaveTreeCount = SaveWorkDir = 0;

	if (TreeCount > 0) {
		SaveListData = new (std::nothrow) TreeItem[TreeCount];

		if (SaveListData) {
			for (int i = 0; i < TreeCount; i++)
				SaveListData[i] = *ListData[i];

			SaveTreeCount = TreeCount;
			SaveWorkDir = WorkDir;
			TreeCache.Copy(&tempTreeCache);
			return true;
		}
	}

	return false;
}

bool TreeList::RestoreState()
{
	if (ListData) {
		for (long i = 0; i < TreeCount; i++)
			delete ListData[i];

		free(ListData);
	}

	TreeCount = WorkDir = 0;
	ListData = nullptr;

	if (SaveTreeCount > 0 && (ListData = (TreeItem **)malloc(SaveTreeCount * sizeof(TreeItem *)))) {
		for (int i = 0; i < SaveTreeCount; i++) {
			ListData[i] = new TreeItem;
			*ListData[i] = SaveListData[i];
		}

		TreeCount = SaveTreeCount;
		WorkDir = SaveWorkDir;
		tempTreeCache.Copy(&TreeCache);
		tempTreeCache.Clean();
		return true;
	}

	return false;
}
