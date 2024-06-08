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

int GetDirInfo(const wchar_t *Title, const wchar_t *DirName, uint32_t &DirCount, uint32_t &FileCount,
		uint64_t &FileSize, uint64_t &PhysicalSize, uint32_t &ClusterSize, clock_t MsgWaitTime,
		FileFilter *Filter, DWORD Flags)
{
	FARString strFullDirName;
	FARString strFullName, strCurDirName, strLastDirName;
	ConvertNameToFull(DirName, strFullDirName);
	SaveScreen SaveScr;
	UndoGlobalSaveScrPtr UndSaveScr(&SaveScr);
	TPreRedrawFuncGuard preRedrawFuncGuard(PR_DrawGetDirInfoMsg);
	wakeful W;
	ScanTree ScTree(FALSE, TRUE,
			((Flags & GETDIRINFO_SCANSYMLINKDEF) ? -1 : ((Flags & GETDIRINFO_SCANSYMLINK) != 0)));
	FAR_FIND_DATA_EX FindData;
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
	// DWORD SectorsPerCluster=0,BytesPerSector=0,FreeClusters=0,Clusters=0;

	// Временные хранилища имён каталогов
	strLastDirName.Clear();
	strCurDirName.Clear();
	DirCount = FileCount = 0;
	FileSize = PhysicalSize = 0;
	ClusterSize = 0;
	ScTree.SetFindPath(DirName, L"*", 0);
	ScannedINodes scanned_inodes;

	struct stat s = {0};
	if (sdc_stat(Wide2MB(DirName).c_str(), &s) == 0) {
		if (!Opt.OnlyFilesSize) {	// include size of root dir's node
			FileSize = s.st_size;
			PhysicalSize = ((DWORD64)s.st_blocks) * 512;
		}
		ClusterSize = s.st_blksize;		// TODO: check if its best thing to be used here
	}

	while (ScTree.GetNextName(&FindData, strFullName)) {
		if (!CtrlObject->Macro.IsExecuting() && !WinPortTesting()) {
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

					if (Flags & GETDIRINFO_ENHBREAK) {
						return -1;
					}

					GetInputRecord(&rec);
					break;
			}
		}

		if (!(FindData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) || !Opt.OnlyFilesSize) {
			PhysicalSize+= FindData.nPhysicalSize;
		}

		clock_t CurTime = GetProcessUptimeMSec();

		if (MsgWaitTime != -1 && CurTime - StartTime > MsgWaitTime) {
			StartTime = CurTime;
			MsgWaitTime = 500;
			OldTitle.Set(L"%ls %ls", Msg::ScanningFolder.CPtr(), ShowDirName);	// покажем заголовок консоли
			SetCursorType(FALSE, 0);
			DrawGetDirInfoMsg(Title, ShowDirName, FileSize);
		}
		if (FindData.dwFileAttributes & FILE_ATTRIBUTE_REPARSE_POINT) {
			// include symlink's own size to total size
			if (sdc_lstat(strFullName.GetMB().c_str(), &s) == 0 && !Opt.OnlyFilesSize) {
				FileSize+= s.st_size;
			}
			if (!ScTree.IsSymlinksScanEnabled())
				continue;
		}
		if (!scanned_inodes.Put(FindData.UnixDevice, FindData.UnixNode)) {
			continue;
		}

		if (FindData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) {
			/*
				Счётчик каталогов наращиваем только если не включен фильтр,
				в противном случае это будем делать в подсчёте количества файлов
			*/
			if (!(Flags & GETDIRINFO_USEFILTER)) {
				DirCount++;
				if (!Opt.OnlyFilesSize)
					FileSize+= FindData.nFileSize;
			} else {
				/*
					Если каталог не попадает под фильтр то его надо полностью
					пропустить - иначе при включенном подсчёте total
					он учтётся (mantis 551)
				*/
				if (Filter->FileInFilter(FindData)) {
					if (!Opt.OnlyFilesSize)
						FileSize+= FindData.nFileSize;	// TODO: add size at same condifion as DirCount increment
				} else
					ScTree.SkipDir();
			}
		} else {
			/*
				$ 17.04.2005 KM
				Проверка попадания файла в условия фильра
			*/
			if ((Flags & GETDIRINFO_USEFILTER)) {
				if (!Filter->FileInFilter(FindData))
					continue;
			}

			/*
				Наращиваем счётчик каталогов при включенном фильтре только тогда,
				когда в таком каталоге найден файл, удовлетворяющий условиям
				фильтра.
			*/
			if ((Flags & GETDIRINFO_USEFILTER)) {
				strCurDirName = strFullName;
				CutToSlash(strCurDirName);	//???

				if (StrCmp(strCurDirName, strLastDirName)) {
					DirCount++;
					strLastDirName = strCurDirName;
				}
			}

			FileCount++;
			FileSize+= FindData.nFileSize;
		}
	}

	return 1;
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
