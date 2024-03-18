/*
panelmix.cpp

Commonly used panel related functions
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

#include "panelmix.hpp"
#include "strmix.hpp"
#include "filepanels.hpp"
#include "config.hpp"
#include "panel.hpp"
#include "ctrlobj.hpp"
#include "keys.hpp"
#include "treelist.hpp"
#include "filelist.hpp"
#include "pathmix.hpp"
#include "panelctype.hpp"
#include "lang.hpp"
#include "datetime.hpp"

int ColumnTypeWidth[] = {0, 6, 6, 8, 5, 14, 14, 14, 14, 10, 0, 0, 3, 3, 6, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
		0, 0, 0, 0, 0, 0, 0, 0, 0};

static const wchar_t *ColumnSymbol[] = {L"N", L"S", L"P", L"D", L"T", L"DM", L"DC", L"DA", L"DE", L"A", L"Z",
		L"O", L"U", L"LN", L"F", L"G", L"C0", L"C1", L"C2", L"C3", L"C4", L"C5", L"C6", L"C7", L"C8", L"C9",
		L"C10", L"C11", L"C12", L"C13", L"C14", L"C15", L"C16", L"C17", L"C18", L"C19"};

void ShellUpdatePanels(Panel *SrcPanel, BOOL NeedSetUpADir)
{
	if (!SrcPanel)
		SrcPanel = CtrlObject->Cp()->ActivePanel;

	Panel *AnotherPanel = CtrlObject->Cp()->GetAnotherPanel(SrcPanel);

	switch (SrcPanel->GetType()) {
		case QVIEW_PANEL:
		case INFO_PANEL:
			SrcPanel = CtrlObject->Cp()->GetAnotherPanel(AnotherPanel = SrcPanel);
	}

	int AnotherType = AnotherPanel->GetType();

	if (AnotherType != QVIEW_PANEL && AnotherType != INFO_PANEL) {
		if (NeedSetUpADir) {
			FARString strCurDir;
			SrcPanel->GetCurDir(strCurDir);
			AnotherPanel->SetCurDir(strCurDir, TRUE);
			AnotherPanel->Update(UPDATE_KEEP_SELECTION | UPDATE_SECONDARY);
		} else {
			// TODO: ???
			// if(AnotherPanel->NeedUpdatePanel(SrcPanel))
			//	AnotherPanel->Update(UPDATE_KEEP_SELECTION|UPDATE_SECONDARY);
			// else
			{
				// Сбросим время обновления панели. Если там есть нотификация - обновится сама.
				if (AnotherType == FILE_PANEL)
					((FileList *)AnotherPanel)->ResetLastUpdateTime();

				AnotherPanel->UpdateIfChanged(UIC_UPDATE_NORMAL);
			}
		}
	}

	SrcPanel->Update(UPDATE_KEEP_SELECTION);

	if (AnotherType == QVIEW_PANEL)
		AnotherPanel->Update(UPDATE_KEEP_SELECTION | UPDATE_SECONDARY);

	CtrlObject->Cp()->Redraw();
}

bool CheckUpdateAnotherPanel(Panel *SrcPanel, const wchar_t *SelName)
{
	if (!SrcPanel)
		SrcPanel = CtrlObject->Cp()->ActivePanel;

	Panel *AnotherPanel = CtrlObject->Cp()->GetAnotherPanel(SrcPanel);
	AnotherPanel->CloseFile();

	if (AnotherPanel->GetMode() == NORMAL_PANEL) {
		FARString strAnotherCurDir;
		FARString strFullName;
		AnotherPanel->GetCurDir(strAnotherCurDir);
		AddEndSlash(strAnotherCurDir);
		ConvertNameToFull(SelName, strFullName);
		AddEndSlash(strFullName);

		if (wcsstr(strAnotherCurDir, strFullName)) {
			((FileList *)AnotherPanel)->CloseChangeNotification();
			return true;
		}
	}

	return false;
}

int _MakePath1(DWORD Key, FARString &strPathName, const wchar_t *Param2, int escaping)
{
	int RetCode = FALSE;
	int NeedRealName = FALSE;
	strPathName.Clear();

	switch (Key) {
		case KEY_CTRLALTBRACKET:		// Вставить сетевое (UNC) путь из левой панели
		case KEY_CTRLALTBACKBRACKET:	// Вставить сетевое (UNC) путь из правой панели
		case KEY_ALTSHIFTBRACKET:		// Вставить сетевое (UNC) путь из активной панели
		case KEY_ALTSHIFTBACKBRACKET:	// Вставить сетевое (UNC) путь из пассивной панели
			NeedRealName = TRUE;
		case KEY_CTRLBRACKET:			// Вставить путь из левой панели
		case KEY_CTRLBACKBRACKET:		// Вставить путь из правой панели
		case KEY_CTRLSHIFTBRACKET:		// Вставить путь из активной панели
		case KEY_CTRLSHIFTBACKBRACKET:	// Вставить путь из пассивной панели
		case KEY_CTRLSHIFTNUMENTER:		// Текущий файл с пасс.панели
		case KEY_SHIFTNUMENTER:			// Текущий файл с актив.панели
		case KEY_CTRLSHIFTENTER:		// Текущий файл с пасс.панели
		case KEY_SHIFTENTER:			// Текущий файл с актив.панели
		{
			Panel *SrcPanel = nullptr;
			FilePanels *Cp = CtrlObject->Cp();

			switch (Key) {
				case KEY_CTRLALTBRACKET:
				case KEY_CTRLBRACKET:
					SrcPanel = Cp->LeftPanel;
					break;
				case KEY_CTRLALTBACKBRACKET:
				case KEY_CTRLBACKBRACKET:
					SrcPanel = Cp->RightPanel;
					break;
				case KEY_SHIFTNUMENTER:
				case KEY_SHIFTENTER:
				case KEY_ALTSHIFTBRACKET:
				case KEY_CTRLSHIFTBRACKET:
					SrcPanel = Cp->ActivePanel;
					break;
				case KEY_CTRLSHIFTNUMENTER:
				case KEY_CTRLSHIFTENTER:
				case KEY_ALTSHIFTBACKBRACKET:
				case KEY_CTRLSHIFTBACKBRACKET:
					SrcPanel = Cp->GetAnotherPanel(Cp->ActivePanel);
					break;
			}

			if (SrcPanel) {
				if (Key == KEY_SHIFTENTER || Key == KEY_CTRLSHIFTENTER || Key == KEY_SHIFTNUMENTER
						|| Key == KEY_CTRLSHIFTNUMENTER) {
					SrcPanel->GetCurName(strPathName);
				} else {
					/* TODO: Здесь нужно учесть, что у TreeList тоже есть путь :-) */
					if (!(SrcPanel->GetType() == FILE_PANEL || SrcPanel->GetType() == TREE_PANEL))
						return FALSE;

					SrcPanel->GetCurDirPluginAware(strPathName);
					if (NeedRealName && SrcPanel->GetType() == FILE_PANEL
							&& SrcPanel->GetMode() != PLUGIN_PANEL) {
						FileList *SrcFilePanel = (FileList *)SrcPanel;
						SrcFilePanel->CreateFullPathName(strPathName, FILE_ATTRIBUTE_DIRECTORY, strPathName,
								TRUE);
					}

					AddEndSlash(strPathName);
				}

				if (escaping & Opt.QuotedName & QUOTEDNAME_INSERT)
					EscapeSpace(strPathName);

				if (Param2)
					strPathName+= Param2;

				RetCode = TRUE;
			}
		} break;
	}

	return RetCode;
}

void TextToViewSettings(const wchar_t *ColumnTitles, const wchar_t *ColumnWidths,
		unsigned int *ViewColumnTypes, int *ViewColumnWidths, int *ViewColumnWidthsTypes, int &ColumnCount)
{
	const wchar_t *TextPtr = ColumnTitles;

	for (ColumnCount = 0; ColumnCount < PANEL_COLUMNCOUNT; ColumnCount++) {
		FARString strArgName;

		if (!(TextPtr = GetCommaWord(TextPtr, strArgName)))
			break;

		strArgName.Upper();

		if (strArgName.At(0) == L'N') {
			unsigned int &ColumnType = ViewColumnTypes[ColumnCount];
			ColumnType = NAME_COLUMN;
			const wchar_t *Ptr = strArgName.CPtr() + 1;

			while (*Ptr) {
				switch (*Ptr) {
					case L'M':
						ColumnType|= COLUMN_MARK;
						break;
					case L'O':
						ColumnType|= COLUMN_NAMEONLY;
						break;
					case L'R':
						ColumnType|= COLUMN_RIGHTALIGN;
						break;
				}

				Ptr++;
			}
		} else {
			if (strArgName.At(0) == L'S' || strArgName.At(0) == L'P' || strArgName.At(0) == L'G') {
				unsigned int &ColumnType = ViewColumnTypes[ColumnCount];
				ColumnType = (strArgName.At(0) == L'S') ? SIZE_COLUMN : PHYSICAL_COLUMN;
				const wchar_t *Ptr = strArgName.CPtr() + 1;

				while (*Ptr) {
					switch (*Ptr) {
						case L'C':
							ColumnType|= COLUMN_COMMAS;
							break;
						case L'E':
							ColumnType|= COLUMN_ECONOMIC;
							break;
						case L'F':
							ColumnType|= COLUMN_FLOATSIZE;
							break;
						case L'T':
							ColumnType|= COLUMN_THOUSAND;
							break;
					}

					Ptr++;
				}
			} else {
				if (!StrCmpN(strArgName, L"DM", 2) || !StrCmpN(strArgName, L"DC", 2)
						|| !StrCmpN(strArgName, L"DA", 2) || !StrCmpN(strArgName, L"DE", 2)) {
					unsigned int &ColumnType = ViewColumnTypes[ColumnCount];

					switch (strArgName.At(1)) {
						case L'M':
							ColumnType = WDATE_COLUMN;
							break;
						case L'C':
							ColumnType = CDATE_COLUMN;
							break;
						case L'A':
							ColumnType = ADATE_COLUMN;
							break;
						case L'E':
							ColumnType = CHDATE_COLUMN;
							break;
					}

					const wchar_t *Ptr = strArgName.CPtr() + 2;

					while (*Ptr) {
						switch (*Ptr) {
							case L'B':
								ColumnType|= COLUMN_BRIEF;
								break;
							case L'M':
								ColumnType|= COLUMN_MONTH;
								break;
						}

						Ptr++;
					}
				} else {
					if (strArgName.At(0) == L'U') {
						unsigned int &ColumnType = ViewColumnTypes[ColumnCount];
						ColumnType = GROUP_COLUMN;
					} else if (strArgName.At(0) == L'O') {
						unsigned int &ColumnType = ViewColumnTypes[ColumnCount];
						ColumnType = OWNER_COLUMN;

						if (strArgName.At(1) == L'L')
							ColumnType|= COLUMN_FULLOWNER;
					} else {
						for (unsigned I = 0; I < ARRAYSIZE(ColumnSymbol); I++) {
							if (!StrCmp(strArgName, ColumnSymbol[I])) {
								ViewColumnTypes[ColumnCount] = I;
								break;
							}
						}
					}
				}
			}
		}
	}

	TextPtr = ColumnWidths;

	for (int I = 0; I < ColumnCount; I++) {
		FARString strArgName;

		if (!(TextPtr = GetCommaWord(TextPtr, strArgName)))
			break;

		ViewColumnWidths[I] = _wtoi(strArgName);
		ViewColumnWidthsTypes[I] = COUNT_WIDTH;

		if (strArgName.GetLength() > 1) {
			switch (strArgName.At(strArgName.GetLength() - 1)) {
				case L'%':
					ViewColumnWidthsTypes[I] = PERCENT_WIDTH;
					break;
			}
		}
	}
}

void ViewSettingsToText(unsigned int *ViewColumnTypes, int *ViewColumnWidths, int *ViewColumnWidthsTypes,
		int ColumnCount, FARString &strColumnTitles, FARString &strColumnWidths)
{
	strColumnTitles.Clear();
	strColumnWidths.Clear();

	for (int I = 0; I < ColumnCount; ++I) {
		const int ColumnType = ViewColumnTypes[I] & 0xff;
		strColumnTitles+= ColumnSymbol[ColumnType];
		switch (ColumnType) {
			case NAME_COLUMN:
				if (ViewColumnTypes[I] & COLUMN_MARK)
					strColumnTitles+= L'M';

				if (ViewColumnTypes[I] & COLUMN_NAMEONLY)
					strColumnTitles+= L'O';

				if (ViewColumnTypes[I] & COLUMN_RIGHTALIGN)
					strColumnTitles+= L'R';
				break;

			case SIZE_COLUMN:
			case PHYSICAL_COLUMN:
				if (ViewColumnTypes[I] & COLUMN_COMMAS)
					strColumnTitles+= L'C';

				if (ViewColumnTypes[I] & COLUMN_ECONOMIC)
					strColumnTitles+= L'E';

				if (ViewColumnTypes[I] & COLUMN_FLOATSIZE)
					strColumnTitles+= L'F';

				if (ViewColumnTypes[I] & COLUMN_THOUSAND)
					strColumnTitles+= L'T';
				break;

			case WDATE_COLUMN:
			case ADATE_COLUMN:
			case CDATE_COLUMN:
			case CHDATE_COLUMN:
				if (ViewColumnTypes[I] & COLUMN_BRIEF)
					strColumnTitles+= L'B';

				if (ViewColumnTypes[I] & COLUMN_MONTH)
					strColumnTitles+= L'M';
				break;

			case OWNER_COLUMN:
				if (ViewColumnTypes[I] & COLUMN_FULLOWNER)
					strColumnTitles+= L'L';
				break;
		}

		strColumnWidths.AppendFormat(L"%d", ViewColumnWidths[I]);
		if (ViewColumnWidthsTypes[I] == PERCENT_WIDTH)
			strColumnWidths+= L'%';

		if (I < ColumnCount - 1) {
			strColumnTitles+= L',';
			strColumnWidths+= L',';
		}
	}
}

const FARString FormatStr_Attribute(DWORD FileAttributes, DWORD UnixMode, int Width)
{
	FormatString strResult;
	wchar_t OutStr[16] = {};
	if (UnixMode != 0) {
		if (FileAttributes & FILE_ATTRIBUTE_BROKEN)
			OutStr[0] = L'B';
		else if (FileAttributes & FILE_ATTRIBUTE_DEVICE_CHAR)
			OutStr[0] = L'c';
		else if (FileAttributes & FILE_ATTRIBUTE_DEVICE_BLOCK)
			OutStr[0] = L'b';
		else if (FileAttributes & FILE_ATTRIBUTE_DEVICE_FIFO)
			OutStr[0] = L'p';
		else if (FileAttributes & FILE_ATTRIBUTE_DEVICE_SOCK)
			OutStr[0] = L's';
		/*else if (FileAttributes & FILE_ATTRIBUTE_DEVICE)
			OutStr[0] = L'V';*/
		else if (FileAttributes & FILE_ATTRIBUTE_REPARSE_POINT)
			OutStr[0] = L'l';
		else if (FileAttributes & FILE_ATTRIBUTE_DIRECTORY)
			OutStr[0] = L'd';
		else
			OutStr[0] = L'-';

		OutStr[1] = UnixMode & S_IRUSR ? L'r' : L'-';
		OutStr[2] = UnixMode & S_IWUSR ? L'w' : L'-';
		OutStr[3] = UnixMode & S_IXUSR ? (UnixMode & S_ISUID ? L's' : L'x') : (UnixMode & S_ISUID ? L'S' : L'-');
		OutStr[4] = UnixMode & S_IRGRP ? L'r' : L'-';
		OutStr[5] = UnixMode & S_IWGRP ? L'w' : L'-';
		OutStr[6] = UnixMode & S_IXGRP ? (UnixMode & S_ISGID ? L's' : L'x') : (UnixMode & S_ISGID ? L'S' : L'-');
		OutStr[7] = UnixMode & S_IROTH ? L'r' : L'-';
		OutStr[8] = UnixMode & S_IWOTH ? L'w' : L'-';
		OutStr[9] = UnixMode & S_IXOTH ? (UnixMode & S_ISVTX ? L't' : L'x') : (UnixMode & S_ISVTX ? L'T' : L'-');
	} else {
		OutStr[0] = FileAttributes & FILE_ATTRIBUTE_EXECUTABLE ? L'X' : L' ';
		OutStr[1] = FileAttributes & FILE_ATTRIBUTE_READONLY ? L'R' : L' ';
		OutStr[2] = FileAttributes & FILE_ATTRIBUTE_SYSTEM ? L'S' : L' ';
		OutStr[3] = FileAttributes & FILE_ATTRIBUTE_HIDDEN ? L'H' : L' ';
		OutStr[4] = FileAttributes & FILE_ATTRIBUTE_ARCHIVE ? L'A' : L' ';
		OutStr[5] = FileAttributes & FILE_ATTRIBUTE_REPARSE_POINT ? L'L'
				: FileAttributes & FILE_ATTRIBUTE_SPARSE_FILE
				? L'$'
				: L' ';
		OutStr[6] = FileAttributes & FILE_ATTRIBUTE_COMPRESSED ? L'C'
				: FileAttributes & FILE_ATTRIBUTE_ENCRYPTED
				? L'E'
				: L' ';
		OutStr[7] = FileAttributes & FILE_ATTRIBUTE_TEMPORARY ? L'T' : L' ';
		OutStr[8] = FileAttributes & FILE_ATTRIBUTE_NOT_CONTENT_INDEXED ? L'I' : L' ';
		OutStr[9] = FileAttributes & FILE_ATTRIBUTE_OFFLINE ? L'O' : L' ';
		OutStr[10] = FileAttributes & FILE_ATTRIBUTE_VIRTUAL ? L'V' : L' ';
	}

	if (Width > 0)
		strResult << fmt::Size(Width);

	strResult << OutStr;

	return std::move(strResult.strValue());
}

const FARString FormatStr_DateTime(const FILETIME *FileTime, int ColumnType, DWORD Flags, int Width)
{
	FormatString strResult;

	if (Width < 0) {
		if (ColumnType == DATE_COLUMN)
			Width = 0;
		else
			return std::move(strResult.strValue());
	}

	int ColumnWidth = Width;
	int Brief = Flags & COLUMN_BRIEF;
	int TextMonth = Flags & COLUMN_MONTH;
	int FullYear = FALSE;

	switch (ColumnType) {
		case DATE_COLUMN:
		case TIME_COLUMN: {
			Brief = FALSE;
			TextMonth = FALSE;
			if (ColumnType == DATE_COLUMN)
				FullYear = ColumnWidth > 9;
			break;
		}
		case WDATE_COLUMN:
		case CDATE_COLUMN:
		case ADATE_COLUMN:
		case CHDATE_COLUMN: {
			if (!Brief) {
				int CmpWidth = ColumnWidth - TextMonth;

				if (CmpWidth == 15 || CmpWidth == 16 || CmpWidth == 18 || CmpWidth == 19 || CmpWidth > 21)
					FullYear = TRUE;
			}
			ColumnWidth-= 9;
			break;
		}
	}

	FARString strDateStr, strTimeStr;

	ConvertDate(*FileTime, strDateStr, strTimeStr, ColumnWidth, Brief, TextMonth, FullYear);

	strResult << fmt::Size(Width);
	switch (ColumnType) {
		case DATE_COLUMN:
			strResult << strDateStr;
			break;
		case TIME_COLUMN:
			strResult << strTimeStr;
			break;
		default:
			strResult << (strDateStr + L" " + strTimeStr);
			break;
	}

	return std::move(strResult.strValue());
}

const FARString FormatStr_Size(int64_t FileSize, int64_t PhysicalSize, const FARString &strName,
		DWORD FileAttributes, uint8_t ShowFolderSize, int ColumnType, DWORD Flags, int Width)
{
	FormatString strResult;

	bool Physical = (ColumnType == PHYSICAL_COLUMN);

	if (ShowFolderSize == 2) {
		Width--;
		strResult << L"~";
	}

	if (!Physical && (FileAttributes & (FILE_ATTRIBUTE_DIRECTORY | FILE_ATTRIBUTE_REPARSE_POINT))
			&& !ShowFolderSize) {
		const wchar_t *PtrName = Msg::ListFolder;

		if (TestParentFolderName(strName)) {
			PtrName = Msg::ListUp;
		} else if (FileAttributes & FILE_ATTRIBUTE_REPARSE_POINT) {
			PtrName = Msg::ListSymLink;
		}

		strResult << fmt::Size(Width);
		if (StrLength(PtrName) <= Width - 2) {
			// precombine into tmp string to avoid miseffect of fmt::Size etc (#1137)
			strResult << FARString(L"<").Append(PtrName).Append(L'>');
		} else {
			strResult << PtrName;
		}

	} else {
		FARString strOutStr;
		strResult << FileSizeToStr(strOutStr, Physical ? PhysicalSize : FileSize, Width, Flags).CPtr();
	}

	return std::move(strResult.strValue());
}
