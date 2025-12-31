/*
fileedit2options.cpp

Фаровское горизонтальное меню (вызов hmenu.cpp с конкретными параметрами) для редактора
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

#include "options.hpp"
#include "lang.hpp"
#include "keys.hpp"
#include "hmenu.hpp"
#include "vmenu.hpp"
#include "config.hpp"
#include "ConfigOptEdit.hpp"
#include "ConfigOptSaveLoad.hpp"
#include "strmix.hpp"
#include "interf.hpp"
#include "manager.hpp"
#include "ctrlobj.hpp"
#include "fileedit.hpp"

#include "fileedit2options.hpp"

/*
File:
------------------------------------------------
Open...									ShiftF4
Save									F2
Save as...								ShiftF2
Save quetly								ShiftF10
Recent list...							AltF11
------------------------------------------------
Print...								AltF5
Printer settings...
------------------------------------------------
Go to file in Panel				        CtrlF10
Exit							        F10
------------------------------------------------
Help									F1
Plugins...								F11
Screen list...							F12
Configuration...						AltShiftF9
Exit far2l								AltF10


Edit:
------------------------------------------------
Undo									CtrlZ
Redo									CtrlShiftZ
------------------------------------------------
Select all								CtrlA
Select vertical block					AltShift+Arrows
------------------------------------------------
Cut										CtrlX
Copy									CtrlC
Paste									CtrlV
Delete									CtrlBackspace
Вelete line								CtrlY
Delete leading spaces from line			CtrlDel
Unmark macro block						CtrlU
Delete to the end of line				CtrlK
------------------------------------------------
Search...								F7
Replace...								CtrlF7
Search next								ShiftF7
Search previous							AltF7
------------------------------------------------
Type short file name					ShiftEnter
Type full file name						CtrlF
Insert path from left panel				CtrlAlt(
Insert path from right panel			CtrlAlt)
Insert path from active panel			ShiftAlt(
Insert path from passive panel			ShiftAlt)
Block left								AltU
Block right								AltI
------------------------------------------------


Navigate
------------------------------------------------
Go to line...							AltF8
Go to top								CtrlN
Go to bottom							CtrlE
Go to begin of file						CtrlHome
Go to end of file						CtrlEnd
------------------------------------------------
Go to bookmark 0						Ctrl0
Go to bookmark 1						Ctrl1
Go to bookmark 2						Ctrl2
Go to bookmark 3						Ctrl3
Go to bookmark 4						Ctrl4
Go to bookmark 5						Ctrl5
Go to bookmark 6						Ctrl6
Go to bookmark 7						Ctrl7
Go to bookmark 8						Ctrl8
Go to bookmark 9						Ctrl9
------------------------------------------------
Pin bookmark 0							CtrlShift0
Pin bookmark 1							CtrlShift1
Pin bookmark 2							CtrlShift2
Pin bookmark 3							CtrlShift3
Pin bookmark 4							CtrlShift4
Pin bookmark 5							CtrlShift5
Pin bookmark 6							CtrlShift6
Pin bookmark 7							CtrlShift7
Pin bookmark 8							CtrlShift8
Pin bookmark 9							CtrlShift9
------------------------------------------------


View:
------------------------------------------------
Show/Hide key bar						CtrlB
Show/Hide title bar						CtrlShiftB
Toggle word wrap						F3
Toggle limne numbers					CtrlF3
Toggle white spaces						F5
Toggle tabs to spaces					CtrlF5
Toggle insert/overtype mode				Insert
Toggle full screen						AltF9
Toggle lock mode						CtrlL
Toggle console							CtrlO
------------------------------------------------
Change tab size							ShiftF5
Toggle code page UTF8-ANSI-OEM			F8
Change code page...						ShiftF8
Switch to viewer						F6
------------------------------------------------

*/

void EditorShellOptions(int LastCommand, MOUSE_EVENT_RECORD *MouseEvent, FileEditor* fileEditor)
{
	MenuDataEx FileMenu[] = {
		{Msg::EditorMenuFileOpen,	0,	KEY_SHIFTF4  },
		{Msg::EditorMenuFileSave,	0,	KEY_F2  },
		{Msg::EditorMenuFileSaveAs,	0,	KEY_SHIFTF2  },
		{Msg::EditorMenuFileSaveQ,	0,	KEY_SHIFTF10  },
		{Msg::EditorMenuFileHistory,	0,	KEY_ALTF11  },
		{L"", LIF_SEPARATOR, 0  },
		{Msg::EditorMenuFilePrint,	0,	KEY_ALTF5  },
		{Msg::EditorMenuFilePrinter,	0,	0  },
		{L"", LIF_SEPARATOR, 0  },
		{Msg::EditorMenuFileGoToPanel,	0,	KEY_CTRLF10  },
		{Msg::EditorMenuFileExit,	0,	KEY_F10  },
		{L"", LIF_SEPARATOR, 0  },
		{Msg::EditorMenuFileHelp,	0,	KEY_F1  },
		{Msg::EditorMenuFilePlugins,	0,	KEY_F11  },
		{Msg::EditorMenuFileScreens,	0,	KEY_F12  },
		{Msg::EditorMenuFileOptions,	0,	KEY_ALTSHIFTF9  },
		{Msg::EditorMenuFileExitFar,	0,	KEY_ALTF10  },
	};

	MenuDataEx EditMenu[] = {
		{Msg::EditorMenuEditUndo,	0,	KEY_CTRLZ  },
		{Msg::EditorMenuEditRedo,	0,	KEY_CTRLSHIFTZ  },
		{L"", LIF_SEPARATOR, 0  },
		{Msg::EditorMenuEditSelectAll,	0,	KEY_CTRLA  },
		{Msg::EditorMenuEditSelectVertical,	0,	KEY_ALTSHIFTDOWN  },
		{L"", LIF_SEPARATOR, 0  },
		{Msg::EditorMenuEditCut,	0,	KEY_CTRLX  },
		{Msg::EditorMenuEditCopy,	0,	KEY_CTRLC  },
		{Msg::EditorMenuEditPaste,	0,	KEY_CTRLV  },
		{Msg::EditorMenuEditDelete,	0,	KEY_CTRLBS  },
		{Msg::EditorMenuEditDelLine,	0,	KEY_CTRLY  },
		{Msg::EditorMenuEditDeleteLeadSpaces,	0,	KEY_CTRLDEL  },
		{Msg::EditorMenuEditDeleteEOL,	0,	KEY_CTRLK  },
		{L"", LIF_SEPARATOR, 0  },
		{Msg::EditorMenuEditFind,	0,	KEY_F7  },
		{Msg::EditorMenuEditFindReplace,	0,	KEY_CTRLF7  },
		{Msg::EditorMenuEditFindNext,	0,	KEY_SHIFTF7  },
		{Msg::EditorMenuEditFindPrev,	0,	KEY_ALTF7  },
		{L"", LIF_SEPARATOR, 0  },
		{Msg::EditorMenuEditShortFName,	0,	KEY_SHIFTENTER  },
		{Msg::EditorMenuEditLongFName,	0,	KEY_CTRLF  },
		{Msg::EditorMenuEditLeftPaneName,	0,	KEY_CTRLALTBRACKET  },
		{Msg::EditorMenuEditRightPanel,	0,	KEY_CTRLALTBACKBRACKET  },
		{Msg::EditorMenuEditActivePanel,	0,	KEY_ALTSHIFTBRACKET  },
		{Msg::EditorMenuEditInactivePanel,	0,	KEY_ALTSHIFTBACKBRACKET  },
		{Msg::EditorMenuEditBlockIndent,	0,	KEY_ALTU  },
		{Msg::EditorMenuEditBlockUnindent,	0,	KEY_ALTI  },
	};

	MenuDataEx NavigateMenu[] = {
		{Msg::EditorMenuNavigateGoToLine,	0,	KEY_ALTF8  },
		{Msg::EditorMenuNavigateTop,	0,	KEY_CTRLN  },
		{Msg::EditorMenuNavigateBottom,	0,	KEY_CTRLE  },
		{Msg::EditorMenuNavigateBegin,	0,	KEY_CTRLHOME  },
		{Msg::EditorMenuNavigateEnd,	0,	KEY_CTRLEND  },
		{L"", LIF_SEPARATOR, 0  },
		{Msg::EditorMenuNavigateBm0,	0,	KEY_CTRL0  },
		{Msg::EditorMenuNavigateBm1,	0,	KEY_CTRL1  },
		{Msg::EditorMenuNavigateBm2,	0,	KEY_CTRL2  },
		{Msg::EditorMenuNavigateBm3,	0,	KEY_CTRL3  },
		{Msg::EditorMenuNavigateBm4,	0,	KEY_CTRL4  },
		{Msg::EditorMenuNavigateBm5,	0,	KEY_CTRL5  },
		{Msg::EditorMenuNavigateBm6,	0,	KEY_CTRL6  },
		{Msg::EditorMenuNavigateBm7,	0,	KEY_CTRL7  },
		{Msg::EditorMenuNavigateBm8,	0,	KEY_CTRL8  },
		{Msg::EditorMenuNavigateBm9,	0,	KEY_CTRL9  },
		{L"", LIF_SEPARATOR, 0  },
		{Msg::EditorMenuNavigatePin0,	0,	KEY_CTRLSHIFT0  },
		{Msg::EditorMenuNavigatePin1,	0,	KEY_CTRLSHIFT1  },
		{Msg::EditorMenuNavigatePin2,	0,	KEY_CTRLSHIFT2  },
		{Msg::EditorMenuNavigatePin3,	0,	KEY_CTRLSHIFT3  },
		{Msg::EditorMenuNavigatePin4,	0,	KEY_CTRLSHIFT4  },
		{Msg::EditorMenuNavigatePin5,	0,	KEY_CTRLSHIFT5  },
		{Msg::EditorMenuNavigatePin6,	0,	KEY_CTRLSHIFT6  },
		{Msg::EditorMenuNavigatePin7,	0,	KEY_CTRLSHIFT7  },
		{Msg::EditorMenuNavigatePin8,	0,	KEY_CTRLSHIFT8  },
		{Msg::EditorMenuNavigatePin9,	0,	KEY_CTRLSHIFT9  },
	};

	MenuDataEx ViewMenu[] = {
		{Msg::EditorMenuViewKeyBar,	0,	KEY_CTRLB  },
		{Msg::EditorMenuViewTitleBar,	0,	KEY_CTRLSHIFTB  },
		{Msg::EditorMenuViewWordWrap,	0,	KEY_F3  },
		{Msg::EditorMenuViewNumbers,	0,	KEY_CTRLF3  },
		{Msg::EditorMenuViewSpaces,	0,	KEY_F5  },
		{Msg::EditorMenuViewTabs,	0,	KEY_CTRLF5  },
		{Msg::EditorMenuViewOvertype,	0,	KEY_INS  },
		{Msg::EditorMenuViewFullScreen,	0,	KEY_ALTF9  },
		{Msg::EditorMenuViewLock,	0,	KEY_CTRLL  },
		{Msg::EditorMenuViewConsole,	0,	KEY_CTRLO  },
		{L"", LIF_SEPARATOR, 0  },
		{Msg::EditorMenuViewTabSize,	0,	KEY_SHIFTF5  },
		{Msg::EditorMenuViewOem,	0,	KEY_F8  },
		{Msg::EditorMenuViewCodepage,	0,	KEY_SHIFTF8  },
		{Msg::EditorMenuViewViewer,	0,	KEY_F6  },
	};

	HMenuData MainMenu[] = {
		{Msg::EditorMenuFileTitle,     1, FileMenu,    ARRAYSIZE(FileMenu),    L"FileMenu"},
		{Msg::EditorMenuEditTitle,    0, EditMenu,   ARRAYSIZE(EditMenu),   L"EditMenu" },
		{Msg::EditorMenuNavigateTitle, 0, NavigateMenu,     ARRAYSIZE(NavigateMenu),     L"NavigateMenu" },
		{Msg::EditorMenuViewTitle,  0, ViewMenu, ARRAYSIZE(ViewMenu), L"ViewMenu" }
	};

	static int LastHItem = -1, LastVItem = 0;
	int HItem, VItem;

	fprintf(stderr, "editor menu: start\n");

	// Навигация по меню
	{
		HMenu HOptMenu(MainMenu, ARRAYSIZE(MainMenu));
		HOptMenu.SetHelp(L"Menus");
		int gap = Opt.EdOpt.ShowTitleBar && Opt.EdOpt.ShowMenuBar ? 1 : 0;
		HOptMenu.SetPosition(0, gap, ScrX, gap);

		if (LastCommand) {
			MenuDataEx *VMenuTable[] = {FileMenu, EditMenu, NavigateMenu, ViewMenu};
			int HItemToShow = LastHItem;

			MainMenu[0].Selected = 0;
			MainMenu[HItemToShow].Selected = 1;
			VMenuTable[HItemToShow][0].SetSelect(0);
			VMenuTable[HItemToShow][LastVItem].SetSelect(1);
			HOptMenu.Show();
			HOptMenu.ProcessKey(KEY_DOWN);
		}
		else if (MouseEvent) {
			HOptMenu.Show();
			HOptMenu.ProcessMouse(MouseEvent);
		}
		else {
			HOptMenu.Show();
			HOptMenu.ProcessKey(KEY_DOWN);
		}

		HOptMenu.Process();

		HOptMenu.GetExitCode(HItem, VItem);
	}

	if (HItem >= 0 && VItem >= 0) {
		FarKey key = MainMenu[HItem].SubMenu[VItem].AccelKey;
		fileEditor->ProcessMenuCommand(HItem, VItem, key);
	}

	if (HItem != -1 && VItem != -1) {
		LastHItem = HItem;
		LastVItem = VItem;
	}
}
