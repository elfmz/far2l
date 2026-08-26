/*
dirinfo.cpp

GetDirInfo & GetPluginDirInfo
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

#include "dirinfo.hpp"
#include "plugapi.hpp"
#include "keys.hpp"
#include "scantree.hpp"
#include "savescr.hpp"
#include "lang.hpp"
#include "RefreshFrameManager.hpp"
#include "TPreRedrawFunc.hpp"
#include "ctrlobj.hpp"
#include "filefilter.hpp"
#include "interf.hpp"
#include "message.hpp"
#include "constitle.hpp"
#include "keyboard.hpp"
#include "pathmix.hpp"
#include "strmix.hpp"
#include "wakeful.hpp"
#include "config.hpp"

static void DrawGetDirInfoMsg(const wchar_t *Title, const wchar_t *Name, const UINT64 Size)
{
	if (Title == nullptr || Name == nullptr) {
		return;
	}

	FARString strSize;
	FileSizeToStr(strSize, Size, 8, COLUMN_FLOATSIZE | COLUMN_COMMAS);
	RemoveLeadingSpaces(strSize);
	Message(0, 0, Title, Msg::ScanningFolder, Name, strSize);
	PreRedrawItem preRedrawItem = PreRedraw.Peek();
	preRedrawItem.Param.Param1 = (void *)Title;
	preRedrawItem.Param.Param2 = (void *)Name;
	preRedrawItem.Param.Param3 = reinterpret_cast<LPCVOID>(Size);
	PreRedraw.SetParam(preRedrawItem.Param);
}

static void PR_DrawGetDirInfoMsg()
{
	PreRedrawItem preRedrawItem = PreRedraw.Peek();
	DrawGetDirInfoMsg((const wchar_t *)preRedrawItem.Param.Param1,
			(const wchar_t *)preRedrawItem.Param.Param2,
			reinterpret_cast<const UINT64>(preRedrawItem.Param.Param3));
}

static int ScanDirInfoImpl(const wchar_t *DirName, DirInfoData &Data, bool CountDirSize,
		bool ScanSymlinks, FileFilter *Filter, bool UseFilter,
		const std::function<int(const DirInfoData &)> &Progress)
{
	ScanTree ScTree(FALSE, TRUE, ScanSymlinks);
	FAR_FIND_DATA_EX FindData;
	FARString strFullName, strCurDirName, strLastDirName;
	ScannedINodes scanned_inodes;
	struct stat s = {0};

	Data = {};
	ScTree.SetFindPath(DirName, L"*", 0);

	if (sdc_stat(Wide2MB(DirName).c_str(), &s) == 0) {
		if (CountDirSize) {
			Data.FileSize = s.st_size;
			Data.PhysicalSize = ((DWORD64)s.st_blocks) * 512;
		}
		Data.ClusterSize = s.st_blksize;
	}

	while (ScTree.GetNextName(&FindData, strFullName)) {
		if (Progress) {
			const int Result = Progress(Data);
			if (Result != 1)
				return Result;
		}

		const DWORD file_attributes = FindData.dwFileAttributes;
		const bool is_directory = (file_attributes & FILE_ATTRIBUTE_DIRECTORY) != 0;
		const bool is_reparse_point = (file_attributes & FILE_ATTRIBUTE_REPARSE_POINT) != 0;

		if (!is_directory || CountDirSize)
			Data.PhysicalSize+= FindData.nPhysicalSize;

		if (is_reparse_point) {
			if (CountDirSize && sdc_lstat(strFullName.GetMB().c_str(), &s) == 0)
				Data.FileSize+= s.st_size;

			Data.FileCount++;
			if (!ScanSymlinks)
				continue;
		}

		if (!scanned_inodes.Put(FindData.UnixDevice, FindData.UnixNode))
			continue;

		if (is_directory) {
			if (!UseFilter) {
				Data.DirCount++;
				if (CountDirSize)
					Data.FileSize+= FindData.nFileSize;
			} else if (Filter->FileInFilter(FindData)) {
				if (CountDirSize)
					Data.FileSize+= FindData.nFileSize;
			} else {
				ScTree.SkipDir();
			}
		} else {
			if (UseFilter && !Filter->FileInFilter(FindData))
				continue;

			if (UseFilter) {
				strCurDirName = strFullName;
				CutToSlash(strCurDirName);

				if (StrCmp(strCurDirName, strLastDirName)) {
					Data.DirCount++;
					strLastDirName = strCurDirName;
				}
			}

			Data.FileCount++;
			Data.FileSize+= FindData.nFileSize;
		}
	}

	return 1;
}

bool ScanDirInfo(const wchar_t *DirName, DirInfoData &Data, bool CountDirSize, bool ScanSymlinks,
		const DirInfoProgress &Progress)
{
	return ScanDirInfoImpl(DirName, Data, CountDirSize, ScanSymlinks, nullptr, false,
			[&](const DirInfoData &Current) { return (!Progress || Progress(Current)) ? 1 : 0; }) == 1;
}

int GetDirInfo(const wchar_t *Title, const wchar_t *DirName, uint32_t &DirCount, uint32_t &FileCount,
		uint64_t &FileSize, uint64_t &PhysicalSize, uint32_t &ClusterSize, clock_t MsgWaitTime,
		FileFilter *Filter, DWORD Flags)
{
	FARString strFullDirName;
	ConvertNameToFull(DirName, strFullDirName);
	SaveScreen SaveScr;
	UndoGlobalSaveScrPtr UndSaveScr(&SaveScr);
	TPreRedrawFuncGuard preRedrawFuncGuard(PR_DrawGetDirInfoMsg);
	wakeful W;
	clock_t StartTime = GetProcessUptimeMSec();
	SetCursorType(FALSE, 0);
	/*
		$ 20.03.2002 DJ
		для . - покажем имя родительского каталога
	*/
	const wchar_t *ShowDirName = DirName;

	if (DirName[0] == L'.' && !DirName[1]) {
		const wchar_t *p = LastSlash(strFullDirName);

		if (p)
			ShowDirName = p + 1;
	}

	ConsoleTitle OldTitle;
	RefreshFrameManager frref(ScrX, ScrY, MsgWaitTime, Flags & GETDIRINFO_DONTREDRAWFRAME);
	const bool count_dir_size = !Opt.OnlyFilesSize;
	const bool use_filter = (Flags & GETDIRINFO_USEFILTER) != 0;
	const bool scan_symlinks = (Flags & GETDIRINFO_SCANSYMLINKDEF)
			? Opt.ScanJunction
			: (Flags & GETDIRINFO_SCANSYMLINK) != 0;
	const bool can_break = !CtrlObject->Macro.IsExecuting() && !WinPortTesting();
	DirInfoData Data;

	const int Result = ScanDirInfoImpl(DirName, Data, count_dir_size, scan_symlinks, Filter, use_filter,
			[&](const DirInfoData &Current) {
		if (can_break) {
			INPUT_RECORD rec;

			switch (PeekInputRecord(&rec)) {
				case 0:
				case KEY_IDLE:
					break;
				case KEY_NONE:
				case KEY_ALT:
				case KEY_CTRL:
				case KEY_SHIFT:
				case KEY_RALT:
				case KEY_RCTRL:
					GetInputRecord(&rec);
					break;
				case KEY_ESC:
				case KEY_BREAK:
					GetInputRecord(&rec);
					return 0;
				default:

					if (Flags & GETDIRINFO_ENHBREAK)
						return -1;

					GetInputRecord(&rec);
					break;
			}
		}

		if (MsgWaitTime != -1) {
			clock_t CurTime = GetProcessUptimeMSec();

			if (CurTime - StartTime > MsgWaitTime) {
				StartTime = CurTime;
				MsgWaitTime = 500;
				OldTitle.Set(L"%ls %ls", Msg::ScanningFolder.CPtr(), ShowDirName);	// покажем заголовок консоли
				SetCursorType(FALSE, 0);
				DrawGetDirInfoMsg(Title, ShowDirName, Current.FileSize);
			}
		}

		return 1;
	});

	DirCount = Data.DirCount;
	FileCount = Data.FileCount;
	FileSize = Data.FileSize;
	PhysicalSize = Data.PhysicalSize;
	ClusterSize = Data.ClusterSize;
	return Result;
}

int GetPluginDirInfo(HANDLE hPlugin, const wchar_t *DirName, uint32_t &DirCount, uint32_t &FileCount,
		uint64_t &FileSize, uint64_t &PhysicalSize)
{
	PluginPanelItem *PanelItem = nullptr;
	int ItemsNumber, ExitCode;
	DirCount = FileCount = 0;
	FileSize = PhysicalSize = 0;
	PluginHandle *ph = (PluginHandle *)hPlugin;

	if ((ExitCode = FarGetPluginDirList((INT_PTR)ph->pPlugin, ph->hPlugin, DirName, &PanelItem, &ItemsNumber))
			== TRUE)	// INT_PTR - BUGBUG
	{
		for (int I = 0; I < ItemsNumber; I++) {
			if (PanelItem[I].FindData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) {
				DirCount++;
			} else {
				FileCount++;
				FileSize+= PanelItem[I].FindData.nFileSize;
				PhysicalSize+= PanelItem[I].FindData.nPhysicalSize
						? PanelItem[I].FindData.nPhysicalSize
						: PanelItem[I].FindData.nFileSize;
			}
		}
	}

	if (PanelItem)
		FarFreePluginDirList(PanelItem, ItemsNumber);

	return (ExitCode);
}
