/*
flshow.cpp

Файловая панель - вывод на экран
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

#include <errno.h>

#include "headers.hpp"

#include "filelist.hpp"
#include "colors.hpp"
#include "palette.hpp"
#include "lang.hpp"
#include "filefilter.hpp"
#include "cmdline.hpp"
#include "filepanels.hpp"
#include "ctrlobj.hpp"
#include "syslog.hpp"
#include "interf.hpp"
#include "config.hpp"
#include "datetime.hpp"
#include "pathmix.hpp"
#include "strmix.hpp"
#include "panelmix.hpp"

extern PanelViewSettings ViewSettingsArray[];
extern int ColumnTypeWidth[];

static wchar_t OutCharacter[16] = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};

static FarLangMsg __FormatEndSelectedPhrase(int Count)
{
	if (Count == 1)
		return Msg::ListFileSize;

	char StrItems[32];
	_itoa(Count, StrItems, 10);
	int LenItems = (int)strlen(StrItems);

	if (StrItems[LenItems - 1] == '1' && Count != 11)
		return Msg::ListFilesSize1;

	return Msg::ListFilesSize2;
}

void FileList::DisplayObject()
{
	Height = Y2 - Y1 - 4 + !Opt.ShowColumnTitles + (Opt.ShowPanelStatus ? 0 : 2);
	_OT(SysLog(L"[%p] FileList::DisplayObject()", this));

	if (UpdateRequired) {
		UpdateRequired = FALSE;
		Update(UpdateRequiredMode);
	}

	ProcessPluginCommand();
	ShowFileList(FALSE);
}

void FileList::ShowFileList(int Fast)
{
	if (Locked()) {
		CorrectPosition();
		return;
	}

	FARString strTitle;
	FARString strInfoCurDir;
	int Cells;
	OpenPluginInfo Info;

	if (PanelMode == PLUGIN_PANEL) {
		if (ProcessPluginEvent(FE_REDRAW, nullptr))
			return;

		CtrlObject->Plugins.GetOpenPluginInfo(hPlugin, &Info);
		strInfoCurDir = Info.CurDir;
	}

	int CurFullScreen = IsFullScreen();
	PrepareViewSettings(ViewMode, &Info);
	CorrectPosition();

	if (CurFullScreen != IsFullScreen()) {
		CtrlObject->Cp()->SetScreenPosition();
		CtrlObject->Cp()->GetAnotherPanel(this)->Update(UPDATE_KEEP_SELECTION | UPDATE_SECONDARY);
	}

	SetScreen(X1 + 1, Y1 + 1, X2 - 1, Y2 - 1, L' ', FarColorToReal(COL_PANELTEXT));
	Box(X1, Y1, X2, Y2, FarColorToReal(COL_PANELBOX), DOUBLE_BOX);

	if (Opt.ShowColumnTitles) {
		//	SetScreen(X1+1,Y1+1,X2-1,Y1+1,' ',COL_PANELTEXT);
		SetFarColor(COL_PANELTEXT);	//???
									// GotoXY(X1+1,Y1+1);
		// FS<<fmt::Expand(X2-X1-1)<<L"";
	}

	for (int I = 0, ColumnPos = X1 + 1; I < ViewSettings.ColumnCount; I++) {
		if (ViewSettings.ColumnWidth[I] < 0)
			continue;

		if (Opt.ShowColumnTitles) {
			FARString strTitle;
			FarLangMsg IDMessage{FARLANGMSGID_BAD};

			switch (ViewSettings.ColumnType[I] & 0xff) {
				case NAME_COLUMN:
					IDMessage = Msg::ColumnName;
					break;
				case SIZE_COLUMN:
					IDMessage = Msg::ColumnSize;
					break;
				case PHYSICAL_COLUMN:
					IDMessage = Msg::ColumnPhysical;
					break;
				case DATE_COLUMN:
					IDMessage = Msg::ColumnDate;
					break;
				case TIME_COLUMN:
					IDMessage = Msg::ColumnTime;
					break;
				case WDATE_COLUMN:
					IDMessage = Msg::ColumnWrited;
					break;
				case CDATE_COLUMN:
					IDMessage = Msg::ColumnCreated;
					break;
				case ADATE_COLUMN:
					IDMessage = Msg::ColumnAccessed;
					break;
				case CHDATE_COLUMN:
					IDMessage = Msg::ColumnChanged;
					break;
				case ATTR_COLUMN:
					IDMessage = Msg::ColumnAttr;
					break;
				case DIZ_COLUMN:
					IDMessage = Msg::ColumnDescription;
					break;
				case OWNER_COLUMN:
					IDMessage = Msg::ColumnOwner;
					break;
				case GROUP_COLUMN:
					IDMessage = Msg::ColumnGroup;
					break;
				case NUMLINK_COLUMN:
					IDMessage = Msg::ColumnMumLinks;
					break;
			}

			if (IDMessage != FARLANGMSGID_BAD)
				strTitle = IDMessage;

			if (PanelMode == PLUGIN_PANEL && Info.PanelModesArray && ViewMode < Info.PanelModesNumber
					&& Info.PanelModesArray[ViewMode].ColumnTitles) {
				const wchar_t *NewTitle = Info.PanelModesArray[ViewMode].ColumnTitles[I];

				if (NewTitle)
					strTitle = NewTitle;
			}

			FARString strTitleMsg;
			CenterStr(strTitle, strTitleMsg, ViewSettings.ColumnWidth[I]);
			SetFarColor(COL_PANELCOLUMNTITLE);
			GotoXY(ColumnPos, Y1 + 1);
			FS << fmt::Cells() << fmt::Truncate(ViewSettings.ColumnWidth[I]) << strTitleMsg;
		}

		if (I >= ViewSettings.ColumnCount - 1)
			break;

		if (ViewSettings.ColumnWidth[I + 1] < 0)
			continue;

		SetFarColor(COL_PANELBOX);
		ColumnPos+= ViewSettings.ColumnWidth[I];
		GotoXY(ColumnPos, Y1);
		BoxText(BoxSymbols[BS_T_H2V1]);

		if (Opt.ShowColumnTitles) {
			GotoXY(ColumnPos, Y1 + 1);
			BoxText(BoxSymbols[BS_V1]);
		}

		if (!Opt.ShowPanelStatus) {
			GotoXY(ColumnPos, Y2);
			BoxText(BoxSymbols[BS_B_H2V1]);
		}

		ColumnPos++;
	}

	int NextX1 = X1 + 1;

	if (Opt.ShowSortMode) {
		static int SortModes[] = {UNSORTED, BY_NAME, BY_EXT, BY_MTIME, BY_CTIME, BY_ATIME, BY_CHTIME, BY_SIZE,
				BY_DIZ, BY_OWNER, BY_PHYSICALSIZE, BY_NUMLINKS, BY_FULLNAME, BY_CUSTOMDATA};
		static FarLangMsg SortStrings[] = {Msg::MenuUnsorted, Msg::MenuSortByName, Msg::MenuSortByExt,
				Msg::MenuSortByWrite, Msg::MenuSortByCreation, Msg::MenuSortByAccess, Msg::MenuSortByChange,
				Msg::MenuSortBySize, Msg::MenuSortByDiz, Msg::MenuSortByOwner, Msg::MenuSortByPhysicalSize,
				Msg::MenuSortByNumLinks, Msg::MenuSortByFullName, Msg::MenuSortByCustomData};

		for (size_t I = 0; I < ARRAYSIZE(SortModes); I++) {
			if (SortModes[I] == SortMode) {
				const wchar_t *SortStr = SortStrings[I];
				const wchar_t *Ch = wcschr(SortStr, L'&');

				if (Ch) {
					if (Opt.ShowColumnTitles)
						GotoXY(NextX1, Y1 + 1);
					else
						GotoXY(NextX1, Y1);

					SetFarColor(COL_PANELCOLUMNTITLE);
					OutCharacter[0] = SortOrder == 1 ? Lower(Ch[1]) : Upper(Ch[1]);
					Text(OutCharacter);
					NextX1++;

					if (Filter && Filter->IsEnabledOnPanel()) {
						OutCharacter[0] = L'*';
						Text(OutCharacter);
						NextX1++;
					}
				}

				break;
			}
		}
	}

	if (!Opt.ShowHidden) {
		if (Opt.ShowColumnTitles)
			GotoXY(NextX1, Y1 + 1);
		else
			GotoXY(NextX1, Y1);

		SetFarColor(COL_PANELCOLUMNTITLE);
		OutCharacter[0] = L'h';
		Text(OutCharacter);
		NextX1++;
	}

	/* <режимы сортировки> */
	if (/*GetNumericSort() || GetCaseSensitiveSort() || GetSortGroups() || */ GetSelectedFirstMode()) {
		if (Opt.ShowColumnTitles)
			GotoXY(NextX1, Y1 + 1);
		else
			GotoXY(NextX1, Y1);

		SetFarColor(COL_PANELCOLUMNTITLE);
		wchar_t *PtrOutCharacter = OutCharacter;
		*PtrOutCharacter = 0;

		// if (GetSelectedFirstMode())
		*PtrOutCharacter++ = L'^';

		/*
			if(GetNumericSort())
				*PtrOutCharacter++=L'#';
			if(GetSortGroups())
				*PtrOutCharacter++=L'@';
		*/
		/*
		if(GetCaseSensitiveSort())
		{

		}
		*/
		*PtrOutCharacter = 0;
		Text(OutCharacter);
		PtrOutCharacter[1] = 0;
	}

	/* </режимы сортировки> */

	if (!Fast && GetFocus()) {
		if (PanelMode == PLUGIN_PANEL)
			CtrlObject->CmdLine->SetCurDir(Info.CurDir);
		else
			CtrlObject->CmdLine->SetCurDir(strCurDir);

		CtrlObject->CmdLine->Show();
	}

	int TitleX2 = Opt.Clock && !Opt.ShowMenuBar ? Min(ScrX - 4, X2) : X2;
	int TruncSize = TitleX2 - X1 - 3;

	if (!Opt.ShowColumnTitles && Opt.ShowSortMode && Filter && Filter->IsEnabledOnPanel())
		TruncSize-= 2;

	GetTitle(strTitle, TruncSize, 2);	//,(PanelMode==PLUGIN_PANEL?0:2));
	Cells = (int)strTitle.CellsCount();
	int ClockCorrection = FALSE;

	if ((Opt.Clock && !Opt.ShowMenuBar) && TitleX2 == ScrX - 4) {
		ClockCorrection = TRUE;
		TitleX2+= 4;
	}

	int TitleX = X1 + (TitleX2 - X1 + 1 - Cells) / 2;

	if (ClockCorrection) {
		int Overlap = TitleX + Cells - TitleX2 + 5;

		if (Overlap > 0)
			TitleX-= Overlap;
	}

	if (TitleX <= X1)
		TitleX = X1 + 1;

	SetFarColor(Focus ? COL_PANELSELECTEDTITLE : COL_PANELTITLE);
	GotoXY(TitleX, Y1);
	Text(strTitle);

	if (ListData.IsEmpty()) {
		SetScreen(X1 + 1, Y2 - 1, X2 - 1, Y2 - 1, L' ', FarColorToReal(COL_PANELTEXT));
		SetFarColor(COL_PANELTEXT);	//???
									// GotoXY(X1+1,Y2-1);
		// FS<<fmt::Expand(X2-X1-1)<<L"";
	}

	if (PanelMode == PLUGIN_PANEL && !ListData.IsEmpty() && (Info.Flags & OPIF_REALNAMES)) {
		if (!strInfoCurDir.IsEmpty()) {
			strCurDir = strInfoCurDir;
		} else {
			if (!TestParentFolderName(ListData[CurFile]->strName)) {
				strCurDir = ListData[CurFile]->strName;
				size_t pos;

				if (FindLastSlash(pos, strCurDir)) {
					if (pos) {
						if (strCurDir.At(pos - 1) != L':')
							strCurDir.Truncate(pos);
						else
							strCurDir.Truncate(pos + 1);
					}
				}
			} else {
				strCurDir = strOriginalCurDir;
			}
		}

		if (GetFocus()) {
			CtrlObject->CmdLine->SetCurDir(strCurDir);
			CtrlObject->CmdLine->Show();
		}
	}

	if ((Opt.ShowPanelTotals || Opt.ShowPanelFree) && (Opt.ShowPanelStatus || !SelFileCount)) {
		ShowTotalSize(Info);
	}

	ShowList(FALSE, 0);
	ShowSelectedSize();

	if (Opt.ShowPanelScrollbar) {
		SetFarColor(COL_PANELSCROLLBAR);
		ScrollBarEx(X2, Y1 + 1 + Opt.ShowColumnTitles, Height, Round(CurTopFile, Columns),
				Round(ListData.Count(), Columns));
	}

	ShowScreensCount();

	if (!ProcessingPluginCommand && LastCurFile != CurFile) {
		LastCurFile = CurFile;
		UpdateViewPanel();
	}

	if (PanelMode == PLUGIN_PANEL)
		CtrlObject->Cp()->RedrawKeyBar();
}

DWORD64 FileList::GetShowColor(int Position, int ColorType)
{
	DWORD64 ColorAttr = FarColorToReal(COL_PANELTEXT);
	const DWORD FarColor[] = {COL_PANELTEXT, COL_PANELSELECTEDTEXT, COL_PANELCURSOR, COL_PANELSELECTEDCURSOR};

	if (Position >= 0 && Position < ListData.Count()) {
		int Pos = HIGHLIGHTCOLOR_NORMAL;

		if (CurFile == Position && Focus && !ListData.IsEmpty()) {
			Pos = ListData[Position]->Selected
					? HIGHLIGHTCOLOR_SELECTEDUNDERCURSOR
					: HIGHLIGHTCOLOR_UNDERCURSOR;
		} else if (ListData[Position]->Selected)
			Pos = HIGHLIGHTCOLOR_SELECTED;

		ColorAttr = ListData[Position]->ColorsPtr->Color[ColorType][Pos];

		if (!ColorAttr || !Opt.Highlight)
			ColorAttr = FarColorToReal(FarColor[Pos]);
	}

//	return (4 << 4) + 15;
	return ColorAttr;
}

void FileList::SetShowColor(int Position, int ColorType)
{
	SetColor(GetShowColor(Position, ColorType));
}

void FileList::ShowSelectedSize()
{
	int Length;
	FARString strSelStr, strFormStr;

	if (Opt.ShowPanelStatus) {
		SetFarColor(COL_PANELBOX);
		DrawSeparator(Y2 - 2);

		for (int I = 0, ColumnPos = X1 + 1; I < ViewSettings.ColumnCount - 1; I++) {
			if (ViewSettings.ColumnWidth[I] < 0
					|| (I == ViewSettings.ColumnCount - 2 && ViewSettings.ColumnWidth[I + 1] < 0))
				continue;

			ColumnPos+= ViewSettings.ColumnWidth[I];
			GotoXY(ColumnPos, Y2 - 2);
			BoxText(BoxSymbols[BS_B_H1V1]);
			ColumnPos++;
		}
	}

	if (SelFileCount) {
		InsertCommas(SelFileSize, strFormStr);
		strSelStr.Format(__FormatEndSelectedPhrase(SelFileCount), strFormStr.CPtr(), SelFileCount);
		TruncStr(strSelStr, X2 - X1 - 1);
		Length = (int)strSelStr.GetLength();
		SetFarColor(COL_PANELSELECTEDINFO);
		GotoXY(X1 + (X2 - X1 + 1 - Length) / 2, Y2 - 2 * Opt.ShowPanelStatus);
		Text(strSelStr);
	}
}

void FileList::ShowTotalSize(OpenPluginInfo &Info)
{
	if (!Opt.ShowPanelTotals && PanelMode == PLUGIN_PANEL && !(Info.Flags & OPIF_REALNAMES))
		return;

	FARString strTotalStr, strFormSize, strFreeSize;
	int Length;
	InsertCommas(TotalFileSize, strFormSize);

	if (Opt.ShowPanelFree && (PanelMode != PLUGIN_PANEL || (Info.Flags & OPIF_REALNAMES)))
		InsertCommas(FreeDiskSize, strFreeSize);

	if (Opt.ShowPanelTotals) {
		if (!Opt.ShowPanelFree || strFreeSize.IsEmpty())
			strTotalStr.Format(__FormatEndSelectedPhrase(TotalFileCount), strFormSize.CPtr(), TotalFileCount);
		else {
			wchar_t DHLine[4] = {BoxSymbols[BS_H2], BoxSymbols[BS_H2], BoxSymbols[BS_H2], 0};
			strTotalStr.Format(L" %ls (%ld) %ls %ls ", strFormSize.CPtr(), TotalFileCount, DHLine,
					strFreeSize.CPtr());

			if ((int)strTotalStr.GetLength() > X2 - X1 - 1) {
				InsertCommas(FreeDiskSize >> 20, strFreeSize);
				InsertCommas(TotalFileSize >> 20, strFormSize);
				strTotalStr.Format(L" %ls %ls (%ld) %ls %ls %ls ", strFormSize.CPtr(), Msg::ListMb.CPtr(),
						TotalFileCount, DHLine, strFreeSize.CPtr(), Msg::ListMb.CPtr());
			}
		}
	} else
		strTotalStr.Format(Msg::ListFreeSize, !strFreeSize.IsEmpty() ? strFreeSize.CPtr() : L"???");

	SetFarColor(COL_PANELTOTALINFO);
	/*
		$ 01.08.2001 VVM
		+ Обрезаем строчку справа, а не слева
	*/
	TruncStrFromEnd(strTotalStr, X2 - X1 - 1);
	Length = (int)strTotalStr.GetLength();
	GotoXY(X1 + (X2 - X1 + 1 - Length) / 2, Y2);
	const wchar_t *FirstBox = wcschr(strTotalStr, BoxSymbols[BS_H2]);
	int BoxPos = FirstBox ? (int)(FirstBox - strTotalStr.CPtr()) : -1;
	int BoxLength = 0;

	if (BoxPos != -1)
		for (int I = 0; strTotalStr.At(BoxPos + I) == BoxSymbols[BS_H2]; I++)
			BoxLength++;

	if (BoxPos == -1 || !BoxLength)
		Text(strTotalStr);
	else {
		FS << fmt::Cells() << fmt::Truncate(BoxPos) << strTotalStr;
		SetFarColor(COL_PANELBOX);
		FS << fmt::Cells() << fmt::Truncate(BoxLength) << strTotalStr.CPtr() + BoxPos;
		SetFarColor(COL_PANELTOTALINFO);
		Text(strTotalStr.CPtr() + BoxPos + BoxLength);
	}
}

bool FileList::ResolveSymlink(FARString &target_path, const wchar_t *link_name, FileListItem *fi)
{
	std::wstring link_name_str(link_name);
	auto it = SymlinksCache.find(link_name_str);
	if (it != SymlinksCache.end()) {
		target_path = it->second;
		return true;
	}

	if (PanelMode == NORMAL_PANEL
			&& !strCurDir.IsEmpty()
			&& strCurDir[0] == GOOD_SLASH
			&& MixToFullPath(link_name, target_path, strCurDir)) {
		char buf[MAX_PATH + 1] = {0};
		ssize_t r = sdc_readlink(target_path.GetMB().c_str(), buf, ARRAYSIZE(buf) - 1);
		if (r <= 0 || r >= (ssize_t)ARRAYSIZE(buf)) {
			fprintf(stderr, "ResolveSymlink: sdc_readlink errno %u\n", errno);
			return false;
		}
		buf[r] = 0;
		target_path = buf;

	} else if (PanelMode == PLUGIN_PANEL) {
		PluginPanelItem pi;
		FileListToPluginItem(fi, &pi);
		if (!CtrlObject->Plugins.GetLinkTarget(hPlugin, &pi, target_path, 0)) {
			fprintf(stderr, "ResolveSymlink: GetLinkTarget failed\n");
			return false;
		}
	} else {
		return false;
	}
	SymlinksCache.emplace(link_name, target_path);
	return true;
}

int FileList::ConvertName(FARString &strDest, const wchar_t *SrcName, int MaxLength,
		int RightAlign, int ShowStatus, DWORD FileAttr, FileListItem *fi)
{
	if (ShowStatus && (FileAttr & FILE_ATTRIBUTE_REPARSE_POINT) != 0) {
		FARString strTemp;
		if (ResolveSymlink(strTemp, SrcName, fi)) {
			strTemp.Insert(0, L" ->");
			strTemp.Insert(0, SrcName);
			return ConvertName(strDest, strTemp, MaxLength, RightAlign, ShowStatus,
					FileAttr & (~(DWORD)FILE_ATTRIBUTE_REPARSE_POINT), fi);
		}
	}

	int SrcLength = StrLength(SrcName);
	int SrcVisualLength = StrCellsCount(SrcName, SrcLength);

	if (RightAlign && SrcVisualLength > MaxLength) {
		size_t SkipCells = SrcVisualLength - MaxLength;
		size_t SkipOfs = StrSizeOfCells(SrcName, SrcLength, SkipCells, true);
		strDest.Copy(SrcName + SkipOfs, SrcLength - SkipOfs);
		return TRUE;
	}

	const wchar_t *DotPtr;

	if (!ShowStatus
			&& ((!(FileAttr & FILE_ATTRIBUTE_DIRECTORY) && ViewSettings.AlignExtensions)
					|| ((FileAttr & FILE_ATTRIBUTE_DIRECTORY) && ViewSettings.FolderAlignExtensions))
			&& SrcVisualLength <= MaxLength && (DotPtr = wcsrchr(SrcName, L'.')) && DotPtr != SrcName
			&& (SrcName[0] != L'.' || SrcName[2]) && !wcschr(DotPtr + 1, L' ')) {
		int DotLength = StrLength(DotPtr + 1);
		int NameLength = DotLength ? (int)(DotPtr - SrcName) : SrcLength;
		int DotPos = MaxLength - Max(DotLength, 3);

		if (DotPos <= NameLength)
			DotPos = NameLength + 1;

		strDest.Copy(SrcName, NameLength);
		if (DotPos > 0 && NameLength > 0 && SrcName[NameLength - 1] == L' ') {
			strDest.Append(L'.');
			DotPos--;
		}
		if (DotPos > NameLength) {
			strDest.Append(L' ', DotPos - NameLength);
		}
		strDest.Append(DotPtr + 1, DotLength);
	} else {
		size_t CellsCount = MaxLength;
		size_t CopyLen = StrSizeOfCells(SrcName, SrcLength, CellsCount, false);
		strDest.Copy(SrcName, CopyLen);
	}

	const size_t CopiedCellsCount = strDest.CellsCount();
	if (CopiedCellsCount < size_t(MaxLength)) {
		strDest.Append(L' ', size_t(MaxLength - CopiedCellsCount));
	}

	return (SrcVisualLength > MaxLength);
}

void FileList::PrepareViewSettings(int ViewMode, OpenPluginInfo *PlugInfo)
{
	OpenPluginInfo Info = {0};

	if (PanelMode == PLUGIN_PANEL) {
		if (!PlugInfo)
			CtrlObject->Plugins.GetOpenPluginInfo(hPlugin, &Info);
		else
			Info = *PlugInfo;
	}

	ViewSettings = ViewSettingsArray[ViewMode];

	if (PanelMode == PLUGIN_PANEL) {
		if (Info.PanelModesArray && ViewMode < Info.PanelModesNumber
				&& Info.PanelModesArray[ViewMode].ColumnTypes
				&& Info.PanelModesArray[ViewMode].ColumnWidths) {
			TextToViewSettings(Info.PanelModesArray[ViewMode].ColumnTypes,
					Info.PanelModesArray[ViewMode].ColumnWidths, ViewSettings.ColumnType,
					ViewSettings.ColumnWidth, ViewSettings.ColumnWidthType, ViewSettings.ColumnCount);

			if (Info.PanelModesArray[ViewMode].StatusColumnTypes
					&& Info.PanelModesArray[ViewMode].StatusColumnWidths)
				TextToViewSettings(Info.PanelModesArray[ViewMode].StatusColumnTypes,
						Info.PanelModesArray[ViewMode].StatusColumnWidths, ViewSettings.StatusColumnType,
						ViewSettings.StatusColumnWidth, ViewSettings.StatusColumnWidthType,
						ViewSettings.StatusColumnCount);
			else if (Info.PanelModesArray[ViewMode].DetailedStatus) {
				ViewSettings.StatusColumnType[0] = COLUMN_RIGHTALIGN | NAME_COLUMN;
				ViewSettings.StatusColumnType[1] = SIZE_COLUMN;
				ViewSettings.StatusColumnType[2] = DATE_COLUMN;
				ViewSettings.StatusColumnType[3] = TIME_COLUMN;
				ViewSettings.StatusColumnWidth[0] = 0;
				ViewSettings.StatusColumnWidth[1] = 8;
				ViewSettings.StatusColumnWidth[2] = 0;
				ViewSettings.StatusColumnWidth[3] = 5;
				ViewSettings.StatusColumnCount = 4;
			} else {
				ViewSettings.StatusColumnType[0] = COLUMN_RIGHTALIGN | NAME_COLUMN;
				ViewSettings.StatusColumnWidth[0] = 0;
				ViewSettings.StatusColumnCount = 1;
			}

			ViewSettings.FullScreen = Info.PanelModesArray[ViewMode].FullScreen;
			ViewSettings.AlignExtensions = Info.PanelModesArray[ViewMode].AlignExtensions;

			if (!Info.PanelModesArray[ViewMode].CaseConversion) {
				ViewSettings.FolderUpperCase = 0;
				ViewSettings.FileLowerCase = 0;
				ViewSettings.FileUpperToLowerCase = 0;
			}
		} else
			for (int I = 0; I < ViewSettings.ColumnCount; I++)
				if ((ViewSettings.ColumnType[I] & 0xff) == NAME_COLUMN) {
					if (Info.Flags & OPIF_SHOWNAMESONLY)
						ViewSettings.ColumnType[I]|= COLUMN_NAMEONLY;

					if (Info.Flags & OPIF_SHOWRIGHTALIGNNAMES)
						ViewSettings.ColumnType[I]|= COLUMN_RIGHTALIGN;

					if (Info.Flags & OPIF_SHOWPRESERVECASE) {
						ViewSettings.FolderUpperCase = 0;
						ViewSettings.FileLowerCase = 0;
						ViewSettings.FileUpperToLowerCase = 0;
					}
				}
	}

	Columns = PreparePanelView(&ViewSettings);
	Height = Y2 - Y1 - 4;

	if (!Opt.ShowColumnTitles)
		Height++;

	if (!Opt.ShowPanelStatus)
		Height+= 2;
}

int FileList::PreparePanelView(PanelViewSettings *PanelView)
{
	PrepareColumnWidths(PanelView->StatusColumnType, PanelView->StatusColumnWidth,
			PanelView->StatusColumnWidthType, PanelView->StatusColumnCount, PanelView->FullScreen);
	return (PrepareColumnWidths(PanelView->ColumnType, PanelView->ColumnWidth, PanelView->ColumnWidthType,
			PanelView->ColumnCount, PanelView->FullScreen));
}

int FileList::PrepareColumnWidths(unsigned int *ColumnTypes, int *ColumnWidths, int *ColumnWidthsTypes,
		int &ColumnCount, int FullScreen)
{
	int TotalWidth, TotalPercentWidth, TotalPercentCount, ZeroLengthCount, EmptyColumns, I;
	ZeroLengthCount = EmptyColumns = 0;
	TotalWidth = ColumnCount - 1;
	TotalPercentCount = TotalPercentWidth = 0;

	for (I = 0; I < ColumnCount; I++) {
		if (ColumnWidths[I] < 0) {
			EmptyColumns++;
			continue;
		}

		int ColumnType = ColumnTypes[I] & 0xff;

		if (!ColumnWidths[I]) {
			ColumnWidthsTypes[I] = COUNT_WIDTH;		// manage all zero-width columns in same way
			ColumnWidths[I] = ColumnTypeWidth[ColumnType];

			if (ColumnType == WDATE_COLUMN || ColumnType == CDATE_COLUMN || ColumnType == ADATE_COLUMN
					|| ColumnType == CHDATE_COLUMN) {
				if (ColumnTypes[I] & COLUMN_BRIEF)
					ColumnWidths[I]-= 3;

				if (ColumnTypes[I] & COLUMN_MONTH)
					ColumnWidths[I]++;
			}
		}

		if (!ColumnWidths[I])
			ZeroLengthCount++;

		switch (ColumnWidthsTypes[I]) {
			case COUNT_WIDTH:
				TotalWidth+= ColumnWidths[I];
				break;
			case PERCENT_WIDTH:
				TotalPercentWidth+= ColumnWidths[I];
				TotalPercentCount++;
				break;
		}
	}

	TotalWidth-= EmptyColumns;
	int PanelTextWidth = X2 - X1 - 1;

	if (FullScreen)
		PanelTextWidth = ScrX - 1;

	int ExtraWidth = PanelTextWidth - TotalWidth;

	if (TotalPercentCount > 0) {
		int ExtraPercentWidth = (TotalPercentWidth > 100 || !ZeroLengthCount)
				? ExtraWidth
				: ExtraWidth * TotalPercentWidth / 100;
		int TempWidth = 0;

		for (I = 0; I < ColumnCount && TotalPercentCount > 0; I++)
			if (ColumnWidthsTypes[I] == PERCENT_WIDTH) {
				int PercentWidth = (TotalPercentCount > 1)
						? (ExtraPercentWidth * ColumnWidths[I] / TotalPercentWidth)
						: (ExtraPercentWidth - TempWidth);

				if (PercentWidth < 1)
					PercentWidth = 1;

				TempWidth+= PercentWidth;
				ColumnWidths[I] = PercentWidth;
				ColumnWidthsTypes[I] = COUNT_WIDTH;
				TotalPercentCount--;
			}

		ExtraWidth-= TempWidth;
	}

	for (I = 0; I < ColumnCount && ZeroLengthCount > 0; I++)
		if (!ColumnWidths[I]) {
			int AutoWidth = ExtraWidth / ZeroLengthCount;

			if (AutoWidth < 1)
				AutoWidth = 1;

			ColumnWidths[I] = AutoWidth;
			ExtraWidth-= AutoWidth;
			ZeroLengthCount--;
		}

	while (1) {
		int LastColumn = ColumnCount - 1;
		TotalWidth = LastColumn - EmptyColumns;

		for (I = 0; I < ColumnCount; I++)
			if (ColumnWidths[I] > 0)
				TotalWidth+= ColumnWidths[I];

		if (TotalWidth <= PanelTextWidth)
			break;

		if (ColumnCount <= 1) {
			ColumnWidths[0] = PanelTextWidth;
			break;
		} else if (PanelTextWidth >= TotalWidth - ColumnWidths[LastColumn]) {
			ColumnWidths[LastColumn] = PanelTextWidth - (TotalWidth - ColumnWidths[LastColumn]);
			break;
		} else
			ColumnCount--;
	}

	ColumnsInGlobal = 1;
	int GlobalColumns = 0;
	bool UnEqual;
	int Remainder;

	for (int i = 0; i < ViewSettings.ColumnCount; i++) {
		UnEqual = false;
		Remainder = ViewSettings.ColumnCount % ColumnsInGlobal;
		GlobalColumns = ViewSettings.ColumnCount / ColumnsInGlobal;

		if (!Remainder) {
			for (int k = 0; k < GlobalColumns - 1; k++) {
				for (int j = 0; j < ColumnsInGlobal; j++) {
					if ((ViewSettings.ColumnType[k * ColumnsInGlobal + j] & 0xFF)
							!= (ViewSettings.ColumnType[(k + 1) * ColumnsInGlobal + j] & 0xFF))
						UnEqual = true;
				}
			}

			if (!UnEqual)
				break;
		}

		ColumnsInGlobal++;
	}

	return (GlobalColumns);
}

extern void GetColor(int PaletteIndex);

static int MakeCurLeftPos(int ColumnWidth, const wchar_t *Str, int LeftPos, int &MaxLeftPos)
{
	int Cells = Str ? StrZCellsCount(Str) : 0;
	if (Cells < ColumnWidth || !Cells)
		return 0;

	if (LeftPos > Cells - ColumnWidth)
		LeftPos = Cells - ColumnWidth;

	size_t ng = LeftPos;
	int out = StrSizeOfCells(Str, wcslen(Str), ng, false);

	if (MaxLeftPos < (int)ng)
		MaxLeftPos = (int)ng;

	return out;
}

void FileList::ShowList(int ShowStatus, int StartColumn)
{
	int StatusShown = FALSE;
	int MaxLeftPos = 0, MinLeftPos = FALSE;
	int ColumnCount = ShowStatus ? ViewSettings.StatusColumnCount : ViewSettings.ColumnCount;

	for (int I = Y1 + 1 + Opt.ShowColumnTitles, J = CurTopFile; I < Y2 - 2 * Opt.ShowPanelStatus; I++, J++) {
		int CurColumn = StartColumn;

		if (ShowStatus) {
			SetFarColor(COL_PANELTEXT);
			GotoXY(X1 + 1, Y2 - 1);
		} else {
			SetShowColor(J);
			GotoXY(X1 + 1, I);
		}

		int StatusLine = FALSE;
		int Level = 1;

		for (int K = 0; K < ColumnCount; K++) {
			int ListPos = J + CurColumn * Height;

			if (ShowStatus) {
				if (CurFile != ListPos) {
					CurColumn++;
					continue;
				} else
					StatusLine = TRUE;
			}

			int CurX = WhereX();
			int CurY = WhereY();
			int ShowDivider = TRUE;
			unsigned int *ColumnTypes = ShowStatus ? ViewSettings.StatusColumnType : ViewSettings.ColumnType;
			int *ColumnWidths = ShowStatus ? ViewSettings.StatusColumnWidth : ViewSettings.ColumnWidth;
			int ColumnType = ColumnTypes[K] & 0xff;
			int ColumnWidth = ColumnWidths[K];

			if (ColumnWidth < 0) {
				if (!ShowStatus && K == ColumnCount - 1) {
					SetFarColor(COL_PANELBOX);
					GotoXY(CurX - 1, CurY);
					BoxText(CurX - 1 == X2 ? BoxSymbols[BS_V2] : L' ');
				}
				continue;
			}

			if (ListPos < ListData.Count()) {
				if (!ShowStatus && !StatusShown && CurFile == ListPos && Opt.ShowPanelStatus) {
					ShowList(TRUE, CurColumn);
					GotoXY(CurX, CurY);
					StatusShown = TRUE;
					SetShowColor(ListPos);
				}

				if (!ShowStatus)
					SetShowColor(ListPos);

				if (ColumnType >= CUSTOM_COLUMN0 && ColumnType <= CUSTOM_COLUMN_LAST) {
					int ColumnNumber = ColumnType - CUSTOM_COLUMN0;
					const wchar_t *ColumnData = nullptr;

					if (ColumnNumber < ListData[ListPos]->CustomColumnNumber)
						ColumnData = ListData[ListPos]->CustomColumnData[ColumnNumber];

					if (!ColumnData)
						ColumnData = ListData[ListPos]->strCustomData;	// L"";

					int CurLeftPos = 0;

					if (!ShowStatus && LeftPos > 0) {
						CurLeftPos = MakeCurLeftPos(ColumnWidth, ColumnData, LeftPos, MaxLeftPos);
					}

					FS << fmt::Cells() << fmt::LeftAlign() << fmt::Size(ColumnWidth)
						<< ColumnData + CurLeftPos;
				} else {
					switch (ColumnType) {
						case NAME_COLUMN: {
							int Width = ColumnWidth;
							int ViewFlags = ColumnTypes[K];

							if ((ViewFlags & COLUMN_MARK) && Width > 2) {
								Text(ListData[ListPos]->Selected ? L"\x221A " : L"  ");
								Width -= 2;
							}
#if 1
							{ /// Draw mark str
							size_t prews = std::min(Opt.MinFilenameIndentation, Opt.MaxFilenameIndentation);

							if (Opt.ShowFilenameMarks && Opt.Highlight ) {
								const HighlightDataColor *const hl = ListData[ListPos]->ColorsPtr;

								if (Opt.FilenameMarksAlign && MarkLM > prews)
									prews = std::min(MarkLM, (size_t)Opt.MaxFilenameIndentation);

								if (hl->MarkLen && Width > 2) {
									const uint64_t OldColor = GetColor();
									size_t	ng = Width, outlen;

									outlen = StrSizeOfCells(hl->Mark, hl->MarkLen, ng, false);
									ng = StrCellsCount( hl->Mark, outlen );

									Width -= ng;
									if (ng < prews)
										prews -= ng;
									else
										prews = 0;

									if (!ShowStatus)
										SetShowColor(ListPos, HIGHLIGHTCOLORTYPE_MARKSTR);

									Text(hl->Mark, outlen);
									SetColor(OldColor);
								}
							}

							if (!ShowStatus && prews && Width > 2) {
								if (prews >= (size_t)Width) {
									prews = Width;
									Width = 0;
								}
								else
									Width -= prews;

								Text(L' ', prews);
							}
							} /// Draw mark str
#endif
							const wchar_t *NamePtr = ListData[ListPos]->strName;
							const wchar_t *NameCopy = NamePtr;

							if (ViewFlags & COLUMN_NAMEONLY) {
								// BUGBUG!!!
								//  !!! НЕ УВЕРЕН, но то, что отображается пустое
								//  пространство вместо названия - бага
								NamePtr = PointToFolderNameIfFolder(NamePtr);
							}

							int CurLeftPos = 0;
							int RightAlign = (ViewFlags & COLUMN_RIGHTALIGN);
							int LeftBracket = FALSE, RightBracket = FALSE;

							if (!ShowStatus && LeftPos) {
								if (LeftPos > 0 && !RightAlign) {
									CurLeftPos = MakeCurLeftPos(Width, NamePtr, LeftPos, MaxLeftPos);
									NamePtr+= CurLeftPos;
								} else if (RightAlign) {
									int Cells = (int)StrZCellsCount(NamePtr);
									if (Cells > Width) {
										int CurRightPos = LeftPos;
										if (Cells + CurRightPos < Width)
											CurRightPos = Width - Cells;
										else
											RightBracket = TRUE;

										size_t ng = Cells + CurRightPos - Width;
										NamePtr+= StrSizeOfCells(NamePtr, wcslen(NamePtr), ng, false);
										RightAlign = FALSE;

										if (MinLeftPos > CurRightPos)
											MinLeftPos = CurRightPos;
									}
								}
							}

							FARString strName;
							int TooLong = ConvertName(strName, NamePtr, Width, RightAlign,
								ShowStatus, ListData[ListPos]->FileAttr, ListData[ListPos]);
							if (CurLeftPos)
								LeftBracket = TRUE;

							if (TooLong) {
								if (RightAlign)
									LeftBracket = TRUE;

								if (!RightAlign && StrZCellsCount(NamePtr) > size_t(Width))
									RightBracket = TRUE;
							}

							if (!ShowStatus) {
								if (ViewSettings.FileUpperToLowerCase)
									if (!(ListData[ListPos]->FileAttr & FILE_ATTRIBUTE_DIRECTORY)
											&& !IsCaseMixed(NameCopy))
										strName.Lower();

								if ((ViewSettings.FolderUpperCase)
										&& (ListData[ListPos]->FileAttr & FILE_ATTRIBUTE_DIRECTORY))
									strName.Upper();

								if ((ViewSettings.FileLowerCase)
										&& !(ListData[ListPos]->FileAttr & FILE_ATTRIBUTE_DIRECTORY))
									strName.Lower();
							}

							Text(strName);
//							Text(NamePtr);
							int NameX = WhereX();

							if (!ShowStatus) {
								if (LeftBracket) {
									GotoXY(CurX - 1, CurY);

									if (Level == 1)
										SetFarColor(COL_PANELBOX);

									Text(openBracket);
									SetShowColor(J);
								}

								if (RightBracket) {
									if (Level == ColumnsInGlobal)
										SetFarColor(COL_PANELBOX);

									GotoXY(NameX, CurY);
									Text(closeBracket);
									ShowDivider = FALSE;

									if (Level == ColumnsInGlobal)
										SetFarColor(COL_PANELTEXT);
									else
										SetShowColor(J);
								}
							}
						} break;
						case SIZE_COLUMN:
						case PHYSICAL_COLUMN: {
							Text(FormatStr_Size(ListData[ListPos]->FileSize, ListData[ListPos]->PhysicalSize,
									ListData[ListPos]->strName, ListData[ListPos]->FileAttr,
									ListData[ListPos]->ShowFolderSize, ColumnType, ColumnTypes[K],
									ColumnWidth).CPtr());
							break;
						}

						case DATE_COLUMN:
						case TIME_COLUMN:
						case WDATE_COLUMN:
						case CDATE_COLUMN:
						case ADATE_COLUMN:
						case CHDATE_COLUMN: {
							FILETIME *FileTime;

							switch (ColumnType) {
								case CDATE_COLUMN:
									FileTime = &ListData[ListPos]->CreationTime;
									break;
								case ADATE_COLUMN:
									FileTime = &ListData[ListPos]->AccessTime;
									break;
								case CHDATE_COLUMN:
									FileTime = &ListData[ListPos]->ChangeTime;
									break;
								case DATE_COLUMN:
								case TIME_COLUMN:
								case WDATE_COLUMN:
								default:
									FileTime = &ListData[ListPos]->WriteTime;
									break;
							}

							FS << FormatStr_DateTime(FileTime, ColumnType, ColumnTypes[K], ColumnWidth);
							break;
						}

						case ATTR_COLUMN: {
							FS << FormatStr_Attribute(ListData[ListPos]->FileAttr,
									ListData[ListPos]->FileMode, ColumnWidth);
							break;
						}

						case DIZ_COLUMN: {
							int CurLeftPos = 0;

							if (!ShowStatus && LeftPos > 0) {
								CurLeftPos = MakeCurLeftPos(ColumnWidth, ListData[ListPos]->DizText, LeftPos,
										MaxLeftPos);
							}

							FARString strDizText = ListData[ListPos]->DizText
									? ListData[ListPos]->DizText + CurLeftPos
									: L"";
							size_t pos;

							if (strDizText.Pos(pos, L'\4'))
								strDizText.Truncate(pos);

							FS << fmt::Cells() << fmt::LeftAlign() << fmt::Size(ColumnWidth) << strDizText;
							break;
						}

						case OWNER_COLUMN: {
							const wchar_t *Owner = ListData[ListPos]->strOwner;

							if (Owner && !(ColumnTypes[K] & COLUMN_FULLOWNER) && PanelMode != PLUGIN_PANEL) {
								const wchar_t *SlashPos = FirstSlash(Owner);

								if (SlashPos)
									Owner = SlashPos + 1;
							} else if (Owner && IsSlash(*Owner)) {
								Owner++;
							}

							int CurLeftPos = 0;

							if (!ShowStatus && LeftPos > 0) {
								int Cells = StrZCellsCount(Owner);

								if (Cells > ColumnWidth) {
									CurLeftPos = LeftPos;

									if (CurLeftPos > Cells - ColumnWidth)
										CurLeftPos = Cells - ColumnWidth;

									if (CurLeftPos > MaxLeftPos)
										MaxLeftPos = CurLeftPos;
								}
							}

							FS << fmt::Cells() << fmt::LeftAlign() << fmt::Size(ColumnWidth)
								<< Owner + CurLeftPos;
							break;
						}

						case GROUP_COLUMN: {
							const wchar_t *Group = ListData[ListPos]->strGroup;

							int CurLeftPos = 0;

							if (!ShowStatus && LeftPos > 0) {
								int Cells = StrZCellsCount(Group);

								if (Cells > ColumnWidth) {
									CurLeftPos = LeftPos;

									if (CurLeftPos > Cells - ColumnWidth)
										CurLeftPos = Cells - ColumnWidth;

									if (CurLeftPos > MaxLeftPos)
										MaxLeftPos = CurLeftPos;
								}
							}

							FS << fmt::Cells() << fmt::LeftAlign() << fmt::Size(ColumnWidth)
								<< Group + CurLeftPos;
							break;
						}

						case NUMLINK_COLUMN: {
							FS << fmt::Cells() << fmt::Size(ColumnWidth) << ListData[ListPos]->NumberOfLinks;
							break;
						}
					}
				}
			} else {
				FS << fmt::Cells() << fmt::Expand(ColumnWidth) << L"";
			}

			if (ShowDivider == FALSE)
				GotoXY(CurX + ColumnWidth + 1, CurY);
			else {
				if (!ShowStatus) {
					SetShowColor(ListPos);

					if (Level == ColumnsInGlobal)
						SetFarColor(COL_PANELBOX);
				}

				if (K == ColumnCount - 1)
					SetFarColor(COL_PANELBOX);

				GotoXY(CurX + ColumnWidth, CurY);

				if (K == ColumnCount - 1)
					BoxText(CurX + ColumnWidth == X2 ? BoxSymbols[BS_V2] : L' ');
				else
					BoxText(ShowStatus ? L' ' : BoxSymbols[BS_V1]);

				if (!ShowStatus)
					SetFarColor(COL_PANELTEXT);
			}

			if (!ShowStatus) {
				if (Level == ColumnsInGlobal) {
					Level = 0;
					CurColumn++;
				}

				Level++;
			}
		}

		if ((!ShowStatus || StatusLine) && WhereX() < X2) {
			SetFarColor(COL_PANELTEXT);
			FS << fmt::Cells() << fmt::Expand(X2 - WhereX()) << L"";
		}
	}

	if (!ShowStatus && !StatusShown && Opt.ShowPanelStatus) {
		SetScreen(X1 + 1, Y2 - 1, X2 - 1, Y2 - 1, L' ', FarColorToReal(COL_PANELTEXT));
		SetFarColor(COL_PANELTEXT);	//???
									// GotoXY(X1+1,Y2-1);
		// FS<<fmt::Expand(X2-X1-1)<<L"";
	}

	if (!ShowStatus) {
		if (LeftPos < 0)
			LeftPos = MinLeftPos;

		if (LeftPos > 0)
			LeftPos = MaxLeftPos;
	}
}

int FileList::IsFullScreen()
{
	return this->ViewSettings.FullScreen;
}

int FileList::IsModeFullScreen(int Mode)
{
	return (ViewSettingsArray[Mode].FullScreen);
}

int FileList::IsDizDisplayed()
{
	return (IsColumnDisplayed(DIZ_COLUMN));
}

int FileList::IsColumnDisplayed(int Type)
{

	for (int i = 0; i < ViewSettings.ColumnCount; i++)
		if ((int)(ViewSettings.ColumnType[i] & 0xff) == Type)
			return TRUE;

	for (int i = 0; i < ViewSettings.StatusColumnCount; i++)
		if ((int)(ViewSettings.StatusColumnType[i] & 0xff) == Type)
			return TRUE;

	return FALSE;
}
