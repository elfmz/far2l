/*
infolist.cpp

Информационная панель
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

#include "infolist.hpp"
#include "macroopcode.hpp"
#include "colors.hpp"
#include "farcolors.hpp"
#include "lang.hpp"
#include "keys.hpp"
#include "ctrlobj.hpp"
#include "filepanels.hpp"
#include "panel.hpp"
#include "help.hpp"
#include "fileview.hpp"
#include "fileedit.hpp"
#include "manager.hpp"
#include "cddrv.hpp"
#include "syslog.hpp"
#include "interf.hpp"
#include "drivemix.hpp"
#include "CachedCreds.hpp"
#include "dirmix.hpp"
#include "pathmix.hpp"
#include "strmix.hpp"
#include "mix.hpp"
#include "execute.hpp"
#include "EditorConfigOrg.hpp"
#include "codepage.hpp" // for ShortReadableCodepageName()
#ifdef __APPLE__
// # include <sys/sysctl.h>
#include <mach/mach_host.h>
#include <mach/vm_statistics.h>
#elif !defined(__FreeBSD__) && !defined(__NetBSD__) && !defined(__DragonFly__) && !defined(__HAIKU__)
#include <sys/sysinfo.h>
#endif
#include <sys/statvfs.h>

static int LastDizWrapMode = -1;
static int LastDizWrapType = -1;
static int LastDizShowScrollbar = -1;

enum InfoListSectionStateIndex
{
	ILSS_DISKINFO,
	ILSS_MEMORYINFO,
	ILSS_EdCfgINFO,
	ILSS_GITINFO,
	/*ILSS_DIRDESCRIPTION,
	ILSS_PLDESCRIPTION,
	ILSS_POWERSTATUS,*/

	ILSS_SIZE
};

struct InfoList::InfoListSectionState
{
	bool Show;   // раскрыть/свернуть?
	int Y;       // Где? (<0 - отсутствует)
};

InfoList::InfoList()
	:
	DizView(nullptr), PrevMacroMode(-1), SectionState(ILSS_SIZE)
{
	Type = INFO_PANEL;
	for (auto& i: SectionState)
	{
		i.Show = true;
		i.Y = -1;
	}
	if (LastDizWrapMode < 0) {
		LastDizWrapMode = Opt.ViOpt.ViewerIsWrap;
		LastDizWrapType = Opt.ViOpt.ViewerWrap;
		LastDizShowScrollbar = Opt.ViOpt.ShowScrollbar;
	}
}

InfoList::~InfoList()
{
	CloseFile();
	SetMacroMode(TRUE);
}

// перерисовка, только если мы текущий фрейм
void InfoList::Update(int Mode)
{
	if (!EnableUpdate)
		return;

	if (CtrlObject->Cp() == FrameManager->GetCurrentFrame())
		Redraw();
}

FARString &InfoList::GetTitle(FARString &strTitle, int SubLen, int TruncSize)
{
	strTitle.Format(L" %ls ", Msg::InfoTitle.CPtr());
	TruncStr(strTitle, X2 - X1 - 3);
	return strTitle;
}

void InfoList::DrawTitle(const wchar_t *Str, int Id, int CurY)
{
	SetFarColor(COL_PANELBOX);
	DrawSeparator(CurY);
	SetFarColor(COL_PANELTEXT);

	if (Str != nullptr && *Str) {
		FARString strTitle;
		strTitle.Format(L" %ls ", Str);
		TruncStr(strTitle, X2 - X1 - 3);
		GotoXY(X1 + (X2 - X1 + 1 - (int)strTitle.GetLength()) / 2, CurY);
		PrintText(strTitle);
	}
	if (Id >= 0 && Id < ILSS_SIZE ) {
		GotoXY(X1 + 1, CurY);
		PrintText(SectionState[Id].Show ? L"[-]" : L"[+]");
		SectionState[Id].Y = CurY;
	}
}

inline void InfoList::DrawTitle(FarLangMsg MsgID, int Id, int CurY)
{
	DrawTitle(MsgID.CPtr(), Id, CurY);
}

void InfoList::ClearTitles()
{
	for (int i=0; i<ILSS_SIZE; i++)
		SectionState[i].Y = -1;
}


void InfoList::DisplayObject()
{
	FARString strTitle;
	FARString strOutStr;
	FARString strRealDir;
	Panel *AnotherPanel;
	//	FARString strDriveRoot;
	FARString strVolumeName, strFileSystemName, strFileSystemMountPoint;
	DWORD MaxNameLength, FileSystemFlags;
	DWORD64 VolumeNumber;
	FARString strDiskNumber;
	CloseFile();
	ClearTitles();

	Box(X1, Y1, X2, Y2, FarColorToReal(COL_PANELBOX), DOUBLE_BOX);
	SetScreen(X1 + 1, Y1 + 1, X2 - 1, Y2 - 1, L' ', FarColorToReal(COL_PANELTEXT));
	SetFarColor(Focus ? COL_PANELSELECTEDTITLE : COL_PANELTITLE);
	GetTitle(strTitle);

	if (!strTitle.IsEmpty()) {
		GotoXY(X1 + (X2 - X1 + 1 - (int)strTitle.GetLength()) / 2, Y1);
		Text(strTitle);
	}

	SetFarColor(COL_PANELTEXT);

	int CurY = Y1 + 1;

	/* #1 - computer name/user name */

	{	/*
		GotoXY(X1 + 2, CurY++);
		PrintText(Msg::InfoCompName);
		PrintInfo(CachedComputerName());

		GotoXY(X1 + 2, CurY++);
		PrintText(Msg::InfoUserName);
		PrintInfo(CachedUserName());
		*/

		GotoXY(X1 + 2, CurY++);
		strTitle.Format(L"%ls / %ls", Msg::InfoCompName.CPtr(), Msg::InfoUserName.CPtr());
		PrintText(strTitle);
		strTitle = CachedComputerName() + " / " + CachedUserName();
		PrintInfo(strTitle);
	}

	/* #2 - disk / plugin info */

	AnotherPanel = CtrlObject->Cp()->GetAnotherPanel(this);
	AnotherPanel->GetCurDir(strCurDir);

	/* #2a.1 - disk info */
	if (AnotherPanel->GetMode() != PLUGIN_PANEL) {

		if (strCurDir.IsEmpty())
			apiGetCurrentDirectory(strCurDir);

		ConvertNameToReal(strCurDir, strRealDir);

		fprintf(stderr, "apiGetVolumeInformation: %ls\n", strRealDir.CPtr());
		bool b_info = apiGetVolumeInformation(strRealDir, &strVolumeName, &VolumeNumber, &MaxNameLength, &FileSystemFlags,
					&strFileSystemName, &strFileSystemMountPoint);
		if (b_info) {
			//		strTitle=FARString(L" ")+DiskType+L" "+Msg::InfoDisk+L" "+(strDriveRoot)+L" ("+strFileSystemName+L") ";
			strTitle = L"(" + strFileSystemName + L")";

			strDiskNumber.Format(L"%08X-%08X", (DWORD)(VolumeNumber >> 32), (DWORD)(VolumeNumber & 0xffffffff));
		} else						// Error!
			strTitle = strCurDir;	// strDriveRoot;

		DrawTitle(strTitle.CPtr(), ILSS_DISKINFO, CurY++);
		if (SectionState[ILSS_DISKINFO].Show) {
			/* #2a.2 - disk info: size */

			uint64_t TotalSize, TotalFree, UserFree;

			if (apiGetDiskSize(strCurDir, &TotalSize, &TotalFree, &UserFree)) {
				GotoXY(X1 + 2, CurY++);
				PrintText(Msg::InfoDiskTotal);
				InsertCommas(TotalSize, strOutStr);
				PrintInfo(strOutStr);

				GotoXY(X1 + 2, CurY++);
				PrintText(Msg::InfoDiskFree);
				InsertCommas(UserFree, strOutStr);
				PrintInfo(strOutStr);
			}

			/* #2a.3 - disk info: label & SN */

			if (!strVolumeName.IsEmpty()) {
				GotoXY(X1 + 2, CurY++);
				PrintText(Msg::InfoDiskLabel);
				PrintInfo(strVolumeName);
			}

			GotoXY(X1 + 2, CurY++);
			PrintText(Msg::InfoDiskNumber);
			PrintInfo(strDiskNumber);

			// new fields
			GotoXY(X1 + 2, CurY++);
			PrintText(Msg::InfoDiskCurDir);
			PrintInfo(strCurDir);

			if ( strRealDir != strCurDir ) {
				GotoXY(X1 + 2, CurY++);
				PrintText(Msg::InfoDiskRealDir);
				PrintInfo(strRealDir);
			}

			if (b_info) {
				GotoXY(X1 + 2, CurY++);
				PrintText(Msg::InfoDiskMountPoint);
				PrintInfo(strFileSystemMountPoint);

				GotoXY(X1 + 2, CurY++);
				PrintText(Msg::InfoDiskMaxFilenameLength);
				strTitle.Format(L"%lu", (unsigned long) MaxNameLength);
				PrintInfo(strTitle);

				strOutStr.Clear();
#ifdef ST_RDONLY		/* Mount read-only.  */
				strOutStr += (FileSystemFlags & ST_RDONLY) ? "ro" : "rw";
#endif
#ifdef ST_NOSUID		/* Ignore suid and sgid bits.  */
				if (FileSystemFlags & ST_NOSUID)
					strOutStr += ",nosuid";
#endif
#ifdef ST_NODEV			/* Disallow access to device special files.  */
				if (FileSystemFlags & ST_NODEV)
					strOutStr += ",nodev";
#endif
#ifdef ST_NOEXEC		/* Disallow program execution.  */
				if (FileSystemFlags & ST_NOEXEC)
					strOutStr += ",noexec";
#endif
#ifdef ST_SYNCHRONOUS	/* Writes are synced at once.  */
				if (FileSystemFlags & ST_SYNCHRONOUS)
					strOutStr += ",sync";
#endif
#ifdef ST_MANDLOCK		/* Allow mandatory locks on an FS.  */
				if (FileSystemFlags & ST_MANDLOCK)
					strOutStr += ",mandlock";
#endif
#ifdef ST_WRITE			/* Write on file/directory/symlink.  */
				if (FileSystemFlags & ST_WRITE)
					strOutStr += ",write";
#endif
#ifdef ST_APPEND		/* Append-only file.  */
				if (FileSystemFlags & ST_APPEND)
					strOutStr += ",append";
#endif
#ifdef ST_IMMUTABLE		/* Immutable file.  */
				if (FileSystemFlags & ST_IMMUTABLE)
					strOutStr += ",immutable";
#endif
#ifdef ST_NOATIME		/* Do not update access times.  */
				if (FileSystemFlags & ST_NOATIME)
					strOutStr += ",noatime";
#endif
#ifdef ST_NODIRATIME	/* Do not update directory access times.  */
				if (FileSystemFlags & ST_NODIRATIME)
					strOutStr += ",nodiratime";
#endif
#ifdef ST_RELATIME		/* Update atime relative to mtime/ctime.  */
				if (FileSystemFlags & ST_RELATIME)
					strOutStr += ",relatime";
#endif
#ifdef ST_NOSYMFOLLOW
				if (FileSystemFlags & ST_NOSYMFOLLOW)
					strOutStr += ",nosymfollow";
#endif
				if (!strOutStr.IsEmpty()) {
					GotoXY(X1 + 2, CurY++);
					PrintText(Msg::InfoDiskFlags);
					PrintInfo(strOutStr);
				}
			}

		}
	}
	/* 2b - plugin */
	else {
		DrawTitle(Msg::InfoPluginTitle, ILSS_DISKINFO, CurY++);
		if (SectionState[ILSS_DISKINFO].Show) {
			GotoXY(X1 + 2, CurY++);
			PrintText(Msg::InfoPluginStartDir);
			PrintInfo(strCurDir);

			HANDLE hPlugin = AnotherPanel->GetPluginHandle();
			if (hPlugin != INVALID_HANDLE_VALUE) {
				PluginHandle *ph = (PluginHandle *)hPlugin;
				GotoXY(X1 + 2, CurY++);
				PrintText(Msg::InfoPluginModuleName);
				PrintInfo(PointToName(ph->pPlugin->GetModuleName()));
			}

			OpenPluginInfo Info;
			AnotherPanel->GetOpenPluginInfo(&Info);

			if (Info.HostFile != nullptr && *Info.HostFile != L'\0' ) {
				GotoXY(X1 + 2, CurY++);
				PrintText(Msg::InfoPluginHostFile);
				PrintInfo(Info.HostFile);
			}

			if (Info.CurDir != nullptr && *Info.CurDir != L'\0' ) {
				GotoXY(X1 + 2, CurY++);
				PrintText(Msg::InfoPluginCurDir);
				PrintInfo(Info.CurDir);
			}

			if (Info.PanelTitle != nullptr && *Info.PanelTitle != L'\0' ) {
				GotoXY(X1 + 2, CurY++);
				PrintText(Msg::InfoPluginPanelTitle);
				PrintInfo(Info.PanelTitle);
			}

			if (Info.Format != nullptr && *Info.Format != L'\0' ) {
				GotoXY(X1 + 2, CurY++);
				PrintText(Msg::InfoPluginFormat);
				PrintInfo(Info.Format);
			}

			if (Info.ShortcutData != nullptr && *Info.ShortcutData != L'\0' ) {
				GotoXY(X1 + 2, CurY++);
				PrintText(Msg::InfoPluginShortcutData);
				PrintInfo(Info.ShortcutData);
			}
		}
	}

	/* #3 - memory info */
	DrawTitle(Msg::InfoMemory, ILSS_MEMORYINFO, CurY++);
	if (SectionState[ILSS_MEMORYINFO].Show) {

#ifdef __APPLE__
		unsigned long long totalram;
		vm_size_t page_size;
		unsigned long long freeram;
		int ret_sc;

		// ret_sc =  (sysctlbyname("hw.memsize", &totalram, &ulllen, NULL, 0) ? 1 : 0);
		ret_sc = (KERN_SUCCESS != _host_page_size(mach_host_self(), &page_size) ? 1 : 0);

		mach_msg_type_number_t count = HOST_VM_INFO_COUNT;
		vm_statistics_data_t vmstat;

		ret_sc+= (KERN_SUCCESS != host_statistics(mach_host_self(), HOST_VM_INFO, (host_info_t)&vmstat, &count)
						? 1
						: 0);
		totalram =
				(vmstat.wire_count + vmstat.active_count + vmstat.inactive_count + vmstat.free_count) * page_size;
		freeram = vmstat.free_count * page_size;

		// double total = vmstat.wire_count + vmstat.active_count + vmstat.inactive_count + vmstat.free_count;
		// double wired = vmstat.wire_count;
		// double active = vmstat.active_count;
		// double inactive = vmstat.inactive_count;
		// double free = vmstat.free_count;

		if (!ret_sc) {
			DWORD dwMemoryLoad = 100 - ToPercent64(freeram, totalram);

			GotoXY(X1 + 2, CurY++);
			PrintText(Msg::InfoMemoryLoad);
			strOutStr.Format(L"%d%%", dwMemoryLoad);
			PrintInfo(strOutStr);

			GotoXY(X1 + 2, CurY++);
			PrintText(Msg::InfoMemoryTotal);
			InsertCommas(totalram, strOutStr);
			PrintInfo(strOutStr);

			GotoXY(X1 + 2, CurY++);
			PrintText(Msg::InfoMemoryFree);
			InsertCommas(freeram, strOutStr);
			PrintInfo(strOutStr);
		}

#elif !defined(__FreeBSD__)&& !defined(__NetBSD__) && !defined(__DragonFly__) && !defined(__HAIKU__)
		struct sysinfo si = {};
		if (sysinfo(&si) == 0) {
			DWORD dwMemoryLoad = 100 - ToPercent64(si.freeram + si.freeswap, si.totalram + si.totalswap);

			GotoXY(X1 + 2, CurY++);
			PrintText(Msg::InfoMemoryLoad);
			strOutStr.Format(L"%d%%", dwMemoryLoad);
			PrintInfo(strOutStr);

			GotoXY(X1 + 2, CurY++);
			PrintText(Msg::InfoMemoryTotal);
			InsertCommas(si.totalram, strOutStr);
			PrintInfo(strOutStr);

			GotoXY(X1 + 2, CurY++);
			PrintText(Msg::InfoMemoryFree);
			InsertCommas(si.freeram, strOutStr);
			PrintInfo(strOutStr);

			GotoXY(X1 + 2, CurY++);
			PrintText(Msg::InfoSharedMemory);
			InsertCommas(si.sharedram, strOutStr);
			PrintInfo(strOutStr);

			GotoXY(X1 + 2, CurY++);
			PrintText(Msg::InfoBufferMemory);
			InsertCommas(si.bufferram, strOutStr);
			PrintInfo(strOutStr);

			GotoXY(X1 + 2, CurY++);
			PrintText(Msg::InfoPageFileTotal);
			InsertCommas(si.totalswap, strOutStr);
			PrintInfo(strOutStr);

			if (si.totalswap != 0 ) {
				GotoXY(X1 + 2, CurY++);
				PrintText(Msg::InfoPageFileFree);
				InsertCommas(si.freeswap, strOutStr);
				PrintInfo(strOutStr);
			}
		}
#endif
	}

	/* #4 - .editorconfig info */
	ShowEditorConfig(CurY);

	/* #5 - git status */
	ShowGitStatus(CurY);

	/* #6 - dir description */
	ShowDirDescription(CurY);

	/* #7 - plugin description */
	ShowPluginDescription(CurY);
}

int64_t InfoList::VMProcess(MacroOpcode OpCode, void *vParam, int64_t iParam)
{
	if (DizView)
		return DizView->VMProcess(OpCode, vParam, iParam);

	switch (OpCode) {
		case MCODE_C_EMPTY:
			return 1;
	}

	return 0;
}

int InfoList::ProcessKey(FarKey Key)
{
	if (!IsVisible())
		return FALSE;

	if (Key >= KEY_RCTRL0 && Key <= KEY_RCTRL9) {
		ExecShortcutFolder(Key - KEY_RCTRL0);
		return TRUE;
	}

	switch (Key) {
		case KEY_F1: {
			Help::Present(L"InfoPanel");
		}
			return TRUE;
		case KEY_F3:
		case KEY_NUMPAD5:
		case KEY_SHIFTNUMPAD5:

			if (!strDizFileName.IsEmpty()) {
				CtrlObject->Cp()->GetAnotherPanel(this)->GetCurDir(strCurDir);
				FarChDir(strCurDir);
				new FileViewer(std::make_shared<FileHolder>(strDizFileName), TRUE);	// OT
			}

			CtrlObject->Cp()->Redraw();
			return TRUE;
		case KEY_F4:
			/*
				$ 30.04.2001 DJ
				не показываем редактор, если ничего не задано в именах файлов;
				не редактируем имена описаний со звездочками;
				убираем лишнюю перерисовку панелей
			*/
			{
				Panel *AnotherPanel = CtrlObject->Cp()->GetAnotherPanel(this);
				AnotherPanel->GetCurDir(strCurDir);
				FarChDir(strCurDir);

				if (!strDizFileName.IsEmpty()) {
					new FileEditor(std::make_shared<FileHolder>(strDizFileName), CP_AUTODETECT, FFILEEDIT_ENABLEF6);
				} else if (!Opt.InfoPanel.strFolderInfoFiles.IsEmpty()) {
					FARString strArgName;
					const wchar_t *p = Opt.InfoPanel.strFolderInfoFiles;

					while ((p = GetCommaWord(p, strArgName))) {
						if (!strArgName.ContainsAnyOf("*?")) {
							new FileEditor(std::make_shared<FileHolder>(strArgName), CP_AUTODETECT,
									FFILEEDIT_CANNEWFILE | FFILEEDIT_ENABLEF6);
							break;
						}
					}
				}

				AnotherPanel->Update(UPDATE_KEEP_SELECTION | UPDATE_SECONDARY);
				// AnotherPanel->Redraw();
				Update(0);
			}
			CtrlObject->Cp()->Redraw();
			return TRUE;
		case KEY_CTRLR:
			Redraw();
			return TRUE;
	}

	/*
		$ 30.04.2001 DJ
		обновляем кейбар после нажатия F8, F2 или Shift-F2
	*/
	if (DizView && Key >= 256) {
		int ret = DizView->ProcessKey(Key);

		if (Key == KEY_F8 || Key == KEY_F2 || Key == KEY_SHIFTF2) {
			DynamicUpdateKeyBar();
			CtrlObject->MainKeyBar->Redraw();
		}

		if (Key == KEY_F7 || Key == KEY_SHIFTF7) {
			int64_t Pos, Length;
			DWORD Flags;
			DizView->GetSelectedParam(Pos, Length, Flags);
			// ShellUpdatePanels(nullptr,FALSE);
			DizView->InRecursion++;
			Redraw();
			CtrlObject->Cp()->GetAnotherPanel(this)->Redraw();
			DizView->SelectText(Pos, Length, Flags | 1);
			DizView->InRecursion--;
		}

		return (ret);
	}

	return FALSE;
}

int InfoList::ProcessMouse(MOUSE_EVENT_RECORD *MouseEvent)
{
	int RetCode;

	if ((MouseEvent->dwButtonState & FROM_LEFT_1ST_BUTTON_PRESSED)
			&& (MouseEvent->dwEventFlags != MOUSE_MOVED))
	{
		for (auto &elem : SectionState)
		{
			if (elem.Y<0 || MouseEvent->dwMousePosition.Y != elem.Y)
				continue;

			elem.Show = !elem.Show;
			Redraw();
			return TRUE;
		}
	}

	if (Panel::PanelProcessMouse(MouseEvent, RetCode))
		return (RetCode);

	if (MouseEvent->dwMousePosition.Y >= 14 && DizView) {
		_tran(SysLog(L"InfoList::ProcessMouse() DizView = %p", DizView));
		int DVX1, DVX2, DVY1, DVY2;
		DizView->GetPosition(DVX1, DVY1, DVX2, DVY2);

		if ((MouseEvent->dwButtonState & FROM_LEFT_1ST_BUTTON_PRESSED)
				&& MouseEvent->dwMousePosition.X > DVX1 + 1
				&& MouseEvent->dwMousePosition.X < DVX2 - DizView->GetShowScrollbar() - 1
				&& MouseEvent->dwMousePosition.Y > DVY1 + 1 && MouseEvent->dwMousePosition.Y < DVY2 - 1) {
			ProcessKey(KEY_F3);
			return TRUE;
		}

		if (MouseEvent->dwButtonState & RIGHTMOST_BUTTON_PRESSED) {
			ProcessKey(KEY_F4);
			return TRUE;
		}
	}

	SetFocus();

	if (DizView)
		return (DizView->ProcessMouse(MouseEvent));

	return TRUE;
}

void InfoList::PrintText(const wchar_t *Str)
{
	if (WhereY() <= Y2 - 1) {
		FS << fmt::Cells() << fmt::Truncate(X2 - WhereX()) << Str;
	}
}

void InfoList::PrintText(FarLangMsg MsgID)
{
	PrintText(MsgID.CPtr());
}

void InfoList::PrintInfo(const wchar_t *str)
{
	if (WhereY() > Y2 - 1)
		return;

	const auto SaveColor = GetColor();
	int MaxLength = X2 - WhereX() - 2;

	if (MaxLength < 0)
		MaxLength = 0;

	FARString strStr = str;
	TruncStr(strStr, MaxLength);
	int Length = (int)strStr.GetLength();
	int NewX = X2 - Length - 1;

	if (NewX > X1 && NewX > WhereX()) {
		GotoXY(NewX, WhereY());
		SetFarColor(COL_PANELINFOTEXT);
		FS << strStr << L" ";
		SetColor(SaveColor);
	}
}

void InfoList::PrintInfo(FarLangMsg MsgID)
{
	PrintInfo(MsgID.CPtr());
}

void InfoList::ShowEditorConfig(int &YPos)
{
	Panel *AnotherPanel = CtrlObject->Cp()->GetAnotherPanel(this);

	if (AnotherPanel->GetMode() != FILE_PANEL)
		return;

	FARString str;
	AnotherPanel->GetCurDir(str);
	str += GOOD_SLASH;

	EditorConfigOrg EdCfg;
	EdCfg.Populate(str.GetMB().c_str());

	if( EdCfg.pos_trim_dir_nearest >= 0) {
		// text " EditorConfig " in separator
		DrawTitle(L"EditorConfig", ILSS_EdCfgINFO, YPos++);
		if (SectionState[ILSS_EdCfgINFO].Show) {
			// print .editorconfig nearest dir
			if (EdCfg.pos_trim_dir_nearest != EdCfg.pos_trim_dir_root) {
				str.Truncate(EdCfg.pos_trim_dir_nearest);
				GotoXY(X1 + 2, YPos++);
				PrintText(Msg::InfoEdCfgNearestDir);
				PrintInfo(str);
			}
			// print .editorconfig root dir
			str.Truncate(EdCfg.pos_trim_dir_root);
			GotoXY(X1 + 2, YPos++);
			PrintText(Msg::InfoEdCfgRootDir);
			PrintInfo(str);
			// print result of .editorconfig for mask [*]
			if (EdCfg.ExpandTabs >= 0) {
				GotoXY(X1 + 2, YPos++);
				PrintText(L"[*] indent_style");
				PrintInfo(EdCfg.ExpandTabs==EXPAND_NOTABS ? L"tab"
					: EdCfg.ExpandTabs==EXPAND_NEWTABS ? L"space"
					: L"????");
			}
			if (EdCfg.TabSize > 0) {
				GotoXY(X1 + 2, YPos++);
				PrintText(L"[*] indent_size");
				str.Format(L"%d", EdCfg.TabSize);
				PrintInfo(str);
			}
			if (EdCfg.EndOfLine) {
				GotoXY(X1 + 2, YPos++);
				PrintText(L"[*] end_of_line");
				PrintInfo(wcscmp(EdCfg.EndOfLine,L"\n")==0 ? L"lf (\"\\n\")"
					: wcscmp(EdCfg.EndOfLine,L"\r")==0 ? L"cr (\"\\r\")"
					: wcscmp(EdCfg.EndOfLine,L"\r\n")==0 ? L"crlf (\"\\r\\n\")"
					: L"????");
			}
			if (EdCfg.CodePage > 0) {
				GotoXY(X1 + 2, YPos++);
				PrintText(L"[*] charset");
				ShortReadableCodepageName(EdCfg.CodePage,str);
				if (EdCfg.CodePageBOM > 0)
					str += L"-BOM";
				PrintInfo(str);
			}
			if (EdCfg.TrimTrailingWhitespace >= 0) {
				GotoXY(X1 + 2, YPos++);
				PrintText(L"[*] trim_trailing_whitespace");
				PrintInfo(EdCfg.TrimTrailingWhitespace==1 ? L"true"
					: EdCfg.TrimTrailingWhitespace==0 ? L"false"
					: L"????");
			}
			if (EdCfg.InsertFinalNewline >= 0) {
				GotoXY(X1 + 2, YPos++);
				PrintText(L"[*] insert_final_newline");
				PrintInfo(EdCfg.InsertFinalNewline==1 ? L"true"
					: EdCfg.InsertFinalNewline==0 ? L"false"
					: L"????");
			}
		}
	}
}

void InfoList::ShowGitStatus(int &YPos)
{
	Panel *AnotherPanel = CtrlObject->Cp()->GetAnotherPanel(this);

	if (AnotherPanel->GetMode() != FILE_PANEL)
		return;

	FARString strDir;
	AnotherPanel->GetCurDir(strDir);
	do {
		FARString strGit = strDir + L"/.git";
		struct stat s;
		if (stat(strGit.GetMB().c_str(), &s) == 0) {
			fprintf(stderr, "GIT: %ls\n", strGit.CPtr());
			std::vector<std::wstring> lines;
			std::string cmd = "git -C \"";
			cmd+= EscapeCmdStr(Wide2MB(strDir.CPtr()));
			cmd+= "\" status -s -b";

			if (POpen(lines, cmd.c_str())) {
				// text " git status " in separator
				DrawTitle(L"git status -s -b", ILSS_GITINFO, YPos++);
				if (SectionState[ILSS_GITINFO].Show) {
					// print git root dir
					GotoXY(X1 + 2, YPos++);
					PrintText(Msg::InfoGitRootDir);
					PrintInfo(strDir);
					// print result of git status
					for (const auto &l : lines) {
						GotoXY(X1 + 2, YPos++);
						PrintText(l.c_str());
					}
				}
			}
			break;
		}
	} while (CutToSlash(strDir, true));
}

void InfoList::ShowDirDescription(int YPos)
{
	Panel *AnotherPanel = CtrlObject->Cp()->GetAnotherPanel(this);
	if (AnotherPanel->GetMode() != FILE_PANEL)
		return;

	DrawTitle(nullptr, -1/*ILSS_DIRDESCRIPTION*/, YPos);

	FARString strDir;
	AnotherPanel->GetCurDir(strDir);

	if (!strDir.IsEmpty())
		AddEndSlash(strDir);

	FARString strArgName;
	const wchar_t *NamePtr = Opt.InfoPanel.strFolderInfoFiles;

	while ((NamePtr = GetCommaWord(NamePtr, strArgName))) {
		FARString strFullDizName;
		strFullDizName = strDir;
		strFullDizName+= strArgName;
		FAR_FIND_DATA_EX FindData;

		if (!apiGetFindDataEx(strFullDizName, FindData, FIND_FILE_FLAG_CASE_INSENSITIVE)) {
			continue;
		}

		CutToSlash(strFullDizName, false);
		strFullDizName+= FindData.strFileName;

		if (OpenDizFile(strFullDizName, YPos))
			return;
	}

	CloseFile();

	strDir = L" ";
	strDir += Msg::InfoDescription;
	strDir += L" ";
	TruncStr(strDir, X2 - X1 - 3);
	GotoXY(X1 + (X2 - X1 - (int)strDir.GetLength()) / 2, YPos++);
	SetFarColor(COL_PANELTEXT);
	PrintText(strDir);

	SetFarColor(COL_PANELTEXT);
	GotoXY(X1 + 2, YPos);
	PrintText(Msg::InfoDizAbsent);
}

void InfoList::ShowPluginDescription(int YPos)
{
	Panel *AnotherPanel = CtrlObject->Cp()->GetAnotherPanel(this);

	if (AnotherPanel->GetMode() != PLUGIN_PANEL)
		return;

	OpenPluginInfo Info;
	AnotherPanel->GetOpenPluginInfo(&Info);

	if (Info.InfoLinesNumber <= 0) {
		DrawTitle(Msg::InfoPluginDescription, -1/*ILSS_DIRDESCRIPTION*/, YPos++);
		GotoXY(X1 + 2, YPos);
		PrintText(Msg::InfoPluginAbsent);
		return;
	}

	if (!Info.InfoLines[0].Separator) // show default, if plugin info start without its own separator
		DrawTitle(Msg::InfoPluginDescription, -1/*ILSS_DIRDESCRIPTION*/, YPos++);

	for (int I = 0; I < Info.InfoLinesNumber && YPos < Y2; I++, YPos++) {
		const InfoPanelLine *InfoLine = &Info.InfoLines[I];
		GotoXY(X1 + 2, YPos);

		if (InfoLine->Separator) {
			FARString strTitle;

			if (InfoLine->Text && *InfoLine->Text)
				strTitle.Append(L" ").Append(InfoLine->Text).Append(L" ");

			DrawSeparator(YPos);
			TruncStr(strTitle, X2 - X1 - 3);
			GotoXY(X1 + (X2 - X1 - (int)strTitle.GetLength()) / 2, YPos);
			SetFarColor(COL_PANELTEXT);
			PrintText(strTitle);
		} else {
			SetFarColor(COL_PANELTEXT);
			PrintText(NullToEmpty(InfoLine->Text));
			PrintInfo(NullToEmpty(InfoLine->Data));
		}
	}
}

void InfoList::CloseFile()
{
	if (DizView) {
		if (DizView->InRecursion)
			return;

		LastDizWrapMode = DizView->GetWrapMode();
		LastDizWrapType = DizView->GetWrapType();
		LastDizShowScrollbar = DizView->GetShowScrollbar();
		DizView->SetWrapMode(OldWrapMode);
		DizView->SetWrapType(OldWrapType);
		delete DizView;
		DizView = nullptr;
	}

	strDizFileName.Clear();
}

int InfoList::OpenDizFile(const wchar_t *DizFile, int YPos)
{
	bool bOK = true;
	_tran(SysLog(L"InfoList::OpenDizFile([%ls]", DizFile));

	if (!DizView) {
		DizView = new (std::nothrow) DizViewer;

		if (!DizView)
			return FALSE;

		_tran(SysLog(L"InfoList::OpenDizFile() create new Viewer = %p", DizView));
		DizView->SetRestoreScreenMode(FALSE);
		DizView->SetPosition(X1 + 1, YPos + 1, X2 - 1, Y2 - 1);
		DizView->SetStatusMode(0);
		DizView->EnableHideCursor(0);
		OldWrapMode = DizView->GetWrapMode();
		OldWrapType = DizView->GetWrapType();
		DizView->SetWrapMode(LastDizWrapMode);
		DizView->SetWrapType(LastDizWrapType);
		DizView->SetShowScrollbar(LastDizShowScrollbar);
	} else {
		// не будем менять внутренности если мы посреди операции со вьювером.
		bOK = !DizView->InRecursion;
	}

	if (bOK) {
		if (!DizView->OpenFile(std::make_shared<FileHolder>(DizFile), FALSE)) {
			delete DizView;
			DizView = nullptr;
			return FALSE;
		}

		strDizFileName = DizFile;
	}

	DizView->Show();
	FARString strTitle;
	strTitle.Append(L" ").Append(PointToName(strDizFileName)).Append(L" ");
	TruncStr(strTitle, X2 - X1 - 3);
	GotoXY(X1 + (X2 - X1 - (int)strTitle.GetLength()) / 2, YPos);
	SetFarColor(COL_PANELTEXT);
	PrintText(strTitle);
	return TRUE;
}

void InfoList::SetFocus()
{
	Panel::SetFocus();
	SetMacroMode(FALSE);
}

void InfoList::KillFocus()
{
	Panel::KillFocus();
	SetMacroMode(TRUE);
}

void InfoList::SetMacroMode(int Restore)
{
	if (!CtrlObject)
		return;

	if (PrevMacroMode == -1)
		PrevMacroMode = CtrlObject->Macro.GetMode();

	CtrlObject->Macro.SetMode(Restore ? PrevMacroMode : MACRO_INFOPANEL);
}

int InfoList::GetCurName(FARString &strName)
{
	strName = strDizFileName;
	return (TRUE);
}

BOOL InfoList::UpdateKeyBar()
{
	KeyBar *KB = CtrlObject->MainKeyBar;
	KB->SetAllGroup(KBL_MAIN, Msg::InfoF1, 12);
	KB->SetAllGroup(KBL_SHIFT, Msg::InfoShiftF1, 12);
	KB->SetAllGroup(KBL_ALT, Msg::InfoAltF1, 12);
	KB->SetAllGroup(KBL_CTRL, Msg::InfoCtrlF1, 12);
	KB->SetAllGroup(KBL_CTRLSHIFT, Msg::InfoCtrlShiftF1, 12);
	KB->SetAllGroup(KBL_CTRLALT, Msg::InfoCtrlAltF1, 12);
	KB->SetAllGroup(KBL_ALTSHIFT, Msg::InfoAltShiftF1, 12);
	KB->SetAllGroup(KBL_CTRLALTSHIFT, Msg::InfoCtrlAltShiftF1, 12);
	DynamicUpdateKeyBar();
	return TRUE;
}

void InfoList::DynamicUpdateKeyBar()
{
	KeyBar *KB = CtrlObject->MainKeyBar;

	if (DizView) {
		KB->Change(Msg::InfoF3, 3 - 1);

		if (DizView->GetCodePage() != WINPORT(GetOEMCP)())
			KB->Change(Msg::ViewF8DOS, 7);
		else
			KB->Change(Msg::InfoF8, 7);

		if (!DizView->GetWrapMode()) {
			if (DizView->GetWrapType())
				KB->Change(Msg::ViewShiftF2, 2 - 1);
			else
				KB->Change(Msg::ViewF2, 2 - 1);
		} else
			KB->Change(Msg::ViewF2Unwrap, 2 - 1);

		if (DizView->GetWrapType())
			KB->Change(KBL_SHIFT, Msg::ViewF2, 2 - 1);
		else
			KB->Change(KBL_SHIFT, Msg::ViewShiftF2, 2 - 1);
	} else {
		KB->Change(Msg::F2, 2 - 1);
		KB->Change(KBL_SHIFT, L"", 2 - 1);
		KB->Change(L"", 3 - 1);
		KB->Change(L"", 8 - 1);
		KB->Change(KBL_SHIFT, L"", 8 - 1);
		KB->Change(KBL_ALT, Msg::AltF8, 8 - 1);		// стандартный для панели - "хистори"
	}

	KB->ReadRegGroup(L"Info", Opt.strLanguage);
	KB->SetAllRegGroup();
}
